#include "Movement.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "MovementInfo.hpp"
#include "IDGenerator.hpp"
#include "AStar.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "Math.hpp"
#include "MathUtil.hpp"
#include "Status.hpp"
#include "StatusEntity.hpp"
#include "NPCPool.hpp"
#include "PlayerPool.hpp"
#include "FactionPool.hpp"
#include "GlobalMissionManager.hpp"
#include "ProfessionInfoManager.hpp"
#include "NPCInfoManager.hpp"
#include "ScribeClient.hpp"
#include "AccountPool.hpp"
#include "Time.hpp"
#include "GCAgent.hpp"
#include "NetProto.pb.h"
#include "Debug.hpp"
#include <MD5.hpp>
#include <sys/types.h>
#include <cmath>
#include <cassert>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <climits>
#include <cfloat>
#include <list>

using namespace std;

struct Movement{
	int32_t id;
	MovementAtt *att;
	struct Component *component;
	Vector2i path[MAXSTEPS];
	Vector3f realPath[MAXSTEPS];
	int total;
	int curDest;
	Vector3f moveDir;
	float a;
	float b;
	bool isConstX;
	float constX;
	bool isConstY;
	float constY;
	struct Movement *target;
	Vector2i tPrevCoord;
	float dist;
	Vector2i prevCoord;
	int followDelta;
	int64_t prevFollow;
	int32_t serialNum;
	float speedFactor;
	bool cantMove;
	int32_t nextMap;
	int line;
	int waitRoom;
	int64_t beginWaitTime;
	bool endLoadModel;
	Vector3f offsetPos;
	int32_t multiRoom;
	int curPathNode;
	Vector3f curPathPos;
	int destPathNode;
	Vector3f destPathPos;
	int64_t time;
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct Movement *> pool;
	vector<vector<struct Movement *> > waitRoom;
	vector<int> waitRoomCount;
	int64_t prevRoom;
	list<RoomInfo> multiRoom;
	int32_t multiID;
}package;


void Movement_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
	package.prevRoom = 0;

	package.waitRoom.resize(Config_MaxMaps());
	package.waitRoomCount.resize(Config_MaxMaps(), 0);
	package.multiRoom.clear();
	package.multiID = 0;
}

struct Movement * Movement_Create(MovementAtt *att, struct Component *component) {
	if (att == NULL || component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct Movement *movement = SearchBlock(package.pool, id);
	movement->id = id;
	movement->att = att;
	movement->component = component;
	movement->total = 0;
	movement->curDest = 0;
	movement->target = NULL;
	movement->prevCoord = att->logicCoord();
	movement->serialNum = 0;
	movement->speedFactor = 1.0f;
	movement->cantMove = false;
	movement->nextMap = att->mapID();
	movement->line = 0;
	movement->waitRoom = -1;
	movement->beginWaitTime = CONFIG_INIT_USE_TIME;
	movement->endLoadModel = false;
	movement->multiRoom = -1;
	movement->curPathNode = -1;
	movement->destPathNode = -1;
	movement->time = -1;

	MapPool_UpdateCoord(NULL, &att->logicCoord(), movement);

	return movement;
}

bool Movement_IsValid(struct Movement *movement) {
	return movement != NULL && IDGenerator_IsValid(package.idGenerator, movement->id);
}

void Movement_Destroy(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;
	IDGenerator_Release(package.idGenerator, movement->id);
}

void Movement_Finalize(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;

	MapPool_UpdateCoord(&movement->att->logicCoord(), NULL, movement);
	if (movement->component->player != NULL) {
		const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
		if (info != NULL) {
			if (info->mapType() == MapInfo::PEACE)
				*movement->att->mutable_prevCoord() = movement->att->logicCoord();
		}

		if (movement->att->mapID() == MAP_CHANGING)
			MapPool_Unlink(movement->nextMap, movement);
	}

	Movement_EndWaitRoom(movement);
	Movement_DestroyRoom(movement);
	Movement_LeaveRoom(movement);
}

const MovementAtt * Movement_Att(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return NULL;

	return movement->att;
}

struct Component * Movement_Component(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return NULL;

	return movement->component;
}

int32_t Movement_NextMap(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return -1;

	return movement->nextMap;
}

int Movement_Line(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return -1;
	return movement->line;
}

void Movement_SetLine(struct Movement *movement, int line) {
	if (!Movement_IsValid(movement))
		return;
	movement->line = line;
}

int64_t Movement_Time(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return -1;
	return movement->time;
}

void Movement_SetTime(struct Movement *movement, int time) {
	if (!Movement_IsValid(movement))
		return;
	movement->time = time;
}

bool Movement_CanEnterRoom(struct Movement *movement, int32_t room, bool real, bool noPower) {
	if (!Movement_IsValid(movement))
		return false;

	// assert(movement->att->mapID() != MAP_CHANGING);
	// if (movement->att->mapID() == MAP_CHANGING) {
	//	DEBUG_LOGERROR("Failed to change scene because player is in MAP_CHANGING");
	//	return false;
	// }

	const MapInfo *info = MapInfoManager_MapInfo(real ? room : MapPool_MapInfo(room));
	if (info == NULL)
		return false;
	if (movement->component->player != NULL) {
		if (info->requireMission() != -1 && info->requireMission() != 0) {
			if (Mission_MissionCompleteCount(movement->component->mission, info->requireMission()) <= 0) {
				return false;
			}
		}
	}

	if (movement->component->roleAtt->fightAtt().level() < info->requiredLevel())
		return false;
	if (!noPower && Fight_Power(movement->component->fight) < info->requiredPower())
		return false;

	const ItemPackage *package = Item_Package(movement->component->item);
	if (package->durability() < info->durability())
		return false;

	return true;
}

int Movement_BeginWaitRoom(struct Movement *movement, int32_t room) {
	if (!Movement_IsValid(movement))
		return -1;

	if (movement->waitRoom != -1)
		return -1;

	Fight_EndWaitPVP(movement->component->fight);
	Fight_EndWaitHell(movement->component->fight);
	Movement_DestroyRoom(movement);
	Movement_LeaveRoom(movement);

	int32_t curMap = movement->att->mapID();
	if (curMap < MAP_START)
		return -1;

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(curMap));
	if (info == NULL)
		return -1;
	if (info->mapType() != MapInfo::PEACE)
		return -1;

	info = MapInfoManager_MapInfo(room);
	if (info == NULL)
		return -1;
	if (info->mapType() != MapInfo::ROOM && info->mapType() != MapInfo::ONLY_ROOM)
		return -1;

	//if(info->id() >= Config_TowerBegin() && info->id() <= Config_TowerEnd())
	//	return -1;

	if (!Movement_CanEnterRoom(movement, room, true, false))
		return -1;

	assert(room >= 0 && room < (int)package.waitRoom.size());
	bool done = false;
	for (size_t i = 0; i < package.waitRoom[room].size(); i++) {
		if (package.waitRoom[room][i] == NULL) {
			package.waitRoom[room][i] = movement;
			done = true;
			break;
		}
	}
	if (!done)
		package.waitRoom[room].push_back(movement);
	package.waitRoomCount[room]++;

	movement->waitRoom = room;
	movement->beginWaitTime = Time_ElapsedTime();

	static NetProto_Chat chat;
	chat.Clear();
	chat.set_channel(NetProto_Chat::SYSTEM);
	char buf[CONFIG_FIXEDARRAY];
	if (room >= Config_BossBegin() && room <= Config_BossEnd()) {
		SNPRINTF1(buf, Config_Words(23), movement->component->baseAtt->name(), info->name().c_str());
	}
	else {
		SNPRINTF1(buf, Config_Words(22), movement->component->baseAtt->name(), info->name().c_str());
	}
	chat.set_content(buf);
	GCAgent_SendProtoToAllClients(&chat);

	return 0;
}

void Movement_EndWaitRoom(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;

	if (movement->waitRoom != -1) {
		assert(movement->waitRoom >= 0 && movement->waitRoom < (int)package.waitRoom.size());
		for (size_t i = 0; i < package.waitRoom[movement->waitRoom].size(); i++) {
			if (package.waitRoom[movement->waitRoom][i] == movement) {
				package.waitRoom[movement->waitRoom][i] = NULL;
				package.waitRoomCount[movement->waitRoom]--;
				if (package.waitRoomCount[movement->waitRoom] < 0)
					package.waitRoomCount[movement->waitRoom] = 0;
				break;
			}
		}
		movement->waitRoom = -1;
	}
}

int Movement_RoomWaitCount(int32_t room) {
	if (room < 0 || room >= (int32_t)package.waitRoom.size())
		return 0;
	return package.waitRoomCount[room];
}

static void UpdateWaitRoom(struct Movement *movement) {
	if (movement->waitRoom == -1)
		return;

	int64_t cur = Time_ElapsedTime();
	if (cur - movement->beginWaitTime > Config_MaxWaitRoomTime()) {
		Movement_EndWaitRoom(movement);

		static NetProto_EndWaitRoom endWaitRoom;
		endWaitRoom.Clear();
		int32_t id = PlayerEntity_ID(movement->component->player);
		GCAgent_SendProtoToClients(&id, 1, &endWaitRoom);
		return;
	}

	if (cur - package.prevRoom < Config_WaitRoomInterval())
		return;
	package.prevRoom = cur;

	struct Movement *players[MAX_MUL_TOWER_TEAM_PLAYERS];
	int count = 0;
	for (size_t i = 0; i < package.waitRoom[movement->waitRoom].size(); i++) {
		if (package.waitRoom[movement->waitRoom][i] != NULL) {
			if (!Movement_IsValid(package.waitRoom[movement->waitRoom][i])) {
				package.waitRoom[movement->waitRoom][i] = NULL;
				continue;
			}
			players[count++] = package.waitRoom[movement->waitRoom][i];
			if (count >= Config_MaxTeamPlayers(movement->waitRoom))
				break;
		}
	}

	if (count >= MIN_TEAM_PLAYERS) {
		const MapInfo *info = MapInfoManager_MapInfo(movement->waitRoom);
		assert(info != NULL);

		static Vector2i nextCoord;
		if (!MapInfoManager_EnterCoord(info, 0, &nextCoord)) {
			DEBUG_LOGERROR("Failed to get enter coord");
		} else {
			int32_t room = MapPool_Gen(movement->waitRoom, true);
			if (room == -1) {
				DEBUG_LOGERROR("Failed to gen room");
			} else {
				for (int i = 0; i < count; i++)
					Movement_BeginChangeScene(players[i], room, &nextCoord);
			}
		}
		for (int i = 0; i < count; i++)
			Movement_EndWaitRoom(players[i]);
	}
}

