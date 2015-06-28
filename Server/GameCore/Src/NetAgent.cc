#include "NetAgent.hpp"
#include "Config.hpp"
#include "NetID.hpp"
#include "GCAgent.hpp"
#include "Time.hpp"
#include "Debug.hpp"
#include "SignalHandler.hpp"
#include "NetProto.pb.h"
#include <zmq.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <list>
#include <string>

using namespace std;

#define BACKLOG 32
#define READQUEUE 128
#define KEEPALIVE_IDLE 5
#define KEEPALIVE_INTERVAL 2
#define KEEPALIVE_COUNT 3

static struct{
	int listenFD;
	int epollFD;
}netServer;

static struct{
	zmq::socket_t *gc;
	int fd;
	list<string> toGCCache;
}agent;


static int SetNonblocking(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
		return -1;
	return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

/*
static int SetKeepAlive(int fd) {
	int keepalive = 1;
	int keepidle = KEEPALIVE_IDLE;
	int keepinterval = KEEPALIVE_INTERVAL;
	int keepcount = KEEPALIVE_COUNT;
	int ret = 0;
	ret |= setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
	ret |= setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
	ret |= setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
	ret |= setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));
	return ret;
}
*/

static int AddToEpoll(int fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	return epoll_ctl(netServer.epollFD, EPOLL_CTL_ADD, fd, &ev);
}

static int DelFromEpoll(int fd) {
	return epoll_ctl(netServer.epollFD, EPOLL_CTL_DEL, fd, NULL);
}

static void SendMsgToGCAgent(zmq::message_t *msg, bool noblock = true) {
	while (!agent.toGCCache.empty()) {
		static zmq::message_t cache;
		cache.rebuild((void *)agent.toGCCache.front().data(), agent.toGCCache.front().size(), NULL);
		if (!agent.gc->send(cache, noblock ? ZMQ_NOBLOCK : 0)) {
			if (msg != NULL) {
				agent.toGCCache.push_back(string());
				agent.toGCCache.back().assign((const char *)msg->data(), msg->size());
			}
			return;
		}
		agent.toGCCache.pop_front();
	}
	if (msg != NULL) {
		if (!agent.gc->send(*msg, noblock ? ZMQ_NOBLOCK : 0)) {
			agent.toGCCache.push_back(string());
			agent.toGCCache.back().assign((const char *)msg->data(), msg->size());
		}
	}
}

static void SendOrderToGCAgent(u_int8_t order, int32_t arg1) {
	switch(order) {
		case ORDER_CLOSE_CLIENT:
			{
				int32_t id = arg1;
				if (id == -1)
					break;

				zmq::message_t msg(5);
				*((u_int8_t *)msg.data()) = order;
				*(int32_t *)((int8_t *)msg.data() + 1) = id;

				SendMsgToGCAgent(&msg);
			}
			break;

		case ORDER_NEW_CLIENT:
			{
				int32_t id = arg1;
				if (id == -1)
					break;

				zmq::message_t msg(5);
				*((u_int8_t *)msg.data()) = order;
				*(int32_t *)((int8_t *)msg.data() + 1) = id;

				SendMsgToGCAgent(&msg);
			}
			break;

		default:
			break;
	}
}

static void AddClient(int fd, const char *ip) {
	if (SetNonblocking(fd) == -1) {
		close(fd);
		return;
	}

	/*
	   if (SetKeepAlive(fd) != 0) {
	   close(fd);
	   return;
	   }
	   */

	int32_t id = NetID_AddFD(fd, ip);
	if (id == -1) {
		close(fd);
		return;
	}

	if (AddToEpoll(fd) == -1) {
		close(fd);
		NetID_DelFD(fd);
		return;
	}

	SendOrderToGCAgent(ORDER_NEW_CLIENT, id);
	DEBUG_LOG("AddClient, id: %d, fd: %d, ip: %s", id, fd, ip == NULL ? "unknown" : ip);
}

static void DelClient(int fd) {
	int32_t id = NetID_ID(fd);

	DEBUG_LOG("DelClient, id: %d, fd: %d", id, fd);

	DelFromEpoll(fd);
	NetID_DelFD(fd);
	close(fd);
	if (id != -1)
		SendOrderToGCAgent(ORDER_CLOSE_CLIENT, id);
}

static void InitNetServer() {
	if ((netServer.listenFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		DEBUG_LOGERROR("Failed to create listen socket");
		exit(EXIT_FAILURE);
	}
	if (SetNonblocking(netServer.listenFD) == -1) {
		DEBUG_LOGERROR("Failed to set nonblocking");
		exit(EXIT_FAILURE);
	}

	int reuse = 1;
	setsockopt(netServer.listenFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Config_NetAgentPort());
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(netServer.listenFD, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		DEBUG_LOGERROR("Failed to bind listen socket->%d", Config_NetAgentPort());
		exit(EXIT_FAILURE);
	}
	if (listen(netServer.listenFD, BACKLOG) == -1) {
		DEBUG_LOGERROR("Failed to listen");
		exit(EXIT_FAILURE);
	}

	if ((netServer.epollFD = epoll_create(Config_MaxPlayers()/* + 2*/)) == -1) {
		DEBUG_LOGERROR("Failed to create epoll");
		exit(EXIT_FAILURE);
	}
	if (AddToEpoll(netServer.listenFD) == -1) {
		DEBUG_LOGERROR("Failed to add to epoll");
		exit(EXIT_FAILURE);
	}

	size_t fdSize = sizeof(agent.fd);
	agent.gc->getsockopt(ZMQ_FD, &agent.fd, &fdSize);
	if (AddToEpoll(agent.fd) == -1) {
		DEBUG_LOGERROR("Failed to add to epoll");
		exit(EXIT_FAILURE);
	}
}

void NetAgent_Init(zmq::context_t *context) {
	assert(context != NULL);
	agent.gc = new zmq::socket_t(*context, ZMQ_PAIR);
	char addr[128];
	SNPRINTF1(addr, "inproc://GC-Net-%d", Config_NetAgentPort());
	agent.gc->connect(addr);

	InitNetServer();
}

static void ProcessClient(int fd, int32_t id) {
	for (;;) {
		int bufLen = 0;
		void *buf = NetID_RecvBuf(id, &bufLen);
		if (buf == NULL)
			DEBUG_LOGERROR("Failed to get recvbuf, id: %d, fd: %d", id, fd);
		assert(buf != NULL);
		assert(bufLen > 0);

		int num = recv(fd, buf, bufLen, 0);
		if (num == 0) {
			DEBUG_LOG("Close connection by client");
			DelClient(fd);
			return;
		}
		else if (num < 0) {
			switch(errno) {
				case EAGAIN:
					return;
				case EINTR:
					throw zmq::error_t();
				default:
					// DEBUG_LOGERROR("Failed to recv data, err: %s", strerror(errno));
					DelClient(fd);
					return;
			}
		}
		else {
			NetID_AddDataLen(id, num);

			zmq::message_t gcMsg;
			int cur = NetID_ExtraMsg(id, 0, &gcMsg);
			for (;;) {
				if (cur == -1)
					break;
				if (cur == -100) {
					const char *ip = NetID_IP(id);
					if (ip == NULL)
						ip = "";
					DEBUG_LOGERROR("Client is overload, ip: %s", ip);
					DelClient(fd);
					return;
				}
				if (cur <= -2) {
					DEBUG_LOGERROR("Failed to extra net msg, error: %d", cur);
					DelClient(fd);
					return;
				}

				if (gcMsg.size() >= 6) {
					int groupID = *((u_int8_t *)gcMsg.data() + 4);
					int unitID = *((u_int8_t *)gcMsg.data() + 5);
					if (groupID == NetProto_GMRoleCount::GROUPID
							&& unitID == NetProto_GMRoleCount::UNITID) {
						DEBUG_LOGRECORD("NetAgent recv NetProto_RoleCount, send to GCAgent");
					}
				}
				SendMsgToGCAgent(&gcMsg);
				cur = NetID_ExtraMsg(id, cur, &gcMsg);
			}
		}
	}
}

static void FlushSendCache() {
	static vector<int> fds;
	fds.clear();
	NetID_SendDataFD(&fds);
	for (size_t i = 0; i < fds.size(); i++) {
		int fd = fds[i];
		const string *sendCache = NetID_SendData(fd);
		size_t total = 0;
		for (;;) {
			bool over = false;
			ssize_t num = send(fd, sendCache->data() + total, sendCache->size() - total, 0);
			if (num < 0) {
				switch(errno) {
					case EINTR:
						throw zmq::error_t();
					case EAGAIN:
						{
							if (total > 0) {
								string temp = *sendCache;
								NetID_PopSendData(fd);
								NetID_PushSendData(fd, temp.data() + total, temp.size() - total);
							}
							over = true;
							break;
						}
					default:
						DelClient(fd);
						over = true;
						break;
				}
			}

			if (over)
				break;

			total += (size_t)num;
			if (total >= sendCache->size()) {
				NetID_PopSendData(fd);
				break;
			}
		}
	}
}

static void SendToClient(int32_t id, const void *buf, size_t size) {
	FlushSendCache();

	int fd = NetID_FD(id);
	if (fd == -1)
		return;

	u_int8_t msgLen[5];
	const void *part[2];
	size_t partLen[2];
	if (size < 0xFF) {
		msgLen[0] = (u_int8_t)size;
		part[0] = &msgLen[0];
		part[1] = buf;
		partLen[0] = 1;
		partLen[1] = size;
	} else {
		msgLen[0] = (u_int8_t)0xFF;
		*(int32_t *)&msgLen[1] = (int32_t)size;
		part[0] = &msgLen[0];
		part[1] = buf;
		partLen[0] = 5;
		partLen[1] = size;
	}

	if (NetID_SendData(fd) != NULL) {
		static string sendBuf;
		sendBuf.clear();
		sendBuf.append((const char *)part[0], partLen[0]);
		sendBuf.append((const char *)part[1], partLen[1]);
		NetID_PushSendData(fd, sendBuf.data(), sendBuf.size());
		return;
	}

	size_t i = 0;
	size_t total = 0;
	for (;;) {
		ssize_t num = send(fd, (int8_t *)part[i] + total, partLen[i] - total, 0);
		if (num < 0) {
			switch(errno) {
				case EAGAIN:
					{
						static string sendBuf;
						sendBuf.clear();
						if (i == 0) {
							sendBuf.append((const char *)part[0] + total, partLen[0] - total);
							sendBuf.append((const char *)part[1], partLen[1]);
						}
						else if (i == 1) {
							sendBuf.append((const char *)part[1] + total, partLen[1] - total);
						}
						else {
							assert(0);
						}
						NetID_PushSendData(fd, sendBuf.data(), sendBuf.size());
						return;
					}
				case EINTR:
					throw zmq::error_t();
				default:
					DelClient(fd);
					return;
			}
		}

		total += (size_t)num;
		if (total >= partLen[i]) {
			i++;
			total = 0;
			if (i >= sizeof(part) / sizeof(part[0]))
				break;
		}
	}
}

static bool ProcessLocalOrder(zmq::message_t *msg) {
	if (msg->size() < 1)
		return false;

	u_int8_t order = *(u_int8_t *)msg->data();
	switch(order) {
		case ORDER_CLOSE_CLIENT: // Close client
			{
				if (msg->size() != 5)
					break;

				int32_t id = *(int32_t *)((int8_t *)msg->data() + 1);
				int fd = NetID_FD(id);
				if (fd != -1)
					DelClient(fd);

				return true;
			}
			break;

		case ORDER_PROTECT_ID:
			{
				if (msg->size() != 5)
					break;

				int32_t id = *(int32_t *)((int8_t *)msg->data() + 1);
				NetID_ProtectID(id);
				return true;
			}
			break;

		case ORDER_COLLECT_ID:
			{
				if (msg->size() != 5)
					break;

				int32_t id = *(int32_t *)((int8_t *)msg->data() + 1);
				NetID_CollectID(id);
				return true;
			}
			break;

		case ORDER_EXCHANGE_ID:
			{
				if (msg->size() != 9)
					break;

				int32_t prev = *(int32_t *)((int8_t *)msg->data() + 1);
				int32_t cur = *(int32_t *)((int8_t *)msg->data() + 5);
				int res = NetID_ExchangeID(prev, cur);
				if (res != 0)
					DEBUG_LOGERROR("Failed to exchange id, err: %d, prev: %d, cur: %d", res, prev, cur);
				return true;
			}
			break;

		default:
			break;
	}

	return false;
}

static void ProcessGameCoreSide() {
	zmq::message_t gcMsg;

	for (;;) {
		if (!agent.gc->recv(&gcMsg, ZMQ_NOBLOCK))
			break;

		if (ProcessLocalOrder(&gcMsg))
			continue;

		// First int32_t is count of clients the msg should be sent to.
		// >0: count id(0) id(1) ... id(count - 1)
		// -1: Send to all clients.
		// -2: Send to all clients except one.
		int32_t count = *(int32_t *)gcMsg.data();

		if (count > 0) {
			if (gcMsg.size() - 4 * (1 + count) >= 2) {
				int groupID = *(u_int8_t *)((int32_t *)gcMsg.data() + (1 + count));
				int unitID = *((u_int8_t *)((int32_t *)gcMsg.data() + (1 + count)) + 1);
				if (groupID == NetProto_GMRoleCount::GROUPID
						&& unitID == NetProto_GMRoleCount::UNITID) {
					DEBUG_LOGRECORD("NetAgent recv NetProto_RoleCount, send to client");
				}
			}
			for (int32_t i = 0; i < count; i++) {
				int32_t id = *((int32_t *)gcMsg.data() + i + 1);
				SendToClient(id, (int32_t *)gcMsg.data() + 1 + count, gcMsg.size() - 4 * (1 + count));
			}
		}
		else if (count == -1) {
			static vector<int32_t> ids;
			ids.clear();
			NetID_AllID(&ids);
			for (size_t i = 0; i < ids.size(); i++)
				SendToClient(ids[i], (int32_t *)gcMsg.data() + 1, gcMsg.size() - 4);
		}
		else if (count == -2) {
			int32_t id = *((int32_t *)gcMsg.data() + 1);
			static vector<int32_t> ids;
			ids.clear();
			NetID_AllID(&ids);
			for (size_t i = 0; i < ids.size(); i++) {
				if (ids[i] == id)
					continue;
				SendToClient(ids[i], (int32_t *)gcMsg.data() + 2, gcMsg.size() - 8);
			}
		}
	}
}

void NetAgent_ProcessAlways() {
	if (agent.gc == NULL)
		return;

	while (!SignalHandler_IsOver()) {
		struct epoll_event ev[READQUEUE];
		int n = epoll_wait(netServer.epollFD, ev, READQUEUE, 1000);
		if (n > 0) {
			for (int i = 0; i < n; i++) {
				if (ev[i].data.fd == netServer.listenFD) {
					struct sockaddr_in addr;
					socklen_t len = sizeof(addr);
					int fd = accept(netServer.listenFD, (struct sockaddr *)&addr, &len);
					if (fd == -1)
						continue;

					// getpeername(fd, (struct sockaddr *)&addr,&len);
					AddClient(fd, inet_ntoa(addr.sin_addr));
					continue;
				}

				int32_t id = NetID_ID(ev[i].data.fd);
				if (id == -1) {
					if (ev[i].data.fd != agent.fd)
						DelClient(ev[i].data.fd);
					continue;
				}

				if (ev[i].data.fd != agent.fd)
					ProcessClient(ev[i].data.fd, id);
			}

			uint32_t zmq_events;
			size_t zmq_events_size = sizeof(zmq_events);
			agent.gc->getsockopt(ZMQ_EVENTS, &zmq_events, &zmq_events_size);
			if (zmq_events & ZMQ_POLLIN)
				ProcessGameCoreSide();
		}
		FlushSendCache();
		SendMsgToGCAgent(NULL);
	}
}
