#include "GCAgent.hpp"
#include "GCMsgProcessor.hpp"
#include "Time.hpp"
#include "ConnectionPool.hpp"
#include <zmq.hpp>
#include <sys/types.h>
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <list>
#include <string>

using namespace std;

static struct{
	zmq::context_t *context;
	zmq::socket_t *netAgent;
	zmq::socket_t *dcAgent;
	list<string> toNetCache;
	list<string> toDCCache;
}agent;

void GCAgent_Init() {
	agent.context = new zmq::context_t(4);

	agent.netAgent = new zmq::socket_t(*agent.context, ZMQ_PAIR);
	char addr[128];
	SNPRINTF1(addr, "inproc://GC-Net-%d", Config_NetAgentPort());
	agent.netAgent->bind(addr);

	agent.dcAgent = new zmq::socket_t(*agent.context, ZMQ_PAIR);
	SNPRINTF1(addr, "inproc://GC-DC-%d", Config_NetAgentPort());
	agent.dcAgent->bind(addr);

	agent.toNetCache.clear();
	agent.toDCCache.clear();
}

zmq::context_t * GCAgent_Context() {
	return agent.context;
}

void GCAgent_SendOrderToNetAgent(u_int8_t order, int32_t arg1, int32_t arg2) {
	switch(order) {
		case ORDER_CLOSE_CLIENT:
			{
				int32_t id = arg1;
				ConnectionPool_Del(id);

				zmq::message_t msg(5);
				*((u_int8_t *)msg.data()) = order;
				*(int32_t *)((int8_t *)msg.data() + 1) = id;

				GCAgent_SendMsgToNetAgent(&msg, true);
			}
			break;

		case ORDER_PROTECT_ID:
		case ORDER_COLLECT_ID:
			{
				int32_t id = arg1;

				zmq::message_t msg(5);
				*((u_int8_t *)msg.data()) = order;
				*(int32_t *)((int8_t *)msg.data() + 1) = id;

				GCAgent_SendMsgToNetAgent(&msg, true);
			}
			break;

		case ORDER_EXCHANGE_ID:
			{
				int32_t prev = arg1;
				int32_t cur = arg2;

				zmq::message_t msg(9);
				*((u_int8_t *)msg.data()) = order;
				*(int32_t *)((int8_t *)msg.data() + 1) = prev;
				*(int32_t *)((int8_t *)msg.data() + 5) = cur;

				GCAgent_SendMsgToNetAgent(&msg, true);
			}
			break;

		default:
			break;
	}
}

static void ProcessNetAgentSide() {
	zmq::message_t netAgentMsg;
	while (agent.netAgent->recv(&netAgentMsg, ZMQ_NOBLOCK)) {
		GCMsgProcessor_ProcessNetMsg(&netAgentMsg);
	}
}

static void ProcessDCAgentSide() {
	zmq::message_t dcAgentMsg;
	while (agent.dcAgent->recv(&dcAgentMsg, ZMQ_NOBLOCK)) {
		GCMsgProcessor_ProcessDCMsg(&dcAgentMsg);
	}
}

void GCAgent_Process(int32_t timeout) {
	zmq_pollitem_t pollitems[2];
	memset(pollitems, 0, sizeof(pollitems));

	pollitems[0].socket = (void *)*agent.netAgent;
	pollitems[0].events |= ZMQ_POLLIN;

	pollitems[1].socket = (void *)*agent.dcAgent;
	pollitems[1].events |= ZMQ_POLLIN;

	int64_t begin = Time_ElapsedTime();
	for (int32_t wt = (int32_t)timeout; wt >= 0; wt -= (int32_t)(Time_ElapsedTime() - begin)) {
		int n = zmq::poll(pollitems, sizeof(pollitems) / sizeof(pollitems[0]), wt * 1000);
		for (size_t i = 0; i < sizeof(pollitems) / sizeof(pollitems[0]); i++) {
			if (pollitems[i].revents == 0)
				continue;

			if (pollitems[i].revents & ZMQ_POLLIN) {
				if (i == 0) {
					ProcessNetAgentSide();
				}
				else if (i == 1) {
					ProcessDCAgentSide();
				}
				n--;
			}
		}
		assert(n == 0);
		Time_Flush();
	}
	GCAgent_SendMsgToNetAgent(NULL, true);
	GCAgent_SendMsgToDCAgent(NULL, true);
}

void GCAgent_WaitDCAgent(int count) {
	zmq_pollitem_t pollitems[1];
	memset(pollitems, 0, sizeof(pollitems));

	pollitems[0].socket = (void *)*agent.dcAgent;
	pollitems[0].events |= ZMQ_POLLIN;

	for (int i = 0; i < count; ) {
		zmq::poll(pollitems, sizeof(pollitems) / sizeof(pollitems[0]), -1);
		if (pollitems[0].revents == 0)
			continue;

		if (pollitems[0].revents & ZMQ_POLLIN) {
			ProcessDCAgentSide();
			i++;
		}
	}
}

void GCAgent_SendMsgToNetAgent(zmq::message_t *msg, bool noblock) {
	if (agent.netAgent == NULL)
		return;

	while (!agent.toNetCache.empty()) {
		static zmq::message_t cache;
		cache.rebuild((void *)agent.toNetCache.front().data(), agent.toNetCache.front().size(), NULL);
		if (!agent.netAgent->send(cache, noblock ? ZMQ_NOBLOCK : 0)) {
			if (msg != NULL) {
				agent.toNetCache.push_back(string());
				agent.toNetCache.back().assign((const char *)msg->data(), msg->size());
			}
			return;
		}
		agent.toNetCache.pop_front();
	}
	if (msg != NULL) {
		if (!agent.netAgent->send(*msg, noblock ? ZMQ_NOBLOCK : 0)) {
			agent.toNetCache.push_back(string());
			agent.toNetCache.back().assign((const char *)msg->data(), msg->size());
		}
	}
}

void GCAgent_SendMsgToDCAgent(zmq::message_t *msg, bool noblock) {
	if (agent.dcAgent == NULL)
		return;

	while (!agent.toDCCache.empty()) {
		static zmq::message_t cache;
		cache.rebuild((void *)agent.toDCCache.front().data(), agent.toDCCache.front().size(), NULL);
		if (!agent.dcAgent->send(cache, noblock ? ZMQ_NOBLOCK : 0)) {
			if (msg != NULL) {
				agent.toDCCache.push_back(string());
				agent.toDCCache.back().assign((const char *)msg->data(), msg->size());
			}
			return;
		}
		agent.toDCCache.pop_front();
	}
	if (msg != NULL) {
		if (!agent.dcAgent->send(*msg, noblock ? ZMQ_NOBLOCK : 0)) {
			agent.toDCCache.push_back(string());
			agent.toDCCache.back().assign((const char *)msg->data(), msg->size());
			return;
		}
	}
}