int Movement_MultiRoom(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return -1;
	return movement->multiRoom;
}

void Movement_CreateRoom(struct Movement *movement, int32_t room, bool noPower) {
	if (!Movement_IsValid(movement))
		return;
	if (movement->multiRoom != -1)
		return;

	Fight_EndWaitPVP(movement->component->fight);
	Fight_EndWaitHell(movement->component->fight);
	Movement_EndWaitRoom(movement);

	int32_t curMap = movement->att->mapID();
	if (curMap < MAP_START) {
		// DEBUG_LOGERROR("yes, it's this!");
		return;
	}

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(curMap));
	if (info == NULL)
		return;
	if (info->mapType() != MapInfo::PEACE)
		return;

	info = MapInfoManager_MapInfo(room);
	if (info == NULL)
		return;
	if (info->mapType() != MapInfo::ROOM && info->mapType() != MapInfo::ONLY_ROOM)
		return;

	if(info->id() >= Config_TowerBegin() && info->id() <= Config_TowerEnd())
		return;

	int mulTowerType = Config_MulTowerType(room);
	if (mulTowerType == 0 || mulTowerType == 2)
		return;

	int groupID = Config_MulTowerGroup(info->id());
	if (groupID != -1) {
		int event = movement->component->playerAtt->dayEvent(48);
		int count = (event & (3 << (groupID * 2))) >> (groupID * 2);
		if (count >= Config_MaxMulTowerCount())
			return;
	}

	if (!Movement_CanEnterRoom(movement, room, true, false))
		return;

	if (package.multiID >= INT_MAX)
		package.multiID = 0;

	static RoomInfo roomInfo;
	roomInfo.set_id(package.multiID++);
	roomInfo.set_map(room);
	roomInfo.set_leader(movement->component->baseAtt->name());
	roomInfo.set_count(0);
	roomInfo.set_noPower(noPower);
	package.multiRoom.push_back(roomInfo);

	static NetProto_CreateRoom cr;
	cr.Clear();
	cr.set_map(room);
	cr.set_noPower(noPower);
	*cr.mutable_info() = roomInfo;
	int32_t playerID = PlayerEntity_ID(movement->component->player);
	GCAgent_SendProtoToClients(&playerID, 1, &cr);

	Movement_JoinRoom(movement, roomInfo.id(), false);

	static NetProto_Chat chat;
	chat.Clear();
	chat.set_channel(NetProto_Chat::SYSTEM);
	char buf[CONFIG_FIXEDARRAY];
	if (room >= Config_BossBegin() && room <= Config_BossEnd()) {
		SNPRINTF1(buf, Config_Words(23), movement->component->baseAtt->name(), info->name().c_str());
	}
	else {
		SNPRINTF1(buf, Config_Words(22), movement->component->baseAtt->name(), info->name().c_str());
	}
	chat.set_content(buf);
	GCAgent_SendProtoToAllClients(&chat);
}

RoomInfo * Movement_SearchRoomInfo(int32_t id) {
	for (list<RoomInfo>::iterator it = package.multiRoom.begin(); it != package.multiRoom.end(); it++)
		if (it->id() == id)
			return &(*it);
	return NULL;
}

void Movement_JoinRoom(struct Movement *movement, int32_t id, bool noPower) {
	if (!Movement_IsValid(movement))
		return;

	if (movement->multiRoom != -1)
		return;
	Fight_EndWaitPVP(movement->component->fight);
	Fight_EndWaitHell(movement->component->fight);
	Movement_EndWaitRoom(movement);

	RoomInfo *roomInfo = Movement_SearchRoomInfo(id);
	if (roomInfo == NULL) {
		static NetProto_Error error;
		error.set_content(Config_Words(42));
		int32_t me = PlayerEntity_ID(movement->component->player);
		GCAgent_SendProtoToClients(&me, 1, &error);
		return;
	}
	if (roomInfo->count() >= Config_MaxTeamPlayers(roomInfo->map()))
		return;
	if (!Movement_CanEnterRoom(movement, roomInfo->map(), true, noPower || roomInfo->noPower()))
		return;

	if(roomInfo->map() >= Config_RoomBegin() && roomInfo->map() <= Config_RoomEnd())
	{
		int re = Event_DayEnterRoom(movement->component, roomInfo->map());
		if(re < 0)
			return;
	}

	int32_t playerID = PlayerEntity_ID(movement->component->player);
	for (int i = 0; i < roomInfo->roles_size(); i++) {
		if (roomInfo->roles(i) == playerID)
			return;
	}
	int pos = -1;
	for (int i = 0; i < roomInfo->roles_size(); i++) {
		if (roomInfo->roles(i) == -1) {
			roomInfo->set_roles(i, playerID);
			pos = i;
			break;
		}
	}
	if (pos == -1) {
		if (roomInfo->roles_size() >= Config_MaxTeamPlayers(roomInfo->map()))
			return;
		pos = roomInfo->roles_size();
		roomInfo->add_roles(playerID);
	}
	roomInfo->set_count(roomInfo->count() + 1);

	movement->multiRoom = id;

	static NetProto_JoinRoom jr;
	jr.Clear();
	jr.set_id(id);
	jr.set_pos(pos);
	jr.mutable_info()->set_roleID(movement->component->baseAtt->roleID());
	jr.mutable_info()->set_name(movement->component->baseAtt->name());
	jr.mutable_info()->set_professionType((PB_ProfessionInfo::Type)movement->component->baseAtt->professionType());
	jr.set_power(Fight_Power(movement->component->fight));
	jr.set_playerID(playerID);
	movement->component->roleAtt->equipmentAtt().ToPB(jr.mutable_att());
	jr.set_male(movement->component->baseAtt->male());
	jr.set_map(roomInfo->map());
	for (int j = 0, n = 0; j < jr.att().equipments_size(); j++) {
		int64_t id = jr.att().equipments(j);
		if (id < 0)
			continue;
		movement->component->playerAtt->itemPackage().equips(id).ToPB(jr.add_equips());
		jr.mutable_att()->set_equipments(j, n);
		n++;
	}

	int32_t ids[MAX_MUL_TOWER_TEAM_PLAYERS];
	int count = 0;
	for (int i = 0; i < roomInfo->roles_size(); i++) {
		int32_t target = roomInfo->roles(i);
		if (target != -1)
			ids[count++] = roomInfo->roles(i);
	}
	if (count > 0)
		GCAgent_SendProtoToClients(ids, count, &jr);

	for (int i = 0; i < roomInfo->roles_size(); i++) {
		int32_t target = roomInfo->roles(i);
		if (target != -1 && target != playerID) {
			struct PlayerEntity *player = PlayerEntity_Player(target);
			if (player == NULL)
				continue;
			struct Component *tComponent = PlayerEntity_Component(player);
			jr.Clear();
			jr.set_pos(i);
			jr.mutable_info()->set_roleID(tComponent->baseAtt->roleID());
			jr.mutable_info()->set_name(tComponent->baseAtt->name());
			jr.mutable_info()->set_professionType((PB_ProfessionInfo::Type)tComponent->baseAtt->professionType());
			jr.set_power(Fight_Power(tComponent->fight));
			jr.set_playerID(target);
			tComponent->roleAtt->equipmentAtt().ToPB(jr.mutable_att());
			jr.set_male(tComponent->baseAtt->male());
			jr.set_map(roomInfo->map());
			for (int j = 0, n = 0; j < jr.att().equipments_size(); j++) {
				int64_t id = jr.att().equipments(j);
				if (id < 0)
					continue;
				tComponent->playerAtt->itemPackage().equips(id).ToPB(jr.add_equips());
				jr.mutable_att()->set_equipments(j, n);
				n++;
			}
			GCAgent_SendProtoToClients(&playerID, 1, &jr);
		}
	}
}

void Movement_LeaveRoom(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;
	if (movement->multiRoom == -1)
		return;

	int32_t room = movement->multiRoom;
	RoomInfo *roomInfo = Movement_SearchRoomInfo(room);
	if (roomInfo == NULL) {
		movement->multiRoom = -1;
		return;
	}

	int32_t id = PlayerEntity_ID(movement->component->player);
	for (int i = 1; i < roomInfo->roles_size(); i++) {
		if (roomInfo->roles(i) == id) {
			static NetProto_LeaveRoom lr;
			lr.set_pos(i);
			int32_t ids[MAX_MUL_TOWER_TEAM_PLAYERS];
			int count = 0;
			for (int j = 0; j < roomInfo->roles_size(); j++) {
				int32_t target = roomInfo->roles(j);
				if (target != -1)
					ids[count++] = target;
			}
			GCAgent_SendProtoToClients(ids, count, &lr);

			roomInfo->set_roles(i, -1);
			roomInfo->set_count(roomInfo->count() - 1);
			assert(roomInfo->count() >= 1);

			movement->multiRoom = -1;
			break;
		}
	}
}

static void DelRoomInfo(int32_t id) {
	for (list<RoomInfo>::iterator it = package.multiRoom.begin(); it != package.multiRoom.end(); it++)
		if (it->id() == id) {
			package.multiRoom.erase(it);
			return;
		}
}

void Movement_DestroyRoom(struct Movement *movement, bool notice) {
	if (!Movement_IsValid(movement))
		return;
	if (movement->multiRoom == -1)
		return;

	int32_t room = movement->multiRoom;
	RoomInfo *roomInfo = Movement_SearchRoomInfo(room);
	if (roomInfo == NULL) {
		movement->multiRoom = -1;
		return;
	}

	int32_t id = PlayerEntity_ID(movement->component->player);
	if (roomInfo->roles(0) == id) {
		int32_t ids[MAX_MUL_TOWER_TEAM_PLAYERS];
		int count = 0;
		ids[count++] = id;

		for (int i = 1; i < roomInfo->roles_size(); i++) {
			int32_t target = roomInfo->roles(i);
			if (target != -1) {
				struct PlayerEntity *player = PlayerEntity_Player(target);
				if (player == NULL)
					continue;
				struct Component *component = PlayerEntity_Component(player);
				component->movement->multiRoom = -1;
				ids[count++] = target;
			}
		}

		if (notice) {
			static NetProto_DestroyRoom dr;
			GCAgent_SendProtoToClients(ids, count, &dr);
		}

		DelRoomInfo(room);
		movement->multiRoom = -1;
	}
}

