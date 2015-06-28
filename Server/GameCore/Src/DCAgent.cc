#include "DCAgent.hpp"
#include "Config.hpp"
#include "Time.hpp"
#include "SignalHandler.hpp"
#include "DCMsgProcessor.hpp"
#include <zmq.hpp>
#include <sys/types.h>
#include <iostream>
#include <cstdarg>
#include <pthread.h>
#include <cstdio>
#include <list>
#include <string>
#include <cmath>

using namespace std;

struct Agent{
	zmq::socket_t *gcAgent;
	list<string> toGCCache;
};

static struct Agent agent;

void DCAgent_Init(zmq::context_t *context) {
	assert(context != NULL);
	agent.gcAgent = new zmq::socket_t(*context, ZMQ_PAIR);
	char addr[128];
	SNPRINTF1(addr, "inproc://GC-DC-%d", Config_NetAgentPort());
	agent.gcAgent->connect(addr);

	agent.toGCCache.clear();
}

static void ProcessGCSide() {
	zmq::message_t gcAgentMsg;
	while (agent.gcAgent->recv(&gcAgentMsg, ZMQ_NOBLOCK)) {
		Time_Flush();
		int64_t begin = Time_ElapsedTime();
		DCMsgProcessor_ProcessDCMsg(&gcAgentMsg);
		Time_Flush();
		int64_t end = Time_ElapsedTime();
		if (end - begin > 200) {
			if (gcAgentMsg.size() >= 2) {
				int groupID = *((u_int8_t *)gcAgentMsg.data());
				int unitID = *((u_int8_t *)gcAgentMsg.data() + 1);
				DEBUG_LOGRECORD("process dc msg too long, group: %d, unit: %d, time: %lld", groupID, unitID, end - begin);
			}
		}
	}
}

/*
   void DCAgent_ProcessAlways() {
   if (agent.gcAgent == NULL)
   return;

   zmq_pollitem_t pollitems[1];
   memset(pollitems, 0, sizeof(pollitems));

   pollitems[0].socket = (void *)*agent.gcAgent;
   pollitems[0].events |= ZMQ_POLLIN;

   while (!SignalHandler_IsGCOver()) {
   int n = zmq::poll(pollitems, sizeof(pollitems) / sizeof(pollitems[0]), -1);
   for (size_t i = 0; i < sizeof(pollitems) / sizeof(pollitems[0]); i++) {
   if (pollitems[i].revents == 0)
   continue;

   if (pollitems[i].revents & ZMQ_POLLIN) {
   if (i == 0) {
   ProcessGCSide();
   }
   n--;
   }
   }
   assert(n == 0);
   }
   }
 */

void DCAgent_Process(int32_t timeout) {
	if (agent.gcAgent == NULL)
		return;

	zmq_pollitem_t pollitems[1];
	memset(pollitems, 0, sizeof(pollitems));

	pollitems[0].socket = (void *)*agent.gcAgent;
	pollitems[0].events |= ZMQ_POLLIN;

	/*
	   int64_t begin = Time_ElapsedTime();
	   for (int32_t wt = (int32_t)timeout; wt >= 0; wt -= (int32_t)(fabs(Time_ElapsedTime() - begin))) {
	   int n = zmq::poll(pollitems, sizeof(pollitems) / sizeof(pollitems[0]), wt * 1000);
	   for (size_t i = 0; i < sizeof(pollitems) / sizeof(pollitems[0]); i++) {
	   if (pollitems[i].revents == 0)
	   continue;

	   if (pollitems[i].revents & ZMQ_POLLIN) {
	   if (i == 0) {
	   ProcessGCSide();
	   }
	   n--;
	   }
	   }
	   assert(n == 0);
	   Time_Flush();
	   }
	 */

	zmq::poll(pollitems, sizeof(pollitems) / sizeof(pollitems[0]), timeout * 1000);
	for (size_t i = 0; i < sizeof(pollitems) / sizeof(pollitems[0]); i++) {
		if (pollitems[i].revents == 0)
			continue;

		if (pollitems[i].revents & ZMQ_POLLIN) {
			if (i == 0) {
				ProcessGCSide();
			}
		}
	}

	// ProcessGCSide();

	DCAgent_SendMsgToGCAgent(NULL, true);
}

void DCAgent_SendMsgToGCAgent(zmq::message_t *msg, bool noblock) {
	if (agent.gcAgent == NULL)
		return;

	while (!agent.toGCCache.empty()) {
		static zmq::message_t cache;
		cache.rebuild((void *)agent.toGCCache.front().data(), agent.toGCCache.front().size(), NULL);
		if (!agent.gcAgent->send(cache, noblock ? ZMQ_NOBLOCK : 0)) {
			if (msg != NULL) {
				agent.toGCCache.push_back(string());
				agent.toGCCache.back().assign((const char *)msg->data(), msg->size());
			}
			return;
		}
		agent.toGCCache.pop_front();
	}
	if (msg != NULL) {
		if (!agent.gcAgent->send(*msg, noblock ? ZMQ_NOBLOCK : 0)) {
			agent.toGCCache.push_back(string());
			agent.toGCCache.back().assign((const char *)msg->data(), msg->size());
		}
	}
}
