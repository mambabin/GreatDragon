#include "ConnectionPool.hpp"
#include "Debug.hpp"
#include "GCAgent.hpp"
#include "AccountPool.hpp"
#include "PlayerEntity.hpp"
#include "Movement.hpp"
#include "Time.hpp"
#include "Timer.hpp"
#include "Config.hpp"
#include "MapPool.hpp"
#include "MapInfoManager.hpp"
#include "NetProto.pb.h"
#include <sys/types.h>

using namespace std;

struct Connection{
	int32_t id;
	int64_t heartbeatTime;
	int64_t prevHB;
	int64_t lastServerTime;
	int64_t step;
	int speedUpCount;
};

static struct{
	int max;
	struct Connection *table;
}package;

void ConnectionPool_Init() {
	package.max = Config_MaxPlayers();
	package.table = new struct Connection[package.max];
	for (int i = 0; i < package.max; i++)
		package.table[i].id = -1;
}

static void UpdateConnection(void *arg) {
	int64_t cur = Time_ElapsedTime();
	for (int i = 0; i < package.max; i++) {
		struct Connection *con = &package.table[i];
		if (con->id == -1)
			continue;

		if (cur - con->heartbeatTime > Config_HeartbeatTime()) {
			// DEBUG_LOGERROR("Has no heartbeat, id: %d", con->id);
			// no drop
			/*
			struct PlayerEntity *player = PlayerEntity_Player(con->id);
			if (player != NULL) {
				struct Component *component = PlayerEntity_Component(player);
				if (Movement_InSingle(component->movement)) {
					AccountPool_Drop(con->id);
					continue;
				}
			}
			*/
			AccountPool_Logout(con->id);
			continue;
		}

		/*
		if (cur - con->prevHB > Config_HeartbeatInterval()) {
			con->prevHB = cur;

			static NetProto_Heartbeat hb;
			GCAgent_SendProtoToClients(&con->id, 1, &hb);
		}
		*/

		if (cur - con->lastServerTime > con->step) {
			con->lastServerTime = cur;
			con->step *= 10;

			static NetProto_ServerTime serverTime;
			serverTime.Clear();
			serverTime.set_time((int64_t)cur);
			serverTime.set_cur((int32_t)Time_TimeStamp());
			GCAgent_SendProtoToClients(&con->id, 1, &serverTime);
		}
	}
}

void ConnectionPool_Prepare() {
	Timer_AddEvent(0, 0, UpdateConnection, NULL, "UpdateConnection");
}

static bool IsValid(int32_t id) {
	return id >= 0 && id < package.max && package.table[id].id != -1;
}

void ConnectionPool_Add(int32_t id) {
	if (IsValid(id))
		return;

	struct Connection *con = &package.table[id];
	con->id = id;
	con->heartbeatTime = Time_ElapsedTime();
	con->lastServerTime = con->heartbeatTime;
	con->step = 1000 * 10;
	con->prevHB = CONFIG_INIT_USE_TIME;
	con->speedUpCount = 0;
}

void ConnectionPool_Del(int32_t id) {
	if (!IsValid(id))
		return;

	package.table[id].id = -1;
}

int ConnectionPool_TotalCount() {
	int count = 0;
	for (int i = 0; i < package.max; i++) {
		struct Connection *con = &package.table[i];
		if (con->id != -1)
			count++;
	}
	return count;
}

void ConnectionPool_DoHeartbeat(int32_t id) {
	if (!IsValid(id))
		return;

	struct Connection *con = &package.table[id];
	int64_t cur = Time_ElapsedTime();
	if (cur - con->heartbeatTime <= Config_SpeedUpInterval()) {
		con->speedUpCount++;
		if (con->speedUpCount >= Config_SpeedUpCount()) {
			AccountPool_Logout(con->id);
			return;
		}
		con->heartbeatTime = cur;
	} else {
		con->heartbeatTime = cur;
		con->speedUpCount = 0;
	}
}