void Movement_EvictRole(struct Movement *movement, int pos) {
	if (!Movement_IsValid(movement))
		return;
	if (movement->multiRoom == -1)
		return;

	RoomInfo *roomInfo = Movement_SearchRoomInfo(movement->multiRoom);
	if (roomInfo == NULL)
		return;

	int32_t id = PlayerEntity_ID(movement->component->player);
	if (roomInfo->roles(0) == id) {
		if (pos < 1 || pos >= roomInfo->roles_size())
			return;
		int32_t target = roomInfo->roles(pos);
		if (target == -1)
			return;

		struct PlayerEntity *player = PlayerEntity_Player(target);
		if (player == NULL)
			return;
		struct Component *component = PlayerEntity_Component(player);

		int32_t ids[MAX_MUL_TOWER_TEAM_PLAYERS];
		int count = 0;
		for (int i = 0; i < roomInfo->roles_size(); i++) {
			if (roomInfo->roles(i) != -1)
				ids[count++] = roomInfo->roles(i);
		}

		component->movement->multiRoom = -1;
		roomInfo->set_roles(pos, -1);
		roomInfo->set_count(roomInfo->count() - 1);
		assert(roomInfo->count() >= 1);

		static NetProto_EvictRole er;
		er.set_pos(pos);
		GCAgent_SendProtoToClients(ids, count, &er);
	}
}

void Movement_RoomList(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;

	static NetProto_RoomList rl;
	rl.Clear();
	for (list<RoomInfo>::iterator it = package.multiRoom.begin(); it != package.multiRoom.end(); it++)
		*rl.add_rooms() = *it;
	int32_t id = PlayerEntity_ID(movement->component->player);
	GCAgent_SendProtoToClients(&id, 1, &rl);
}

void Movement_BeginMultiRoom(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;
	if (movement->multiRoom == -1)
		return;

	RoomInfo *roomInfo = Movement_SearchRoomInfo(movement->multiRoom);
	if (roomInfo == NULL)
		return;

	// for debug
	/*
	if (roomInfo->count() <= 1) {
		static Vector2i nextCoord;
		const MapInfo *info = MapInfoManager_MapInfo(roomInfo->map());
		assert(info != NULL);
		if (!MapInfoManager_EnterCoord(info, 0, &nextCoord)) {
			DEBUG_LOGERROR("Failed to get enter coord");
			return;
		}
		Movement_BeginChangeScene(movement, roomInfo->map(), &nextCoord, -1, roomInfo->noPower());
	} else */
	{
		const MapInfo *info = MapInfoManager_MapInfo(roomInfo->map());
		assert(info != NULL);

		static Vector2i nextCoord;
		if (!MapInfoManager_EnterCoord(info, 0, &nextCoord)) {
			DEBUG_LOGERROR("Failed to get enter coord");
			return;
		}
		int32_t room = MapPool_Gen(roomInfo->map(), true);
		if (room == -1) {
			DEBUG_LOGERROR("Failed to gen room");
			return;
		}
		for (int i = roomInfo->roles_size() - 1; i >= 0; i--) {
			struct PlayerEntity *player = PlayerEntity_Player(roomInfo->roles(i));
			if (player == NULL)
				continue;
			struct Component *component = PlayerEntity_Component(player);
			Movement_BeginChangeScene(component->movement, room, &nextCoord, -1, roomInfo->noPower());
		}

		static char buf[CONFIG_FIXEDARRAY];
		char *index = buf;

		if (Config_ScribeHost() != NULL) {
			memset(buf, 0, sizeof(buf) / sizeof(char));
			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(movement->component->player));
			index += strlen(index);
			SNPRINTF2(buf, index, "{\"mapID\":\"%d", info->id());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"count\":\"%d\"}", roomInfo->count());
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(movement->component->player)), PlayerEntity_ID(movement->component->player), "MulMap", buf));
		}
	}
}

int32_t Movement_SerialNum(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return -1;

	return movement->serialNum;
}

void Movement_AddSerialNum(struct Movement *movement, int32_t delta) {
	if (!Movement_IsValid(movement))
		return;

	movement->serialNum += delta;
	if (movement->serialNum < 0)
		movement->serialNum = 100;
}

static inline void GenFunc(struct Movement *movement, const Vector2i *p1, const Vector2i *p2) {
	float x1 = MapInfoManager_RealByLogic(p1->x());
	float y1 = MapInfoManager_RealByLogic(p1->y());
	float x2 = MapInfoManager_RealByLogic(p2->x());
	float y2 = MapInfoManager_RealByLogic(p2->y());

	if (x1 == x2) {
		movement->isConstX = true;
		movement->constX = x1;
		movement->a = 2.0f;
	}
	else if (y1 == y2) {
		movement->isConstY = true;
		movement->constY = y1;
		movement->a = 0.0f;
	}
	else {
		movement->isConstX = movement->isConstY = false;
		movement->a = (y1 - y2) / (x1 - x2);
		movement->b = y1 - movement->a * x1;
	}
}

static inline float CalculateY(struct Movement *movement, float x) {
	if (movement->isConstY)
		return movement->constY;

	return movement->a * x + movement->b;
}

static inline float CalculateX(struct Movement *movement, float y) {
	if (movement->isConstX)
		return movement->constX;

	return (y - movement->b) / movement->a;
}

static bool GenLine(struct Movement *movement, const Vector2i *end) {
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
	if (mapInfo == NULL)
		return false;
	if (MapInfoManager_Unwalkable(mapInfo, &movement->att->logicCoord(), movement->att->mapID()))
		return false;
	if (MapInfoManager_Unwalkable(mapInfo, end, movement->att->mapID()))
		return false;

	Vector2i *begin = movement->att->mutable_logicCoord();
	GenFunc(movement, begin, end);

	if (abs(movement->a) < 1.0f) {
		float x = MapInfoManager_RealByLogic(begin->x() < end->x() ? begin->x() : end->x());
		float o = MapInfoManager_RealByLogic(begin->x() > end->x() ? begin->x() : end->x());
		static Vector2i node;

		for (x += 0.25f; x < o; x += 0.5f) {
			float y = CalculateY(movement, x);

			node.set_x(MapInfoManager_LogicByReal(x));
			node.set_y(MapInfoManager_LogicByReal(y));
			if (MapInfoManager_Unwalkable(mapInfo, &node, movement->att->mapID()))
				return false;

			node.set_x(node.x() - 1);
			if (MapInfoManager_Unwalkable(mapInfo, &node, movement->att->mapID()))
				return false;
		}
	}
	else {
		float y = MapInfoManager_RealByLogic(begin->y() < end->y() ? begin->y() : end->y());
		float o = MapInfoManager_RealByLogic(begin->y() > end->y() ? begin->y() : end->y());
		static Vector2i node;

		for (y += 0.25f; y < o; y += 0.5f) {
			float x = CalculateX(movement, y);

			node.set_x(MapInfoManager_LogicByReal(x));
			node.set_y(MapInfoManager_LogicByReal(y));
			if (MapInfoManager_Unwalkable(mapInfo, &node, movement->att->mapID()))
				return false;

			node.set_y(node.y() - 1);
			if (MapInfoManager_Unwalkable(mapInfo, &node, movement->att->mapID()))
				return false;
		}
	}

	return true;
}

static inline void ComputeMoveDir(struct Movement *movement) {
	movement->moveDir.set_x(movement->realPath[movement->curDest].x() - movement->att->position().x());
	movement->moveDir.set_y(0.0f);
	movement->moveDir.set_z(movement->realPath[movement->curDest].z() - movement->att->position().z());
	Vector3f_Normalize(&movement->moveDir);
}

static void ToRealPath(struct Movement *movement) {
	for (int i = 0; i < movement->total; i++)
		MapInfoManager_PositionByLogicCoord(&movement->path[i], &movement->realPath[i]);
}

void Movement_Stop(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;

	movement->curDest = movement->total;
	movement->att->set_status(MovementAtt::IDLE);
}

static inline void UpdateMapContainer(struct Movement *movement) {
	if (movement->prevCoord.x() != movement->att->logicCoord().x() || movement->prevCoord.y() != movement->att->logicCoord().y()) {
		MapPool_UpdateCoord(&movement->prevCoord, &movement->att->logicCoord(), movement);
		movement->prevCoord = movement->att->logicCoord();
	}
}

int Movement_SetPosition(struct Movement *movement, const Vector3f *position) {
	if (!Movement_IsValid(movement) || position == NULL)
		return -1;
	if (Fight_Att(movement->component->fight)->status() == FightAtt::DEAD)
		return -1;

	Movement_Stop(movement);

	static Vector2i coord;
	MapInfoManager_LogicCoordByPosition(position, &coord);

	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
	if (mapInfo == NULL)
		return -1;
	if (MapInfoManager_Unwalkable(mapInfo, &coord, movement->att->mapID()))
		return -1;

	movement->att->mutable_position()->set_x(position->x());
	movement->att->mutable_position()->set_z(position->z());
	*movement->att->mutable_logicCoord() = coord;
	UpdateMapContainer(movement);

	return 0;
}

static void SendSyncAtt(struct Movement *movement) {
	static const char *key[] = {"F6E4E1FA-1DE6-EAFD-8535-FA09A073743F",
		"80C35F74-B37A-2248-3088-53C62C6C7795",
		"E20C1C5F-0893-80AD-924C-D26099D24C3D",
		"5108FE26-06D9-CB19-6C99-418A3102F14D",
		"1BB97A7E-E533-DF31-F3E4-E213D36F9EED",
		"312322F7-8E0A-0BEA-70BA-A610BC492967",
		"955606A4-C407-FAEC-27A8-2C6C08E649F1",
		"DBE2AC58-92BA-D809-2B2B-AB710EEDBB12",
		"93C9AB85-8B83-BEF9-961A-AFF80B25AF6B",
		"D26BD519-896F-B103-4FF8-EF0DF3B812CD"};
	static int count = (int)(sizeof(key) / sizeof(key[0]));
	static NetProto_SyncAtt syncAtt;
	syncAtt.Clear();
	const FightAtt *att = Fight_Att(movement->component->fight);
	for (int i = 0; i < att->properties_size(); i++)
		syncAtt.add_properties(att->properties(i));
	for (int i = 0; i < att->propertiesDelta_size(); i++)
		att->propertiesDelta(i).ToPB(syncAtt.add_propertiesDelta());
	syncAtt.set_index(Time_Random(0, count));
	syncAtt.set_time(Time_ElapsedTime());
	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, "%d%d%lld%s", syncAtt.properties(3), syncAtt.propertiesDelta(1).delta(), (long long)syncAtt.time(), key[syncAtt.index()]);
	syncAtt.set_md5(md5(buf));
	int32_t id = PlayerEntity_ID(movement->component->player);
	GCAgent_SendProtoToClients(&id, 1, &syncAtt);
}

