#include "NetID.hpp"
#include "Config.hpp"
#include "IDGenerator.hpp"
#include "Debug.hpp"
#include <zmq.hpp>
#include <sys/types.h>
#include <map>
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <string>
#include <vector>

using namespace std;

#define RECVBUF (1024 * 20)
#define OVERLOAD_RESET_TIME (1000 * 5)
#define OVERLOAD_COUNT (OVERLOAD_RESET_TIME / 1000 * 50)

struct Overload {
	int64_t t;
	int n;
};

static struct{
	map<int, int32_t> fdToID;
	int *idToFD;
	char *idToIP;
	int max;
	void **idToBuf;
	int *idToBufPos;
	struct Overload *idToOverload;
	struct IDGenerator *idGenerator;
	map<int, string> sendBuf;
	bool *protectID;
}package;

void NetID_Init() {
	package.max = Config_MaxPlayers();

	package.idToFD = new int[package.max];
	for (int i = 0; i < package.max; i++)
		package.idToFD[i] = -1;

	package.idToIP = new char[package.max * IP_BUF_LEN];
	memset(package.idToIP, '\0', package.max * IP_BUF_LEN);

	package.idToBuf = (void **)malloc(package.max * sizeof(void *));
	package.idToBufPos = new int[package.max];
	for (int i = 0; i < package.max; i++) {
		package.idToBuf[i] = NULL;
		package.idToBufPos[i] = 0;
	}

	package.idToOverload = new struct Overload[package.max];
	for (int i = 0; i < package.max; i++) {
		package.idToOverload[i].t = 0;
		package.idToOverload[i].n = 0;
	}

	package.sendBuf.clear();

	package.protectID = new bool[package.max];
	for (int i = 0; i < package.max; i++)
		package.protectID[i] = false;

	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
}

int32_t NetID_AddFD(int fd, const char *ip) {
	int32_t id = IDGenerator_Gen(package.idGenerator);
	for (;;) {
		if (id == -1)
			return -1;
		if (package.protectID[id])
			id = IDGenerator_Gen(package.idGenerator);
		else
			break;
	}

	if (!package.fdToID.insert(make_pair(fd, id)).second) {
		IDGenerator_Release(package.idGenerator, id);
		return -1;
	}

	package.idToFD[id] = fd;
	package.idToBufPos[id] = 0;
	if (package.idToBuf[id] == NULL)
		package.idToBuf[id] = malloc(RECVBUF);
	strncpy(&package.idToIP[id * IP_BUF_LEN], ip, IP_BUF_LEN - 1);

	package.idToOverload[id].t = Time_ElapsedTime();
	package.idToOverload[id].n = 0;

	return id;
}

void NetID_ProtectID(int32_t id) {
	if (id < 0 || id >= package.max)
		return;
	package.protectID[id] = true;
}

void NetID_CollectID(int32_t id) {
	if (id < 0 || id >= package.max)
		return;
	package.protectID[id] = false;
}

int NetID_ExchangeID(int32_t prev, int32_t cur) {
	if (prev < 0 || prev >= package.max)
		return -1;
	if (cur < 0 || cur >= package.max)
		return -2;
	if (prev == cur)
		return -3;

	if (package.idToFD[prev] == -1)
		return -4;
	if (package.idToFD[cur] != -1)
		return -5;

	package.idToFD[cur] = package.idToFD[prev];
	package.idToFD[prev] = -1;

	package.fdToID[package.idToFD[cur]] = cur;

	package.idToBufPos[cur] = package.idToBufPos[prev];
	if (package.idToBufPos[prev] > 0)
		memcpy(package.idToBuf[cur], package.idToBuf[prev], package.idToBufPos[prev]);
	package.idToBufPos[prev] = 0;
	// strncpy(&package.idToIP[cur * IP_BUF_LEN], &package.idToIP[prev * IP_BUF_LEN], IP_BUF_LEN - 1);

	// NetID_CollectID(cur);
	IDGenerator_Release(package.idGenerator, prev);
	DEBUG_LOG("Exchange id, prev: %d, cur: %d", prev, cur);
	return 0;
}

int32_t NetID_ID(int fd) {
	map<int, int32_t>::iterator it = package.fdToID.find(fd);
	if (it == package.fdToID.end())
		return -1;

	return it->second;
}

int NetID_FD(int32_t id) {
	if (id < 0 || id >= package.max)
		return -1;

	return package.idToFD[id];
}

const char * NetID_IP(int id) {
	if (id < 0 || id >= package.max)
		return NULL;

	return &package.idToIP[id * IP_BUF_LEN];
}

static void DelSendData(int fd) {
	map<int, string>::iterator it = package.sendBuf.find(fd);
	if (it != package.sendBuf.end())
		package.sendBuf.erase(it);
}

void NetID_DelFD(int fd) {
	map<int, int32_t>::iterator it = package.fdToID.find(fd);
	if (it == package.fdToID.end())
		return;

	package.idToFD[it->second] = -1;
	package.idToIP[it->second * IP_BUF_LEN] = '\0';
	package.idToOverload[it->second].t = 0;
	package.idToOverload[it->second].n = 0;
	IDGenerator_Release(package.idGenerator, it->second);
	package.fdToID.erase(it);

	DelSendData(fd);
	NetID_CollectID(it->second);
}