void Movement_BeginChangeScene(struct Movement *movement, int32_t nextMap, const Vector2i *nextCoord, int curJump, bool noPower) {
	if (!Movement_IsValid(movement))
		return;

	// Only for player.
	assert(movement->component->player != NULL);

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(nextMap));
	if (info == NULL)
		return;

	const MapInfo *curInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));

	if (info->id() >= Config_BossBegin() && info->id() <= Config_BossEnd()) {
		if (movement->component->playerAtt->fixedEvent(4) < info->id() - Config_BossBegin())
			return;
		
		int count = movement->component->playerAtt->dayEvent(77) >> 16;
		if (count >= Config_BossCount()) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			
			count -= 5;

			if (count >= vip->hero()) {
				return;
			}
			
			int heroRmb = 10 + 10 * count;
			
			if (!Item_HasRMB(movement->component->item, heroRmb))
				return;
			Item_ModifyRMB(movement->component->item, -heroRmb, false, 38, 0, 0);

		}

		/*
		int events[] = {31, 42, 43};
		int index = (info->id() - Config_BossBegin()) / 15;
		if (index < (int)(sizeof(events) / sizeof(events[0]))) {
			index = events[index];
			int bit = ((info->id() - Config_BossBegin()) % 15) * 2;
			int count = (movement->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
			if (count >= Config_BossCount())
				return;
		}*/
	} else if (info->id() == Config_SurviveMap()) {
		int count = (movement->component->playerAtt->dayEvent(30) & 0xf00) >> 8;
		if (count >= Config_SurviveCount())
			return;
	} else if (info->id() >= Config_SingleEnhanceBegin() && info->id() <= Config_SingleEnhanceEnd()) {
		int events[] = {32, 33, 36, 37, 38, 39, 40, 41};
		int index = (info->id() - Config_SingleEnhanceBegin()) / 15;
		if (index < (int)(sizeof(events) / sizeof(events[0]))) {
			index = events[index];
			int bit = ((info->id() - Config_SingleEnhanceBegin()) % 15) * 2;
			int count = (movement->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
			if (count >= Config_SingleEnhanceCount())
				return;
		}
	} else if (info->id() == Config_GodMap()) {
		int event = movement->component->playerAtt->dayEvent(29);
		int count = event & 0xffff;
		time_t OpenTime = (time_t)Config_OpenTime(NULL);
		int res = Time_DayDelta(OpenTime, Time_TimeStamp());
		if (res != 0) {
			if (count >= Config_MaxGodCount())
				return;
		}
	} else if (info->id() == Config_EventMap1_Easy()
			|| info->id() == Config_EventMap1_Normal()
			|| info->id() == Config_EventMap1_Hard()) {
		int count = (movement->component->playerAtt->dayEvent(28) & 0xf00) >> 8;
	//	if (count >= Config_MaxEvent1Count())
	//		return;
		if (count == 0) {
		}else if (count == 1) {
			if (!Item_HasRMB(movement->component->item, 10))
				return;
			Item_ModifyRMB(movement->component->item, -10, false, 39, 0, 0);
		}else if (count < 15) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			if (count >= vip->fubenCount() + 2) {
				return;
			}
			
			int rmb = (count + 1) / 2 * 10;
			if (!Item_HasRMB(movement->component->item, rmb))
				return;
			Item_ModifyRMB(movement->component->item, -rmb, false, 39, 0, 0);
		}else {
			return;
		}
	} else if (info->id() == Config_EventMap_ProtectCrystal_Easy()
			|| info->id() == Config_EventMap_ProtectCrystal_Normal()
			|| info->id() == Config_EventMap_ProtectCrystal_Hard()) {
		int count = (movement->component->playerAtt->dayEvent(28) & 0xf000) >> 12;
		//if (count >= Config_MaxCount_EventMap_ProtectCrystal())
		//	return;
		if (count == 0) {
		}else if (count == 1) {
			if (!Item_HasRMB(movement->component->item, 10))
				return;
			Item_ModifyRMB(movement->component->item, -10, false, 39, 0, 0);
		}else if (count < 15) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			if (count >= vip->fubenCount() + 2) {
				return;
			}
			int rmb = (count + 1) / 2 * 10;
			if (!Item_HasRMB(movement->component->item, rmb))
				return;
			Item_ModifyRMB(movement->component->item, -rmb, false, 39, 0, 0);
		}else {
			return;
		}
	} else if (info->id() == Config_EventMap_GoldPig_Easy()
			|| info->id() == Config_EventMap_GoldPig_Normal()
			|| info->id() == Config_EventMap_GoldPig_Hard()) {
		int count = (movement->component->playerAtt->dayEvent(28) & 0xf0000) >> 16;
		//if (count >= Config_MaxCount_EventMap_GoldPig())
		//	return;
		if (count == 0) {
		}else if (count == 1) {
			if (!Item_HasRMB(movement->component->item, 10))
				return;
			Item_ModifyRMB(movement->component->item, -10, false, 39, 0, 0);
		}else if (count < 15) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			if (count >= vip->fubenCount() + 2) {
				return;
			}
			int rmb = (count + 1) / 2 * 10;
			if (!Item_HasRMB(movement->component->item, rmb))
				return;
			Item_ModifyRMB(movement->component->item, -rmb, false, 39, 0, 0);
		}else {
			return;
		}

	} else if (info->id() == Config_EventMap_PetBattle_Easy()
			|| info->id() == Config_EventMap_PetBattle_Normal()
			|| info->id() == Config_EventMap_PetBattle_Hard()) {
		int count = (movement->component->playerAtt->dayEvent(28) & 0xf00000) >> 20;
		//if (count >= Config_MaxCount_EventMap_PetBattle())
		//	return;
		if (count == 0) {
		}else if (count == 1) {
			if (!Item_HasRMB(movement->component->item, 10))
				return;
			Item_ModifyRMB(movement->component->item, -10, false, 39, 0, 0);
		}else if (count < 15) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			if (count >= vip->fubenCount() + 2) {
				return;
			}
			int rmb = (count + 1) / 2 * 10;
			if (!Item_HasRMB(movement->component->item, rmb))
				return;
			Item_ModifyRMB(movement->component->item, -rmb, false, 39, 0, 0);
		}else {
			return;
		}
	} else if (info->id() == Config_EventMap_ObstacleFan_Easy()
			|| info->id() == Config_EventMap_ObstacleFan_Normal()
			|| info->id() == Config_EventMap_ObstacleFan_Hard()) {
		int count = movement->component->playerAtt->dayEvent(76) & 0xf;
		//if (count >= Config_MaxCount_EventMap_ObstacleFan())
		//	return;
		if (count == 0) {
		}else if (count == 1) {
			if (!Item_HasRMB(movement->component->item, 10))
				return;
			Item_ModifyRMB(movement->component->item, -10, false, 39, 0, 0);
		}else if (count < 15) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			if (count >= vip->fubenCount() + 2) {
				return;
			}
			int rmb = (count + 1) / 2 * 10;
			if (!Item_HasRMB(movement->component->item, rmb))
				return;
			Item_ModifyRMB(movement->component->item, -rmb, false, 39, 0, 0);
		}else {
			return;
		}

	} else if (info->id() == Config_EventMap_PetStone_Easy()
			|| info->id() == Config_EventMap_PetStone_Normal()
			|| info->id() == Config_EventMap_PetStone_Hard()) {
		int count = (movement->component->playerAtt->dayEvent(76) & 0xf0) >> 4;
		//if (count >= Config_MaxCount_EventMap_PetStone())
		//	return;
		if (count == 0) {
		}else if (count == 1) {
			if (!Item_HasRMB(movement->component->item, 10))
				return;
			Item_ModifyRMB(movement->component->item, -10, false, 39, 0, 0);
		}else if (count < 15) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			if (count >= vip->fubenCount() + 2) {
				return;
			}
			int rmb = (count + 1) / 2 * 10;
			if (!Item_HasRMB(movement->component->item, rmb))
				return;
			Item_ModifyRMB(movement->component->item, -rmb, false, 39, 0, 0);
		}else {
			return;
		}
	} else if (info->id() == Config_EventMap_RobRes_Easy()
			|| info->id() == Config_EventMap_RobRes_Normal()
			|| info->id() == Config_EventMap_RobRes_Hard()) {
		int count = (movement->component->playerAtt->dayEvent(76) & 0xf00) >> 8;
		//if (count >= Config_MaxCount_EventMap_RobRes())
		//	return;
		if (count == 0) {
		}else if (count == 1) {
			if (!Item_HasRMB(movement->component->item, 10))
				return;
			Item_ModifyRMB(movement->component->item, -10, false, 39, 0, 0);
		}else if (count < 15) {
			const VIPInfo *vip = VIPInfoManager_VIPInfo(movement->component->playerAtt->itemPackage().vip());
			if (vip == NULL)
				return;
			if (count >= vip->fubenCount() + 2) {
				return;
			}
			int rmb = (count + 1) / 2 * 10;
			if (!Item_HasRMB(movement->component->item, rmb))
				return;
			Item_ModifyRMB(movement->component->item, -rmb, false, 39, 0, 0);
		}else {
			return;
		}
	} else if (Config_MulTowerGroup(info->id()) != -1) {
		int groupID = Config_MulTowerGroup(info->id());
		int event = movement->component->playerAtt->dayEvent(48);
		int count = (event & (3 << (groupID * 2))) >> (groupID * 2);
		if (count >= Config_MaxMulTowerCount())
			return;
	}

	if (info->id() >= Config_BossBegin() && info->id() <= Config_BossEnd()) {
		if (Config_ScribeHost() != NULL) {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;

			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(movement->component->player));
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"mapID\":\"%d", info->id());
			index += strlen(index);
			SNPRINTF2(buf, index, "\"}");
			int32_t player = PlayerEntity_ID(movement->component->player);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "Boss", buf));
		}
	} else if (info->id() == Config_SurviveMap()) {
		if (Config_ScribeHost() != NULL) {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;

			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(movement->component->player));
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"mapID\":\"%d", info->id());
			index += strlen(index);
			SNPRINTF2(buf, index, "\"}");
			int32_t player = PlayerEntity_ID(movement->component->player);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "Survive", buf));
		}
	} else if (info->id() >= Config_TowerBegin() && info->id() <= Config_TowerEnd()) {
		if (Config_ScribeHost() != NULL) {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;

			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(movement->component->player));
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"mapID\":\"%d", info->id());
			index += strlen(index);
			SNPRINTF2(buf, index, "\"}");
			int32_t player = PlayerEntity_ID(movement->component->player);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "Tower", buf));
		}
	}

	if (!Movement_CanEnterRoom(movement, nextMap, false, noPower))
		return;

	Item_ModifyDurability(movement->component->item, -info->durability());

	{
		int kill = (movement->component->playerAtt->fixedEvent(6) & 0xffff0000) >> 16;
		int time = movement->component->playerAtt->fixedEvent(7) & 0xffff;
		if (kill > 0 || time > 0) {
			int v = Config_FactionWarAward(kill, time);
			if (v > 0) {
				FactionPool_Donate(movement->component->playerAtt, 0, v);
			}
			time = time % 60;
			PlayerEntity_SetFixedEvent(movement->component->player, 6, 0, true);
			PlayerEntity_SetFixedEvent(movement->component->player, 7, (movement->component->playerAtt->fixedEvent(7) & 0xffff0000) | time, true);
		}
	}

	if (info->mapType() == MapInfo::PEACE) {
		const PlayerAtt *profession = ProfessionInfoManager_ProfessionInfo(movement->component->baseAtt->professionType(), movement->component->baseAtt->male());
		assert(profession != NULL);
		Fight_SetFaction(movement->component->fight, profession->att().fightAtt().selfFaction(), profession->att().fightAtt().friendlyFaction());
	}

	if (curInfo != NULL) {
		if (curInfo->id() == Config_GodMap())
			Fight_SetGodTarget(movement->component->fight, -1, 0, 0);

		if (curInfo->id() == Config_SurviveMap()) {
			Mission_CompleteTarget(movement->component->mission, MissionTarget::CLEAR_SURVIVE, -1, -1);
		} else if (curInfo->mapType() == MapInfo::PVP) {
			Mission_CompleteTarget(movement->component->mission, MissionTarget::PVP, -1, -1);
		}

		if (curInfo->mapType() != MapInfo::PEACE) {
			struct NPCEntity *pet = NPCPool_Pet(movement->att->mapID(), movement->component);
			if (NPCEntity_IsValid(pet)) {
				MapPool_ReleaseNPCs(movement->att->mapID(), NPCEntity_ID(pet), true);
			}
		}
	}

	if (info->mapType() != MapInfo::PEACE)
		Mission_CompleteTarget(movement->component->mission, MissionTarget::ENTER_ANY_ROOM, info->id(), 0);

	movement->endLoadModel = false;
	Movement_EndWaitRoom(movement);
	bool notice = true;
	if (info != NULL) {
		if (info->mapType() == MapInfo::ONLY_ROOM
				|| (info->mapType() == MapInfo::ROOM && !MapPool_IsSingle(nextMap)))
			notice = false;
	}
	Movement_DestroyRoom(movement, notice);
	Movement_LeaveRoom(movement);
	Fight_Idle(movement->component->fight);
	Fight_EndWaitPVP(movement->component->fight);
	Fight_EndWaitHell(movement->component->fight);

	Fight_Transform(movement->component->fight, false);
	Fight_ModifyHP(movement->component->fight, NULL, Fight_FinalProperty(movement->component->fight, FightAtt::MAXHP));
	Fight_ModifyMana(movement->component->fight, NULL, Config_MaxMana());
	Fight_ModifyEnergy(movement->component->fight, NULL, -Config_MaxEnergy());

	int32_t id = PlayerEntity_ID(movement->component->player);

	if (info->mapType() == MapInfo::PEACE) {
		movement->att->set_prevNormalMap(nextMap);
		*movement->att->mutable_prevCoord() = *nextCoord;

		if (curInfo != NULL && Config_ScribeHost() != NULL) {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;

			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(movement->component->player));
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"mapID\":\"%d", curInfo->id());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"endTime\":\"%d", (int)Time_TimeStamp());
			index += strlen(index);
			if (curInfo->id() == Config_WorldBossMap()) {
				SNPRINTF2(buf, index, "\",\"mapType\":\"WorldBoss");
				index += strlen(index);
			} else if (curInfo->id() >= Config_TowerBegin() && curInfo->id() <= Config_TowerEnd()) {
				SNPRINTF2(buf, index, "\",\"mapType\":\"Tower");
				index += strlen(index);
			} else if (curInfo->id() >= Config_BossBegin() && curInfo->id() <= Config_BossEnd()) {
				SNPRINTF2(buf, index, "\",\"mapType\":\"Hero");
				index += strlen(index);
			} else if (curInfo->id() == Config_SurviveMap()) {
				SNPRINTF2(buf, index, "\",\"mapType\":\"Survive");
				index += strlen(index);
			}		
			const FightAtt *fightAtt = Fight_Att(movement->component->fight);
			if (fightAtt != NULL) {
				SNPRINTF2(buf, index, "\",\"exp\":\"%lld", (long long int)fightAtt->exp());
				index += strlen(index);
			}
			const ItemPackage *package = Item_Package(movement->component->item);
			if (package != NULL) {
				SNPRINTF2(buf, index, "\",\"money\":\"%lld", (long long int)package->money());
				index += strlen(index);
			}
			Fight * fight = movement->component->fight;
			if (fight != NULL) {
				SNPRINTF2(buf, index, "\",\"ATK\":\"%d", Fight_FinalProperty(fight, FightAtt::ATK));
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"DEF\":\"%d", Fight_FinalProperty(fight, FightAtt::DEF));
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"MAXHP\":\"%d", Fight_FinalProperty(fight, FightAtt::MAXHP));
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"CRIT\":\"%d", Fight_FinalProperty(fight, FightAtt::CRIT));
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"ACCURACY\":\"%d", Fight_FinalProperty(fight, FightAtt::ACCURACY));
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"DODGE\":\"%d\"}", Fight_FinalProperty(fight, FightAtt::DODGE));
				ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "EndHurdle", buf));
			}
		}
	}
	else if (info->mapType() == MapInfo::ROOM
			|| info->mapType() == MapInfo::SINGLE
			|| info->mapType() == MapInfo::ONLY_ROOM
			|| info->mapType() == MapInfo::PRACTICE
			|| info->mapType() == MapInfo::PVP
			|| info->mapType() == MapInfo::HELL) {
		if (curInfo != NULL) {
			if (curInfo->mapType() == MapInfo::PEACE) {
				movement->att->set_prevNormalMap(movement->att->mapID());
				if (curJump == -1) {
					*movement->att->mutable_prevCoord() = movement->att->logicCoord();
				} else {
					if (!MapInfoManager_EnterCoord(curInfo, curJump, movement->att->mutable_prevCoord()))
						assert(0);
				}
			}

			if (Config_ScribeHost() != NULL) {
				static char buf[CONFIG_FIXEDARRAY];
				memset(buf, 0, sizeof(buf) / sizeof(char));
				char *index = buf;

				SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(movement->component->player));
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"mapID\":\"%d", info->id());
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"beginTime\":\"%d", (int)Time_TimeStamp());
				index += strlen(index);
				if (info->id() == Config_WorldBossMap()) {
					SNPRINTF2(buf, index, "\",\"mapType\":\"WorldBoss");
					index += strlen(index);
				} else if (info->id() >= Config_TowerBegin() && info->id() <= Config_TowerEnd()) {
					SNPRINTF2(buf, index, "\",\"mapType\":\"Tower");
					index += strlen(index);
				} else if (info->id() >= Config_BossBegin() && info->id() <= Config_BossEnd()) {
					SNPRINTF2(buf, index, "\",\"mapType\":\"Hero");
					index += strlen(index);
				} else if (info->id() == Config_SurviveMap()) {
					SNPRINTF2(buf, index, "\",\"mapType\":\"Survive");
					index += strlen(index);
				}		
				const FightAtt *fightAtt = Fight_Att(movement->component->fight);
				if (fightAtt != NULL) {
					SNPRINTF2(buf, index, "\",\"exp\":\"%lld", (long long int)fightAtt->exp());
					index += strlen(index);
				}
				const ItemPackage *package = Item_Package(movement->component->item);
				if (package != NULL) {
					SNPRINTF2(buf, index, "\",\"money\":\"%lld", (long long int)package->money());
					index += strlen(index);
				}
				Fight * fight = movement->component->fight;
				if (fight != NULL) {
					SNPRINTF2(buf, index, "\",\"ATK\":\"%d", Fight_FinalProperty(fight, FightAtt::ATK));
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"DEF\":\"%d", Fight_FinalProperty(fight, FightAtt::DEF));
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"MAXHP\":\"%d", Fight_FinalProperty(fight, FightAtt::MAXHP));
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"CRIT\":\"%d", Fight_FinalProperty(fight, FightAtt::CRIT));
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"ACCURACY\":\"%d", Fight_FinalProperty(fight, FightAtt::ACCURACY));
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"DODGE\":\"%d\"}", Fight_FinalProperty(fight, FightAtt::DODGE));
					ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "BeginHurdle", buf));
				}
			}
		}
	}
	else {
		assert(0);
	}

	if (info->mapType() == MapInfo::ROOM
			|| info->mapType() == MapInfo::SINGLE
			|| info->mapType() == MapInfo::ONLY_ROOM) {
		Fight_ModifyEnergy(movement->component->fight, NULL, Config_MaxEnergy());
	}

	if (info->id() >= Config_BossBegin() && info->id() <= Config_BossEnd()) {
		/*		int events[] = {31, 42, 43};
				int index = (info->id() - Config_BossBegin()) / 15;
				if (index < (int)(sizeof(events) / sizeof(events[0]))) {
				index = events[index];
				int bit = ((info->id() - Config_BossBegin()) % 15) * 2;
				int count = (movement->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
				PlayerEntity_SetDayEvent(movement->component->player, index, (movement->component->playerAtt->dayEvent(index) & (~(3 << bit))) | ((count + 1) << bit));
				}
		 */
	} else if (info->id() == Config_SurviveMap()) {
		/*	int count = (movement->component->playerAtt->dayEvent(30) & 0xf00) >> 8;
			PlayerEntity_SetDayEvent(movement->component->player, 30, (movement->component->playerAtt->dayEvent(30) & (~(0xf00))) | ((count + 1) << 8));
		 */
	} else if (info->id() >= Config_SingleEnhanceBegin() && info->id() <= Config_SingleEnhanceEnd()) {
		/*
		   int events[] = {32, 33, 36, 37, 38, 39, 40, 41};
		   int index = (info->id() - Config_SingleEnhanceBegin()) / 15;
		   if (index < (int)(sizeof(events) / sizeof(events[0]))) {
		   index = events[index];
		   int bit = ((info->id() - Config_SingleEnhanceBegin()) % 15) * 2;
		   int count = (movement->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
		   PlayerEntity_SetDayEvent(movement->component->player, index, (movement->component->playerAtt->dayEvent(index) & (~(3 << bit))) | ((count + 1) << bit));
		   }
		 */
	} else if (info->id() == Config_GodMap()) {
		int event = movement->component->playerAtt->dayEvent(29);
		int count = event & 0xffff;
		time_t OpenTime = (time_t)Config_OpenTime(NULL);
		int res = Time_DayDelta(OpenTime, Time_TimeStamp());
		if (res != 0)
			PlayerEntity_SetDayEvent(movement->component->player, 29, (count + 1) | (event & 0xffff0000));
	}

	// if (movement->att->mapID() == nextMap)
	//	return;

	Fight_Cancel(movement->component->fight);
	Fight_ClearEnemy(movement->component->fight);
	Fight_ClearSkills(movement->component->fight);
	Status_Clear(movement->component->status);
	AI_Idle(movement->component->ai);
	Fight_SetCantBeAttacked(movement->component->fight, false);
	Fight_SetCantAttack(movement->component->fight, false);

	Fight_EnterScene(movement->component->fight, info);

	movement->nextMap = nextMap;

	if (info->mapType() == MapInfo::SINGLE || (info->mapType() == MapInfo::ROOM && MapPool_IsSingle(nextMap)))
		SendSyncAtt(movement);

	// playofftest
	/*
	   if (info->mapType() == MapInfo::PEACE)
	   SendSyncAtt(movement);
	 */

	static NetProto_DoLoadScene doLoadScene;
	doLoadScene.Clear();
	doLoadScene.set_id(id);
	doLoadScene.set_mapID(MapPool_MapInfo(nextMap));
	nextCoord->ToPB(doLoadScene.mutable_coord());
	if (info->mapType() == MapInfo::ROOM)
		doLoadScene.set_multipleRoom(!MapPool_IsSingle(nextMap));
	GCAgent_SendProtoToAllClientsInSameScene(movement->att->mapID(), movement->component->player, &doLoadScene);

	PlayerPool_Del(movement->component->player);
	MapPool_UpdateCoord(&movement->att->logicCoord(), NULL, movement);
	MapPool_Release(movement->att->mapID());

	movement->att->set_mapID(MAP_CHANGING);
	*movement->att->mutable_logicCoord() = *nextCoord;
	movement->prevCoord = *nextCoord;
	MapInfoManager_PositionByLogicCoord(nextCoord, movement->att->mutable_position());
	PlayerPool_Add(movement->component->player);
	MapPool_Link(nextMap, movement);
}

static void GenPet(struct Movement *movement) {
	struct NPCEntity *pets[CONFIG_FIXEDARRAY];
	int count = NPCPool_Pets(movement->att->mapID(), pets, CONFIG_FIXEDARRAY);
	for (int i = 0; i < count; i++) {
		int id = NPCEntity_ID(pets[i]);
		struct Component *master = NPCEntity_Master(pets[i]);
		if (id == -1 || master == NULL)
			continue;

		static NetProto_GenPet proto;
		proto.Clear();
		proto.set_id(id);
		// TODO
		// if master is npc
		proto.set_player(PlayerEntity_ID(master->player));
		int32_t me = PlayerEntity_ID(movement->component->player);
		GCAgent_SendProtoToClients(&me, 1, &proto);
		DEBUG_LOG("sync pet, id: %d", id);
	}

	int fightingPet = movement->component->roleAtt->fightAtt().fightingPet();
	if (fightingPet == -1) {
		return;
	}

	NPCAtt att;
	if (!Fight_ToPetData(movement->component->playerAtt, fightingPet, &att)) {
		return;
	}

	/*
	   for (int i = 0, n = 0; i < att.att().fightAtt().skills_size(); i++) {
	   if (att.att().fightAtt().skills(i).id() == -1) {
	   if (n >= pet->extraSkills_size())
	   break;
	   if (pet->extraSkills(n).id() != -1)
	 *att.mutable_att()->mutable_fightAtt()->mutable_skills(i) = pet->extraSkills(n);
	 else
	 i--;
	 n++;
	 }
	 }
	 */
	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
	const MapUnit *unit = MapInfoManager_MapUnit(info);
	int petID = unit->npcs_size();
	while (NPCPool_NPC(movement->att->mapID(), petID) != NULL) {
		petID++;
	}

	DEBUG_LOG("gen pet, id: %d", petID);
	MapPool_GenNPC(movement->att->mapID(), petID, &att, &movement->att->logicCoord(), movement->component, false);

	static NetProto_GenPet proto;
	proto.Clear();
	proto.set_id(petID);
	proto.set_player(PlayerEntity_ID(movement->component->player));
	GCAgent_SendProtoToAllClientsInSameScene(movement->att->mapID(), movement->component->player, &proto);
}