void NetID_DelID(int32_t id) {
	if (id < 0 || id >= package.max)
		return;

	int fd = package.idToFD[id];
	if (fd == -1)
		return;

	package.idToFD[id] = -1;
	package.idToIP[id * IP_BUF_LEN] = '\0';
	package.idToOverload[id].t = 0;
	package.idToOverload[id].n = 0;
	IDGenerator_Release(package.idGenerator, id);
	package.fdToID.erase(fd);

	DelSendData(fd);
	NetID_CollectID(id);
}

void * NetID_RecvBuf(int32_t id, int *len) {
	if (id < 0 || id >= package.max || len == NULL)
		return NULL;
	if (package.idToFD[id] == -1)
		return NULL;

	*len = RECVBUF - package.idToBufPos[id];
	return (char *)package.idToBuf[id] + package.idToBufPos[id];
}

void NetID_AddDataLen(int32_t id, int len) {
	if (id < 0 || id >= package.max)
		return;
	assert(len >= 0 && len <= RECVBUF);
	if (package.idToFD[id] == -1)
		return;

	package.idToBufPos[id] += len;
	assert(package.idToBufPos[id] <= RECVBUF);
}

void NetID_ClearRecvBuf(int32_t id) {
	if (id < 0 || id >= package.max)
		return;
	if (package.idToFD[id] == -1)
		return;

	package.idToBufPos[id] = 0;
}

static void ResetBuf(int32_t id, int cur) {
	if (cur <= 0)
		return;

	int size = package.idToBufPos[id] - cur;
	memmove(package.idToBuf[id], (int8_t *)package.idToBuf[id] + cur, size);
	package.idToBufPos[id] -= cur;
}

static bool CheckOverload(int32_t id) {
	struct Overload *overload = &package.idToOverload[id];
	overload->n++;
	if (overload->n > OVERLOAD_COUNT)
		return true;
	int64_t cur = Time_ElapsedTime();
	if (cur - overload->t > OVERLOAD_RESET_TIME) {
		overload->t = cur;
		overload->n = 0;
	}
	return false;
}

int NetID_ExtraMsg(int32_t id, int cur, zmq::message_t *msg) {
	if (id < 0 || id >= package.max || cur < 0 || msg == NULL)
		return -2;
	if (package.idToFD[id] == -1)
		return -3;

	int end = package.idToBufPos[id];
	assert(cur <= end);
	if (end - cur < 1) {
		ResetBuf(id, cur);
		return -1;
	}

	u_int8_t *buf = (u_int8_t *)package.idToBuf[id] + cur;
	int32_t msgLen = *buf;
	if (msgLen != 0xFF) {
		if (msgLen < 2)
			return -4;

		if (end - cur < msgLen + 1) {
			ResetBuf(id, cur);
			return -1;
		}

		if (CheckOverload(id))
			return -100;

		msg->rebuild(4 + msgLen);
		*(int32_t *)msg->data() = id;
		memcpy((int32_t *)msg->data() + 1, buf + 1, msgLen);

		return cur + 1 + msgLen;
	} else {
		if (end - cur < 5) {
			ResetBuf(id, cur);
			return -1;
		}

		msgLen = *(int32_t *)(buf + 1);
		if (msgLen < 2)
			return -5;
		else if (msgLen >= CONFIG_MAX_MSG_SIZE)
			return -6;

		if (end - cur < msgLen + 5) {
			ResetBuf(id, cur);
			return -1;
		}

		if (CheckOverload(id))
			return -100;

		msg->rebuild(4 + msgLen);
		*(int32_t *)msg->data() = id;
		memcpy((int32_t *)msg->data() + 1, buf + 5, msgLen);

		return cur + 5 + msgLen;
	}
}

void NetID_SendDataFD(std::vector<int> *fd) {
	if (fd == NULL)
		return;

	for (map<int, string>::iterator it = package.sendBuf.begin(); it != package.sendBuf.end(); it++) {
		fd->push_back(it->first);
	}
}

const string * NetID_SendData(int fd) {
	map<int, string>::iterator it = package.sendBuf.find(fd);
	if (it == package.sendBuf.end())
		return NULL;
	else
		return &it->second;
}

void NetID_PushSendData(int32_t fd, const void *data, size_t size) {
	if (data == NULL || size <= 0)
		return;

	string &cache = package.sendBuf[fd];
	cache.append((const char *)data, size);
}

void NetID_PopSendData(int fd) {
	map<int, string>::iterator it = package.sendBuf.find(fd);
	if (it != package.sendBuf.end())
		package.sendBuf.erase(it);
}

void NetID_AllFD(std::vector<int> *fds) {
	if (fds == NULL)
		return;

	for (map<int, int32_t>::iterator it = package.fdToID.begin(); it != package.fdToID.end(); it++)
		fds->push_back(it->first);
}

void NetID_AllID(std::vector<int32_t> *ids) {
	if (ids == NULL)
		return;

	for (map<int, int32_t>::iterator it = package.fdToID.begin(); it != package.fdToID.end(); it++)
		ids->push_back(it->second);
}