void Movement_EndChangeScene(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;

	movement->time = Time_ElapsedTime();

	// Only for player.
	assert(movement->component->player != NULL);

	// assert(movement->att->mapID() == MAP_CHANGING);
	if (movement->att->mapID() != MAP_CHANGING) {
		DEBUG_LOGERROR("Failed to EndChangeScene because player is not in MAP_CHANGING");
		return;
	}

	if (movement->nextMap < MAP_START) {
		DEBUG_LOGERROR("Failed to EndChangeScene.");
		return;
	}

	movement->serialNum = 0;

	int32_t id = PlayerEntity_ID(movement->component->player);

	PlayerPool_Del(movement->component->player);
	movement->att->set_mapID(movement->nextMap);
	PlayerPool_Add(movement->component->player);
	MapPool_UpdateCoord(NULL, &movement->att->logicCoord(), movement);
	MapPool_Unlink(movement->att->mapID(), movement);

	Fight_ClearEnemy(movement->component->fight);

	GlobalMissionManager_ApplySceneMission(movement);

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(movement->nextMap));
	bool quit = false;
	if (info == NULL) {
		quit = true;
	} else {
		if (info->id() == Config_WorldBossMap()) {
			quit = MapPool_WorldBoss() < 0;
		} else if (info->id() == Config_FactionWarMap()) {
			quit = MapPool_FactionWar() < 0;
		} else if (info->mapType() == MapInfo::HELL) {
			quit = !MapPool_IsHellValid(movement->nextMap);
		} else if (info->id() == Config_FactionBossMap()) {
			// TODO
			quit = MapPool_FactionBoss(movement->component->playerAtt->faction()) < 0;
		}
	}
	if (quit) {
		int32_t nextMap = movement->att->prevNormalMap();
		if (nextMap < MAP_START) {
			Vector2i nextCoord;
			nextMap = MapInfoManager_Next(MapPool_MapInfo(MAP_CREATE), 0, &nextCoord);
			assert(nextMap != -1);
			Movement_BeginChangeScene(movement, nextMap, &nextCoord, 0);
		}
		else {
			Movement_BeginChangeScene(movement, nextMap, &movement->att->prevCoord());
		}
		return;
	}

	// playofftest
	/*
	   if (info->id() == Config_PlayOffMap())
	   SendSyncAtt(movement);
	 */
	if (info->id() == Config_FactionWarMap()) {
		bool done = false;
		static vector<struct PlayerEntity *> players;
		players.clear();
		PlayerPool_Players(movement->att->mapID(), movement->component->player, &players);
		for (size_t i = 0; i < players.size(); i++) {
			struct PlayerEntity *entity = players[i];
			struct Component *component = PlayerEntity_Component(entity);
			if (component == NULL || component->movement == movement)
				continue;
			if (strcmp(component->playerAtt->faction(), movement->component->playerAtt->faction()) == 0) {
				int faction = component->roleAtt->fightAtt().selfFaction();
				Fight_SetFaction(movement->component->fight, faction, faction);
				done = true;
				break;
			}
		}
		if (!done) {
			int faction = PlayerPool_FreeFaction(movement->nextMap, movement->component->player);
			Fight_SetFaction(movement->component->fight, faction, faction);
		}
	} else if (/*info->id() == Config_WorldBossMap() || */
			info->mapType() == MapInfo::PVP
			|| info->mapType() == MapInfo::PRACTICE
			|| info->mapType() == MapInfo::HELL) {
		if (info->mapType() == MapInfo::HELL) {
			struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
			int npcCount = NPCPool_Monsters(movement->nextMap, npcs, CONFIG_FIXEDARRAY);
			for (int i = 0; i < npcCount; i++) {
				struct Component *component = NPCEntity_Component(npcs[i]);
				if (component != NULL) {
					if (component->baseAtt->roleID() == movement->component->baseAtt->roleID()) {
						MapPool_ReleaseNPCs(movement->nextMap, NPCEntity_ID(npcs[i]), true);
					}
				}
			}
		}

		int faction = PlayerPool_FreeFaction(movement->nextMap, movement->component->player);
		Fight_SetFaction(movement->component->fight, faction, faction);
	}

	/*
	   if (movement->component->player != NULL) {
	   int32_t playerID = PlayerEntity_ID(movement->component->player);

	   static NetProto_ModifyHP modifyHP;
	   modifyHP.Clear();
	   modifyHP.set_id(playerID);
	   modifyHP.set_hp(Fight_Att(movement->component->fight)->hp());
	   GCAgent_SendProtoToClients(&playerID, 1, &modifyHP);

	   static NetProto_ModifyMana modifyMana;
	   modifyMana.Clear();
	   modifyMana.set_id(playerID);
	   modifyMana.set_mana(Fight_Att(movement->component->fight)->mana());
	   GCAgent_SendProtoToClients(&playerID, 1, &modifyMana);
	   }
	 */

	if (!Movement_InSingle(movement)) {
		/*
		   static NetProto_ModifyFaction modifyFaction;
		   modifyFaction.Clear();
		   modifyFaction.set_type(NetProto_ModifyFaction::PLAYER);
		   modifyFaction.set_id(PlayerEntity_ID(movement->component->player));
		   modifyFaction.set_selfFaction(movement->component->roleAtt->fightAtt().selfFaction());
		   modifyFaction.set_friendlyFaction(movement->component->roleAtt->fightAtt().friendlyFaction());
		   GCAgent_SendProtoToAllClientsInSameScene(movement->att->mapID(), movement->component->player, &modifyFaction);
		 */
		static vector<struct PlayerEntity *> players;
		players.clear();
		int count = PlayerPool_Players(movement->att->mapID(), movement->component->player, &players);
		if (count < 0) {
			DEBUG_LOGERROR("Failed to get all players");
		} else if (count > 0) {
			static NetProto_AddPlayers addPlayers;
			addPlayers.Clear();

			addPlayers.add_id(id);
			PlayerEntity_ToSceneData(PlayerEntity_Att(movement->component->player), addPlayers.add_att());
			addPlayers.add_type(FactionPool_GetMemOffic(PlayerEntity_Att(movement->component->player)->faction(), PlayerEntity_Att(movement->component->player)->att().baseAtt().roleID()));
			GCAgent_SendProtoToAllClientsExceptOneInSameScene(movement->component->player, &addPlayers);

			/*
			   vector<struct PlayerEntity *> players;
			   int count = PlayerPool_Players(movement->nextMap, &players);
			   if (count > 1) {
			   DEBUG_LOGRECORD("map: %d, self: %s, send me to others, size: %d", movement->nextMap, movement->component->baseAtt->name(), count - 1);
			   }
			 */

			addPlayers.Clear();
			for (int i = 0; i < count; i++) {
				struct PlayerEntity *another = players[i];
				int aID = PlayerEntity_ID(another);
				if (aID == id)
					continue;

				addPlayers.add_id(aID);
				PlayerEntity_ToSceneData(PlayerEntity_Att(another), addPlayers.add_att());
				addPlayers.add_type(FactionPool_GetMemOffic(PlayerEntity_Att(another)->faction(), PlayerEntity_Att(another)->att().baseAtt().roleID()));
			}
			// Send offline players
			/*
			   if (info->mapType() == MapInfo::PEACE) {
			   if (addPlayers.att_size() < Config_MaxPlayersOfLineInPeace() - 1) {
			   static vector<int64_t> exists;
			   exists.clear();
			   for (int i = 0; i < count; i++) {
			   struct Component *component = PlayerEntity_Component(players[i]);
			   if (component != NULL)
			   exists.push_back(component->baseAtt->roleID());
			   }

			   int before = addPlayers.att_size();
			   PlayerPool_RandomFreeRoles(info->id(), Config_MaxPlayersOfLineInPeace() - 1 - addPlayers.att_size(), &exists, addPlayers.mutable_att());
			   int after = addPlayers.att_size();
			   DEBUG_LOG("before: %d, after: %d", before, after);
			   int newID = 0;
			   for (int i = before; i < after; i++) {
			   while (true) {
			   bool find = false;
			   for (int j = 0; j < addPlayers.id_size(); j++) {
			   if (addPlayers.id(j) == newID) {
			   find = true;
			   break;
			   }
			   }
			   if (!find) {
			   if (id == newID)
			   find = true;
			   }
			   newID++;
			   if (!find) {
			   addPlayers.add_id(newID - 1);
			   addPlayers.add_type(FactionPool_GetMemOffic(addPlayers.att(i).faction().c_str(), addPlayers.att(i).att().baseAtt().roleID()));
			   break;
			   }
			   }
			   }
			   }
			   }
			 */
			if (addPlayers.id_size() > 0) {
				GCAgent_SendProtoToClients(&id, 1, &addPlayers);
				// DEBUG_LOGRECORD("map: %d, self: %s, send others to me, count: %d", movement->nextMap, movement->component->baseAtt->name(), addPlayers.id_size());
			}
		}

		if (info->mapType() == MapInfo::ONLY_ROOM
				|| info->mapType() == MapInfo::ROOM
				|| info->mapType() == MapInfo::PVP) {
			static NetProto_AddNPCs addNPCs;
			addNPCs.Clear();

			struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
			int count = NPCPool_Monsters(movement->att->mapID(), npcs, CONFIG_FIXEDARRAY);
			if (count < 0)
				DEBUG_LOGERROR("Failed to get all npcs");
			for (int i = 0; i < count; i++) {
				if (!NPCEntity_IsValid(npcs[i]))
					continue;
				if (NPCEntity_Master(npcs[i]) != NULL)
					continue;

				addNPCs.add_id(NPCEntity_ID(npcs[i]));
				NPCEntity_ToSceneData(NPCEntity_Att(npcs[i]), addNPCs.add_att());
			}

			if (addNPCs.id_size() > 0) {
				GCAgent_SendProtoToClients(&id, 1, &addNPCs);
			}

			for (int i = 0; i < count; i++) {
				if (!NPCEntity_IsValid(npcs[i]))
					continue;
				if (NPCEntity_Master(npcs[i]) != NULL)
					continue;

				struct Component *component = NPCEntity_Component(npcs[i]);
				if (Fight_IsSpecial(component->fight)) {
					static NetProto_SetSpecial proto;
					proto.set_npc(NPCEntity_ID(npcs[i]));
					proto.set_enable(true);
					GCAgent_SendProtoToClients(&id, 1, &proto);
				}
			}
		}

		if (info->mapType() == MapInfo::ONLY_ROOM
				|| info->mapType() == MapInfo::ROOM
				|| info->mapType() == MapInfo::PRACTICE
				|| info->mapType() == MapInfo::HELL
				|| info->mapType() == MapInfo::PVP) {
			GenPet(movement);
		}

		Movement_EndLoadModel(movement);
	}
}

void Movement_EndLoadModel(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return;

	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
	if (mapInfo == NULL)
		return;

	if (mapInfo->mapType() != MapInfo::PVP && mapInfo->mapType() != MapInfo::PRACTICE)
		return;

	movement->endLoadModel = true;

	if (mapInfo->id() == Config_PlayOffMap())
		return;

	static vector<struct PlayerEntity *> players;
	players.clear();
	int count1 = PlayerPool_Players(movement->att->mapID(), movement->component->player, &players);
	if (count1 < 2) {
		int count2 = NPCPool_Count(movement->att->mapID());
		if (count2 <= 0)
			return;
	}

	bool enter = true;
	for (int i = 0; i < count1; i++) {
		struct Component *component = PlayerEntity_Component(players[i]);
		if (component == NULL)
			continue;

		if (!component->movement->endLoadModel) {
			enter = false;
			break;
		}
	}
	if (enter) {
		static NetProto_EndLoadModel elm;
		elm.Clear();
		GCAgent_SendProtoToAllClientsInSameScene(movement->att->mapID(), movement->component->player, &elm);
	}
}

bool Movement_InSingle(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return false;
	if (movement->component->player == NULL)
		return false;

	int32_t map = movement->att->mapID();
	return MapPool_IsSingle(map);
}

void Movement_SetSpeedFactor(struct Movement *movement, float factor) {
	if (!Movement_IsValid(movement))
		return;

	movement->speedFactor = factor;
}

float Movement_SpeedFactor(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return 1.0f;

	return movement->speedFactor;
}

void Movement_SetCantMove(struct Movement *movement, bool cantMove) {
	if (!Movement_IsValid(movement))
		return;

	movement->cantMove = cantMove;
}

bool Movement_CantMove(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return true;

	return movement->cantMove;
}

bool Movement_SameScene(struct Movement *a, struct Movement *b) {
	if (!Movement_IsValid(a) || !Movement_IsValid(b))
		return false;

	return a->att->mapID() == b->att->mapID();
}

void Movement_SetOriginalOffsetPos(struct Movement *movement, const Vector3f *pos) {
	if (!Movement_IsValid(movement) || pos == NULL)
		return;
	movement->offsetPos = *pos;
}

const Vector3f * Movement_OriginalOffsetPos(struct Movement *movement) {
	if (!Movement_IsValid(movement))
		return NULL;
	return &movement->offsetPos;
}

static int MoveStraightly(struct Movement *movement, const Vector2i *dest) {
	if (Fight_Att(movement->component->fight)->status() == FightAtt::DEAD)
		return -1;
	if (movement->cantMove)
		return -1;
	if (movement->component->npc != NULL && movement->component->roleAtt->aiAtt().moveType() == AIAtt::DONTMOVE)
		return -1;

	if (movement->att->logicCoord().x() == dest->x() && movement->att->logicCoord().y() == dest->y()) {
		/*
		   movement->total = 0;
		   movement->curDest = 0;
		   return 0;
		 */
		return -1;
	}

	if (!GenLine(movement, dest))
		return -1;

	movement->path[0] = *dest;
	movement->total = 1;
	movement->curDest = 0;

	ToRealPath(movement);
	MapInfoManager_PositionByLogicCoord(&movement->att->logicCoord(), movement->att->mutable_position());

	ComputeMoveDir(movement);

	return 0;
}

// int64_t begin;

int Movement_MoveStraightly(struct Movement *movement, const Vector2i *dest) {
	if (!Movement_IsValid(movement) || dest == NULL)
		return -1;

	if (MoveStraightly(movement, dest) == -1)
		return -1;

	movement->att->set_status(MovementAtt::MOVE);
	// begin = Time_ElapsedTime();
	return 0;
}

static int MoveByAStar(struct Movement *movement, const Vector2i *dest) {
	if (Fight_Att(movement->component->fight)->status() == FightAtt::DEAD)
		return -1;
	if (movement->component->npc != NULL && movement->component->roleAtt->aiAtt().moveType() == AIAtt::DONTMOVE)
		return -1;

	if (movement->att->logicCoord().x() == dest->x() && movement->att->logicCoord().y() == dest->y()) {
		/*
		   movement->total = 0;
		   movement->curDest = 0;
		   return 0;
		 */
		return -1;
	}

	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
	if (mapInfo == NULL)
		return -1;
	bool curStep = MAXSTEPS / 2;
	if (movement->component->npc != NULL) {
		const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
		if (NPCEntity_ID(movement->component->npc) == unit->boss())
			curStep = MAXSTEPS;
	}
	int steps = AStar_FindPath(mapInfo, movement->att->mapID(), &movement->att->logicCoord(), dest, curStep, movement->path, MAXSTEPS);
	if (steps == -1)
		return -1;

	for (int i = 0; i < steps; i++) {
		if (MapInfoManager_Unwalkable(mapInfo, &movement->path[i], movement->att->mapID()))
			DEBUG_LOGERROR("AStar error\n");
	}

	movement->total = steps;
	movement->curDest = 1;

	ToRealPath(movement);
	MapInfoManager_PositionByLogicCoord(&movement->att->logicCoord(), movement->att->mutable_position());

	ComputeMoveDir(movement);

	return 0;
}

int Movement_MoveByAStar(struct Movement *movement, const Vector2i *dest) {
	if (!Movement_IsValid(movement) || dest == NULL)
		return -1;
	if (movement->cantMove)
		return -1;

	if (MoveByAStar(movement, dest) == -1)
		return -1;

	movement->att->set_status(MovementAtt::MOVE);
	// begin = Time_ElapsedTime();
	return 0;
}

static int SearchNearNode(struct Movement *movement) {
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
	if (mapInfo == NULL)
		return -1;

	const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
	float dist = FLT_MAX;
	int min = -1;
	Vector3f vec;
	for (int i = 0; i < mapUnit->path_size(); i++) {
		const PathNode *destNode = &mapUnit->path(i);
		if (destNode->id() != i)
			continue;

		vec.FromPB(&destNode->pos());
		float cur = Vector3f_PowerDistance(&vec, &movement->att->position());
		if (cur < dist) {
			dist = cur;
			min = i;
		}
	}

	return min;
}

static int Move(struct Movement *movement, const Vector2i *dest) {
	if (movement->cantMove)
		return -1;

	if (MoveStraightly(movement, dest) == -1) {
		if (MoveByAStar(movement, dest) == -1) {
			return -1;
		}
	}

	return 0;
}

int Movement_MoveByPathNode(struct Movement *movement, int dest) {
	if (!Movement_IsValid(movement))
		return -1;
	if (movement->cantMove)
		return -1;
	if (Fight_Att(movement->component->fight)->status() == FightAtt::DEAD)
		return -1;
	if (movement->component->npc != NULL && movement->component->roleAtt->aiAtt().moveType() == AIAtt::DONTMOVE)
		return -1;

	if (movement->curPathNode == -1) {
		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
		if (mapInfo == NULL)
			return -1;
		const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
		if (dest < 0 || dest >= mapUnit->path_size())
			return -1;

		const PathNode *destNode = &mapUnit->path(dest);
		if (destNode->id() != dest)
			return -1;

		int near = SearchNearNode(movement);
		if (near == -1)
			return -1;
		const PathNode *curNode = &mapUnit->path(near);

		movement->curPathNode = near;
		movement->curPathPos.FromPB(&curNode->pos());
		movement->destPathNode = dest;
		movement->destPathPos.FromPB(&destNode->pos());
	}

	if (movement->curPathNode == movement->destPathNode) {
		if (movement->att->position().x() == movement->destPathPos.x() && movement->att->position().z() == movement->destPathPos.z())
			return -1;
	}

	Vector2i vec;
	MapInfoManager_LogicCoordByPosition(&movement->curPathPos, &vec);
	return Move(movement, &vec);
}

static void SendMove(struct Movement *movement, const Vector3f *dest) {
	static NetProto_Move move;
	move.Clear();
	if (movement->component->player != NULL) {
		move.set_type(NetProto_Move::PLAYER);
		move.set_id(PlayerEntity_ID(movement->component->player));
	}
	else if (movement->component->npc != NULL) {
		move.set_type(NetProto_Move::NPC);
		move.set_id(NPCEntity_ID(movement->component->npc));
	}
	movement->att->position().ToPB(move.mutable_start());
	dest->ToPB(move.mutable_end());
	GCAgent_SendProtoToAllClientsInSameScene(movement->att->mapID(), movement->component->player, &move);
}

int Movement_Follow(struct Movement *movement, struct Movement *target, float dist, int followDelta) {
	if (!Movement_SameScene(movement, target) || dist < 0)
		return -1;
	if (Fight_Att(movement->component->fight)->status() == FightAtt::DEAD)
		return -1;
	if (movement->cantMove)
		return -1;
	if (movement->component->npc != NULL && movement->component->roleAtt->aiAtt().moveType() == AIAtt::DONTMOVE)
		return -1;

	MapInfoManager_PositionByLogicCoord(&movement->att->logicCoord(), movement->att->mutable_position());

	if (Vector3f_Distance(&movement->att->position(), &target->att->position()) <= dist) {
	}
	else {
		if (Move(movement, &target->att->logicCoord()) == -1)
			return -1;

		SendMove(movement, &target->att->position());
	}

	movement->target = target;
	movement->tPrevCoord = target->att->logicCoord();
	movement->att->set_status(MovementAtt::FOLLOW);
	movement->dist = dist;
	movement->followDelta = followDelta;
	movement->prevFollow = Time_ElapsedTime();

	return 0;
}

static inline void RandomDest(const Vector2i *center, int radius, Vector2i *dest) {
	int x = Time_Random(0, radius * 2) - radius;
	int y = Time_Random(0, radius * 2) - radius;
	dest->set_x(center->x() + x);
	dest->set_y(center->y() + y);
}

static inline bool ReachDest(struct Movement *movement) {
	return movement->curDest >= movement->total;
}

static inline bool ReachCurDest(struct Movement *movement) {
	return movement->att->position().x() == movement->realPath[movement->curDest].x()
		&& movement->att->position().z() == movement->realPath[movement->curDest].z();
}

static inline bool IsNear(const Vector3f *v1, const Vector3f *v2, float dist) {
	return Vector3f_Distance(v1, v2) < dist;
}

/*
   static inline bool IsNear(const Vector2i *v1, const Vector2i *v2, int dist) {
   return abs(v1->x() - v2->x()) <= dist && abs(v1->y() - v2->y()) <= dist;
   }
 */

static inline void DoMove(struct Movement *movement) {
	// CharacterController of Unity moves faster than here.
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
	if (mapInfo == NULL)
		return;

	static Vector3f newPos;
	newPos.set_x(movement->att->position().x() + movement->att->moveSpeed() * movement->speedFactor * movement->moveDir.x() * ((float)Time_DeltaTime() / 1000));
	newPos.set_z(movement->att->position().z() + movement->att->moveSpeed() * movement->speedFactor * movement->moveDir.z() * ((float)Time_DeltaTime() / 1000));

	if (Vector3f_PowerDistance(&newPos, &movement->att->position()) > Vector3f_PowerDistance(&movement->att->position(), &movement->realPath[movement->curDest])) {
		movement->att->mutable_position()->set_x(movement->realPath[movement->curDest].x());
		movement->att->mutable_position()->set_z(movement->realPath[movement->curDest].z());
	}
	else {
		movement->att->mutable_position()->set_x(newPos.x());
		movement->att->mutable_position()->set_z(newPos.z());
	}
	MapInfoManager_LogicCoordByPosition(&movement->att->position(), movement->att->mutable_logicCoord());

	if (MapInfoManager_Unwalkable(mapInfo, &movement->att->logicCoord(), movement->att->mapID())) {
		DEBUG_LOG("Bad block, %s, cur: (%f, %f, %f), next: (%f, %f, %f), dir: (%f, %f, %f), total: %d\n", movement->component->player != NULL ? "Player" : "Other", movement->att->position().x(), movement->att->position().y(), movement->att->position().z(), movement->realPath[movement->curDest].x(), movement->realPath[movement->curDest].y(), movement->realPath[movement->curDest].z(), movement->moveDir.x(), movement->moveDir.y(), movement->moveDir.z(), movement->total);
	}

	UpdateMapContainer(movement);
}

void Movement_Update(struct Movement *movement) {
	assert(Movement_IsValid(movement));

	UpdateWaitRoom(movement);

	if (movement->att->status() == MovementAtt::IDLE) {
		if (movement->component->npc != NULL) {
			const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
			if (mapInfo != NULL) {
				if (MapInfoManager_Unwalkable(mapInfo, &movement->att->logicCoord(), movement->att->mapID())) {
					Vector3f pos;
					MapInfoManager_PositionByLogicCoord(&AI_Att(movement->component->ai)->birthCoord(), &pos);
					Movement_SetPosition(movement, &pos);
				}
			}
		}
		return;
	}
	else if (movement->att->status() == MovementAtt::MOVE) {
		if (movement->cantMove
				|| ReachDest(movement)) {
			Movement_Stop(movement);
			return;
		}

		if (ReachCurDest(movement)) {
			MapInfoManager_LogicCoordByPosition(&movement->realPath[movement->curDest], movement->att->mutable_logicCoord());
			MapInfoManager_PositionByLogicCoord(&movement->att->logicCoord(), movement->att->mutable_position());
			UpdateMapContainer(movement);

			movement->curDest++;
			if (ReachDest(movement)) {
				// cout << "Time: " << Time_ElapsedTime() - begin << endl;
				if (movement->curPathNode == -1) {
					Movement_Stop(movement);
				} else {
					if (movement->curPathNode == movement->destPathNode) {
						Movement_Stop(movement);
					} else {
						const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movement->att->mapID()));
						if (mapInfo == NULL)
							return;
						const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);

						const PathNode *cur = &mapUnit->path(movement->curPathNode);
						movement->curPathNode = cur->next(0);
						cur = &mapUnit->path(movement->curPathNode);
						movement->curPathPos.FromPB(&cur->pos());

						Vector2i vec;
						MapInfoManager_LogicCoordByPosition(&movement->curPathPos, &vec);
						Move(movement, &vec);
					}
				}
				return;
			}

			ComputeMoveDir(movement);
		}

		DoMove(movement);
	}
	else if (movement->att->status() == MovementAtt::FOLLOW) {
		if (movement->cantMove
				|| !Movement_SameScene(movement, movement->target)) {
			Movement_Stop(movement);
			return;
		}

		if (IsNear(&movement->att->position(), &movement->target->att->position(), movement->dist)) {
			return;
		}

		int64_t cur = Time_ElapsedTime();
		if (cur >= movement->prevFollow + movement->followDelta) {
			const Vector2i &tCurCoord = movement->target->att->logicCoord();
			bool change = false;
			if (tCurCoord.x() != movement->tPrevCoord.x() || tCurCoord.y() != movement->tPrevCoord.y())
				change = true;

			if (change) {
				if (Move(movement, &movement->target->att->logicCoord()) == 0) {
					movement->tPrevCoord = tCurCoord;
					SendMove(movement, &movement->target->att->position());
					movement->prevFollow = cur;
				}
			}
		}

		if (ReachDest(movement)) {
			return;
		}

		if (ReachCurDest(movement)) {
			MapInfoManager_LogicCoordByPosition(&movement->realPath[movement->curDest], movement->att->mutable_logicCoord());
			MapInfoManager_PositionByLogicCoord(&movement->att->logicCoord(), movement->att->mutable_position());
			UpdateMapContainer(movement);

			movement->curDest++;
			if (ReachDest(movement)) {
				// cout << "Time: " << Time_ElapsedTime() - begin << endl;
				return;
			}

			ComputeMoveDir(movement);
		}

		DoMove(movement);
	}
}
