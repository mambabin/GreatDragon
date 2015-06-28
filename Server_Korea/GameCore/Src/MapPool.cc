#include "MapPool.hpp"
#include "Config.hpp"
#include "MapInfoManager.hpp"
#include "MapInfo.pb.h"
#include "Math.hpp"
#include "MathUtil.hpp"
#include "Movement.hpp"
#include "MovementInfo.hpp"
#include "NPCEntity.hpp"
#include "NPCPool.hpp"
#include "NPCInfoManager.hpp"
#include "PlayOffManager.hpp"
#include "PlayerPool.hpp"
#include "FactionPool.hpp"
#include "AwardInfoManager.hpp"
#include "GCAgent.hpp"
#include "Time.hpp"
#include "Timer.hpp"
#include "Event.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <pthread.h>
#include <fstream>
#include <set>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <cstring>
#include <climits>
#include <algorithm>
#include <cmath>

using namespace std;

#define IDLE_EXPAND_STEP 64

struct Map{
	int32_t info;
	set<struct Movement *> *entities;
	struct Map *next;
	int group;
	int pass;
	int64_t passTime;
	struct Movement *link[MAX_ROOM_PLAYERS];
	int32_t v;
};

static struct{
	struct Map **maps;
	int max;
	struct Map *idles;
	int worldBoss;
	bool worldBossOver;
	int worldBossNum;
	int factionWar;
	bool factionWarOver;
	int factionWarNum;
	pthread_mutex_t worldBossNumLock;
	vector<int> hell;
	vector<struct PlayOffUnit> playoff;
	map<string, pair<int32_t, bool> > factionBoss;
}pool;

static void ExpandIdles() {
	for (int i = 0; i < IDLE_EXPAND_STEP; i++) {
		struct Map *map = new struct Map;
		map->entities = new set<struct Movement *>[Config_MaxSizeOfMap()];
		map->next = pool.idles;
		pool.idles = map;
	}
}

static int WorldBossNum() {
	int begin[24];
	int n = Config_WorldBossBegin(begin);
	int step = 3;
	assert(step >= n);

	time_t cur = Time_TimeStamp();
	struct tm t;
	Time_LocalTime(cur, &t);

	int d = (int)cur / (3600 * 24) * step;
	int h = t.tm_hour;
	if (h >= begin[n - 1]) {
		h = n;
	} else {
		for (int i = 0; i < n; i++) {
			if (h < begin[i]) {
				h = i;
				break;
			}
		}
	}
	return d + h + 1;
}

static int FactionWarNum() {
	int begin = Config_FactionWarBegin();
	time_t cur = Time_TimeStamp();
	struct tm t;
	Time_LocalTime(cur, &t);

	int d = (int)cur / (3600 * 24);
	int h = t.tm_hour;
	if (h >= begin) {
		h = 1;
	} else {
		h = 0;
	}
	return d + h + 1;
}

void MapPool_Init() {
	int res = pthread_mutex_init(&pool.worldBossNumLock, NULL);
	assert(res == 0);

	pool.max = Config_MaxMaps();
	pool.maps = new struct Map *[pool.max];
	memset(pool.maps, 0, sizeof(pool.maps[0]) * pool.max);

	pool.idles = NULL;
	ExpandIdles();

	pool.worldBoss = -1;
	pool.worldBossOver = false;
	pool.worldBossNum = WorldBossNum();
	pool.factionWar = -1;
	pool.factionWarOver = false;
	pool.factionWarNum = FactionWarNum();
	pool.hell.clear();
	pool.playoff.clear();
	pool.factionBoss.clear();
}

void MapPool_ReleaseNPCs(int32_t map, int32_t id, bool notice) {
	struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
	int count = 0;
	if (id == -1) {
		count = NPCPool_NPCs(map, npcs, CONFIG_FIXEDARRAY);
		if (count < 0) {
			DEBUG_LOGERROR("Failed to get all npcs");
			return;
		}
	} else {
		npcs[count++] = NPCPool_NPC(map, id);
		if (npcs[0] == NULL)
			return;
	}

	if (notice) {
		static NetProto_DelNPCs proto;
		proto.Clear();
		for (int i = 0; i < count; i++)
			proto.add_id(NPCEntity_ID(npcs[i]));
		if (proto.id_size() > 0)
			GCAgent_SendProtoToAllClientsInSameScene(map, &proto);
	}

	for (int i = 0; i < count; i++)
		Component_Destroy(NPCEntity_Component(npcs[i]));
}

static void OpenWorldBoss(void *arg) {
	if (pool.worldBoss != -1)
		return;
	int32_t room = MapPool_Gen(Config_WorldBossMap(), true);
	if (room == -1) {
		DEBUG_LOGERROR("Failed to gen worldboss map");
	} else {
		static NetProto_Message proto;
		proto.set_content(Config_Words(35));
		proto.set_count(1);
		GCAgent_SendProtoToAllClients(&proto);
		DEBUG_LOG("open worldboss");
	}
	pool.worldBossOver = false;
	PlayerPool_Push(Config_Words(35));
}
static void CloseWorldBoss(void *arg) {
	if (pool.worldBoss >= 0) {
		if (!pool.worldBossOver) {
			MapPool_ClearRoom(pool.worldBoss, false);

			static DCProto_InitRank proto;
			proto.Clear();
			proto.set_type(NetProto_Rank::WORLD_BOSS);
			proto.set_flag(true);
			proto.mutable_finalKiller()->mutable_role()->set_roleID(-1);
			proto.mutable_finalKiller()->set_arg1(MapPool_WorldBossNum());
			GCAgent_SendProtoToDCAgent(&proto);
		}

		pool.maps[pool.worldBoss]->passTime = -2;
		pool.worldBoss = -1;
		pool.worldBossOver = false;
	}
}

static void OpenFactionWar(void *arg) {
	if (pool.factionWar != -1)
		return;
	int32_t room = MapPool_Gen(Config_FactionWarMap(), true);
	if (room == -1) {
		DEBUG_LOGERROR("Failed to gen factionwar map");
	} else {
		static NetProto_Message proto;
		proto.set_content(Config_Words(75));
		proto.set_count(1);
		GCAgent_SendProtoToAllClients(&proto);
		DEBUG_LOG("open factionwar");
	}
	pool.factionWarOver = false;
	// PlayerPool_Push(Config_Words(35));
}
static void CloseFactionWar(void *arg) {
	if (pool.factionWar >= 0) {
		if (!pool.factionWarOver) {
			MapPool_ClearFactionWar();
		}

		pool.maps[pool.factionWar]->passTime = -2;
		pool.factionWar = -1;
		pool.factionWarOver = false;
	}
}

static void OpenHellReally();
static void CloseHell(void *arg) {
	if (pool.hell.empty())
		return;
	for (size_t i = 0; i < pool.hell.size(); i++) {
		MapPool_ClearRoom(pool.hell[i]);
		pool.maps[pool.hell[i]]->passTime = -2;
	}
	pool.hell.clear();
	OpenHellReally();
}
static void OpenHellReally() {
	// if (pool.hell != -1)
	// 	return;
	CloseHell(NULL);
	if (pool.hell.empty()) {
		int32_t room = MapPool_Gen(Config_HellMap(), true);
		if (room == -1) {
			DEBUG_LOGERROR("Failed to gen hell map");
		}
	}
}
static void OpenHell(void *arg) {
	OpenHellReally();
	static NetProto_Message proto;
	proto.set_content(Config_Words(48));
	proto.set_count(1);
	GCAgent_SendProtoToAllClients(&proto);
	PlayerPool_Push(Config_Words(48));
	DEBUG_LOG("open hell");
}

static void OpenGetRMB(void *arg) {
	static NetProto_Message proto;
	proto.set_content(Config_Words(49));
	proto.set_count(1);
	GCAgent_SendProtoToAllClients(&proto);
	PlayerPool_Push(Config_Words(49));
}

static void OpenGetDurability(void *arg) {
	/*
	   static NetProto_Message proto;
	   proto.set_content(Config_Words(50));
	   proto.set_count(1);
	   GCAgent_SendProtoToAllClients(&proto);
	   PlayerPool_Push(Config_Words(50));
	   */
}

static void DelPlayOff(int32_t room) {
	for (int i = 0; i < (int)pool.playoff.size(); i++) {
		struct PlayOffUnit *unit = &pool.playoff[i];
		if (unit->room == room) {
			unit->total++;
			const PlayOff *info = PlayOffManager_PlayOff(unit->playoff);
			int door = PlayOffManager_Count(info->over(unit->day));
			if (unit->total >= door) {
				pool.playoff.erase(pool.playoff.begin() + i);
			} else {
				int overTime = unit->overTime + (Config_PlayOffTime() + Config_PlayOffInterval() + Config_PlayOffWait()) / 1000;
				unit->room = MapPool_Gen(Config_PlayOffMap(), true, overTime);
				unit->overTime = overTime;
			}
			return;
		}
	}
}

void MapPool_GenNPC(int32_t mapID, int id, NPCAtt *att, const Vector2i *coord, struct Component *master, bool notice) {
	if (MapPool_MapInfo(mapID) == -1 || att == NULL || coord == NULL) {
		DEBUG_LOGERROR("Failed to create npc");
		return;
	}
	att->mutable_att()->mutable_movementAtt()->set_mapID(mapID);
	*att->mutable_att()->mutable_movementAtt()->mutable_logicCoord() = *coord;
	MapInfoManager_PositionByLogicCoord(&att->mutable_att()->mutable_movementAtt()->logicCoord(), att->mutable_att()->mutable_movementAtt()->mutable_position());
	*att->mutable_att()->mutable_aiAtt()->mutable_birthCoord() = att->att().movementAtt().logicCoord();
	att->mutable_att()->mutable_fightAtt()->set_hp(Fight_FinalProperty(&att->att().fightAtt(), FightAtt::MAXHP));
	att->mutable_att()->mutable_fightAtt()->set_mana(Config_MaxMana());
	struct NPCEntity *entity = NPCEntity_Create(id, att, master);
	if (entity == NULL) {
		DEBUG_LOGERROR("Failed to create npc");
		return;
	}

	if (notice) {
		static NetProto_AddNPCs proto;
		proto.Clear();
		proto.add_id(id);
		NPCEntity_ToSceneData(NPCEntity_Att(entity), proto.add_att());
		GCAgent_SendProtoToAllClientsInSameScene(mapID, &proto);
	}
}

static void GenNPC(int32_t mapID, const NPCSceneUnit *npc) {
	if (npc->id() == -1)
		return;
	int32_t resID = npc->resID();
	if (resID == -1)
		return;
	NPCAtt att = *NPCInfoManager_NPC(resID);
	static Vector2i coord;
	coord.FromPB(&npc->coord());
	MapPool_GenNPC(mapID, npc->id(), &att, &coord);
}

static void UpdateMap(void *arg) {
	int64_t cur = Time_ElapsedTime();
	for (int i = MAP_START; i < Config_MaxMaps(); i++) {
		struct Map *map = pool.maps[i];
		if (map == NULL)
			continue;

		const MapInfo *info = MapInfoManager_MapInfo(map->info);
		assert(info != NULL);
		const MapUnit *mapUnit = MapInfoManager_MapUnit(info);

		if (info->id() == Config_PlayOffMap()) {
			if (map->passTime == -2) {
				MapPool_Release(i);
				continue;
			}
			int t = (int)Time_TimeStamp();
			if (t > map->passTime) {
				// end
				if (map->pass != 2) {
					static vector<struct PlayerEntity *> players;
					players.clear();
					int count = PlayerPool_Players(i, &players);
					if (count == 2) {
						int life = 0;
						struct Component *a = PlayerEntity_Component(players[0]);
						if (a != NULL && a->roleAtt->fightAtt().hp() > 0)
							life++;
						struct Component *b = PlayerEntity_Component(players[1]);
						if (b != NULL && b->roleAtt->fightAtt().hp() > 0)
							life++;
						if (life == 2) {
							float hpA = (float)a->roleAtt->fightAtt().hp() / Fight_FinalProperty(a->fight, FightAtt::MAXHP);
							float hpB = (float)b->roleAtt->fightAtt().hp() / Fight_FinalProperty(b->fight, FightAtt::MAXHP);
							if (hpA > hpB) {
								Fight_ClearRoom(a->fight, true, 0);
							} else {
								Fight_ClearRoom(b->fight, true, 0);
							}
						}
					}
					DelPlayOff(i);
					map->pass = 2;
					map->passTime = -2;
				}
			} else if (t > map->passTime - Config_PlayOffTime() / 1000) {
				// ready
				if (map->pass != 1) {
					static vector<struct PlayerEntity *> players;
					players.clear();
					int count = PlayerPool_Players(i, &players);
					if (count != 2) {
						if (count == 1) {
							struct Component *component = PlayerEntity_Component(players[0]);
							if (component != NULL)
								Fight_ClearRoom(component->fight, true, 0);
						} else {
							struct Movement *linkers[MAX_ROOM_PLAYERS];
							count = MapPool_Linkers(i, linkers, MAX_ROOM_PLAYERS);
							for (int j = 0; j < count; j++) {
								struct Component *component = Movement_Component(linkers[j]);
								if (component == NULL)
									continue;
								Fight_ClearRoom(component->fight, false, 0);
								break;
							}
						}
					} else {
						static NetProto_BeginFighting proto;
						GCAgent_SendProtoToAllClientsInSameScene(i, &proto);
					}
					map->pass = 1;
				}
			}
		} else if (info->id() == PlayerPool_ReservationMap() ) {
			/*
			std::vector<struct PlayerEntity *> players;
					DEBUG_LOGERROR("send fighting");

			PlayerPool_Players(info->id(), &players);
			for (int i = 0; i < (int)players.size(); ++i) {
				int64_t roleID = PlayerEntity_RoleID(players[i]);
				int32_t id = PlayerEntity_ID(players[i]);
				if (PlayerPool_ReservationEnterOrPower(roleID, 2)) {
					DEBUG_LOGERROR("send fighting");
					static NetProto_BeginFighting proto;
					GCAgent_SendProtoToAllClientsInSameScene(id, &proto);
				}
			}
*/
		} else if (info->mapType() == MapInfo::HELL) {
			MapPool_Release(i);
			map = pool.maps[i];
			if (map == NULL)
				continue;

			static vector<struct PlayerEntity *> players;
			players.clear();
			int playerCount = PlayerPool_Players(i, &players);
			if (playerCount > 0) {
				struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
				int npcCount = NPCPool_NPCs(i, npcs, CONFIG_FIXEDARRAY);
				if (npcCount < 0)
					npcCount = 0;
				if (npcCount < Config_MaxHellNPCs()) {
					static vector<int64_t> exists;
					exists.clear();
					for (int j = 0; j < npcCount; j++) {
						struct Component *component = NPCEntity_Component(npcs[j]);
						if (component != NULL)
							exists.push_back(component->baseAtt->roleID());
					}
					struct Component *minComponent = NULL;
					int minPower = INT_MAX;
					for (int j = 0; j < playerCount; j++) {
						struct Component *component = PlayerEntity_Component(players[j]);
						if (component != NULL) {
							exists.push_back(component->baseAtt->roleID());
							int power = Fight_Power(component->fight);
							if (power < minPower) {
								minPower = power;
								minComponent = component;
							}
						}
					}
					if (minComponent != NULL) {
						static vector<PlayerAtt> atts;
						atts.clear();
						PlayerPool_RandomFreeRoles(Config_MaxHellNPCs() - npcCount, &exists, &atts);
						for (size_t j = 0; j < atts.size(); j++) {
							PlayerAtt *playerAtt = &atts[j];

							for (int k = 0; k < playerAtt->att().fightAtt().properties_size(); k++) {
								playerAtt->mutable_att()->mutable_fightAtt()->set_properties(k, minComponent->roleAtt->fightAtt().properties(k));
								*playerAtt->mutable_att()->mutable_fightAtt()->mutable_propertiesDelta(k) =  minComponent->roleAtt->fightAtt().propertiesDelta(k);
							}

							Vector2i coord;
							MapInfoManager_EnterCoord(info, MapInfoManager_RandomEnterCoord(info), &coord);

							NPCAtt att;
							PlayerEntity_CopyAsNPC(playerAtt, &att, 0.8f, false, NPCPool_FreeFaction(i), Config_HellReviveTime());
							MapPool_GenNPC(i, NPCPool_FreeID(i), &att, &coord, NULL, true);
						}
					}
				}
			} else {
				MapPool_ReleaseNPCs(i, -1, false);
			}
		} else if (info->id() == Config_WorldBossMap()
				|| info->id() == Config_FactionWarMap()
				|| info->id() == Config_FactionBossMap()
				|| info->mapType() == MapInfo::PVP
				|| info->mapType() == MapInfo::PRACTICE) {
			MapPool_Release(i);
		} else if (info->mapType() == MapInfo::ONLY_ROOM
				|| (info->mapType() == MapInfo::ROOM && !MapPool_IsSingle(i))) {
			MapPool_Release(i);
			map = pool.maps[i];
			if (map == NULL)
				continue;

			if (map->group >= mapUnit->npcGroups_size())
				continue;

			if (map->passTime == -1) {
				if (PlayerPool_Count(i, 0) > 0)
					map->passTime = Time_ElapsedTime();
				else
					continue;
			}

			const NPCGroup *npcGroup = &mapUnit->npcGroups(map->group);
			if (map->pass >= npcGroup->pass_size()) {
				int count = NPCPool_MonsterCount(i);
				if (count <= 0) {
					const ::google::protobuf::RepeatedPtrField< ::ObstacleTriggerInfo >& mObstacleTriggerInfo = mapUnit->obstacleTriggerInfo();
					for (int j = 0; j < mObstacleTriggerInfo.size(); ++j) {
						if (mObstacleTriggerInfo.Get(j).openType() == ObstacleTriggerInfo::KILL_GROUP_NPC) {
							//const ::google::protobuf::RepeatedPtrField< ::google::protobuf::int32 >& mparam = mObstacleTriggerInfo.Get(j).para;
							if (mObstacleTriggerInfo.Get(j).param(0) == map->group/* && mObstacleTriggerInfo.Get(j).param(1) == map->pass*/) {
								map->v |= (1 << (mObstacleTriggerInfo.Get(j).id() - 1));

								static NetProto_OpenObstacle proto;
								proto.Clear();
								proto.set_id(mObstacleTriggerInfo.Get(j).id());
								GCAgent_SendProtoToAllClientsInSameScene(i, &proto);

								static struct Movement *linkers[MAX_ROOM_PLAYERS];
								int count = MapPool_Linkers(i, linkers, MAX_ROOM_PLAYERS);
								for (int j = 0; j < count; j++) {
									struct Component *component = Movement_Component(linkers[j]);
									if (component == NULL)
										continue;
									int32_t id = PlayerEntity_ID(component->player);
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
						}
					}
					map->group++;

					bool over = false;
					if (info->id() == Config_SurviveMap()) {
						const MapInfo *curPowerInfo = MapInfoManager_MapInfo(Config_TowerBegin() + map->group);
						if (curPowerInfo == NULL) {
							over = true;
						} else {
							static vector<struct PlayerEntity *> players;
							players.clear();
							int num = PlayerPool_Players(i, NULL, &players);
							int maxPower = 0;
							for (int j = 0; j < num; j++) {
								if (PlayerEntity_IsValid(players[j])) {
									int power = Fight_Power(PlayerEntity_Component(players[j])->fight);
									if (power > maxPower)
										maxPower = power;
								}
							}
							if (maxPower < curPowerInfo->requiredPower())
								over = true;
						}
					}

					map->pass = 0;
					map->passTime = cur;
					if (over || map->group >= mapUnit->npcGroups_size())
						MapPool_ClearRoom(i);
					continue;
				}
			} else {
				const NPCPass *npcPass = &npcGroup->pass(map->pass);
				if (cur - map->passTime >= npcPass->delay()) {
					static NetProto_GenNPCs genNPCs;
					genNPCs.Clear();
					genNPCs.set_group(map->group);
					genNPCs.set_pass(map->pass);
					GCAgent_SendProtoToAllClientsInSameScene(i, NULL, &genNPCs);

					int maxNPCsPerPass = Config_MaxNPCsPerPass();
					for (int j = 0; j < npcPass->npcs_size(); j++) {
						if (j >= maxNPCsPerPass)
							break;

						int32_t npcID = npcPass->npcs(j);
						if (npcID < 0 || npcID >= mapUnit->npcs_size())
							continue;

						const NPCSceneUnit *unit = &mapUnit->npcs(npcID);
						if (unit->id() != npcID)
							continue;

						GenNPC(i, unit);
					}

					map->pass++;
					map->passTime = cur;
				}
			}
		}
	}
}

static void OpenFactionBoss(void *arg) {
	if (pool.factionBoss.empty()) {
		static NetProto_Message proto;
		proto.set_content(Config_Words(65));
		proto.set_count(1);
		GCAgent_SendProtoToAllClients(&proto);
	}
}

static void CloseFactionBoss(void *arg) {
	for (map<string, pair<int32_t, bool> >::iterator it = pool.factionBoss.begin(); it != pool.factionBoss.end(); it++) {
		if (it->second.first >= 0) {
			if (!it->second.second) {
				static NetProto_Lose lose;
				lose.Clear();
				GCAgent_SendProtoToAllClientsInSameScene(it->second.first, &lose);
			}

			pool.maps[it->second.first]->passTime = -2;
			it->second.first = -1;
			it->second.second = false;
		}
	}
	pool.factionBoss.clear();
}

void MapPool_Prepare() {
	Timer_AddEvent(0, 0, UpdateMap, NULL, "UpdateMap");

	int all[CONFIG_FIXEDARRAY];
	int total = 0;
	int begin[CONFIG_FIXEDARRAY];
	int n = Config_WorldBossBegin(begin);
	for (int i = 0; i < n; i++) {
		all[total++] = begin[i];
		for (int j = begin[i] + 1; j < begin[i] + Config_WorldBossTime() / (1000 * 3600); j++)
			all[total++] = j;
	}
	Timer_AddEvent(-1, all, total, OpenWorldBoss, NULL);
	for (int i = 0; i < n; i++)
		begin[i] += Config_WorldBossTime() / (1000 * 3600);
	Timer_AddEvent(-1, begin, n, CloseWorldBoss, NULL);

	n = Config_FactionWarDay(all);
	for (int i = 0; i < n; i++) {
		begin[0] = Config_FactionWarBegin();
		Timer_AddEvent(all[i], begin, 1, OpenFactionWar, NULL);
		begin[0] = Config_FactionWarBegin() + Config_FactionWarTime() / (1000 * 3600);
		Timer_AddEvent(all[i], begin, 1, CloseFactionWar, NULL);
	}

	OpenHellReally();

	total = 0;
	begin[0] = Config_HellBegin();
	all[total++] = begin[0];
	for (int j = begin[0] + 1; j < begin[0] + Config_HellTime() / (1000 * 3600); j++)
		all[total++] = j;
	Timer_AddEvent(-1, all, total, OpenHell, NULL);
	begin[0] += Config_HellTime() / (1000 * 3600);
	Timer_AddEvent(-1, begin, 1, CloseHell, NULL);

	total = 0;
	begin[0] = Config_FactionBossBegin();
	all[total++] = begin[0];
	for (int j = begin[0] + 1; j < begin[0] + Config_FactionBossTime() / (1000 * 3600); j++)
		all[total++] = j;
	Timer_AddEvent(-1, all, total, OpenFactionBoss, NULL);
	begin[0] += Config_FactionBossTime() / (1000 * 3600);
	Timer_AddEvent(-1, begin, 1, CloseFactionBoss, NULL);

	begin[0] = Config_GetRMBBegin1();
	begin[1] = Config_GetRMBBegin2();
	Timer_AddEvent(-1, begin, 2, OpenGetRMB, NULL);

	begin[0] = Config_GetDurabilityBegin1();
	begin[1] = Config_GetDurabilityBegin2();
	Timer_AddEvent(-1, begin, 2, OpenGetDurability, NULL);
}

int32_t MapPool_WorldBoss() {
	return pool.worldBossOver ? -2 : pool.worldBoss;
}

int32_t MapPool_WorldBossNum() {
	pthread_mutex_lock(&pool.worldBossNumLock);
	int32_t ret = pool.worldBossNum;
	pthread_mutex_unlock(&pool.worldBossNumLock);
	return ret;
}

int32_t MapPool_FactionWar() {
	return pool.factionWarOver ? -2 : pool.factionWar;
}

int32_t MapPool_FactionWarNum() {
	pthread_mutex_lock(&pool.worldBossNumLock);
	int32_t ret = pool.factionWarNum;
	pthread_mutex_unlock(&pool.worldBossNumLock);
	return ret;
}

int32_t MapPool_Hell() {
	if (pool.hell.empty())
		return -1;
	for (size_t i = 0; i < pool.hell.size(); i++) {
		int count1 = PlayerPool_Count(pool.hell[i], -1);
		if (count1 < 0)
			count1 = 0;
		int count2 = MapPool_LinkersCount(pool.hell[i]);
		if (count2 < 0)
			count2 = 0;
		if (count1 + count2 < Config_MaxPlayersOfLineInWorldBoss())
			return pool.hell[i];
	}
	int32_t room = MapPool_Gen(Config_HellMap(), true);
	if (room == -1) {
		DEBUG_LOGERROR("Failed to gen hell map");
	}
	return room;
}

bool MapPool_IsHellValid(int32_t room) {
	for (size_t i = 0; i < pool.hell.size(); i++) {
		if (pool.hell[i] == room)
			return true;
	}
	return false;
}

struct PlayOffUnit * MapPool_SelectPlayOff(int64_t roleID, int32_t playoff, int day, int pass, int turn, int overTime) {
	if (roleID == -1)
		return NULL;

	vector<struct PlayOffUnit> &all = pool.playoff;
	for (size_t i = 0; i < all.size(); i++) {
		struct PlayOffUnit *unit = &all[i];
		if (unit->lhs == roleID || unit->rhs == roleID)
			return unit;
	}

	if (playoff != -1) {
		const PlayOff *info = PlayOffManager_PlayOff(playoff);
		if (info == NULL)
			return NULL;
		if (day < 0 || day >= info->time_size())
			return NULL;

		int n = (day == 0 ? info->limit() : info->over(day - 1)) / 2;
		for (int i = 0; i < pass; i++)
			n /= 2;
		if (n <= 0)
			n = 1;
		if ((int)all.size() < n) {
			struct PlayOffUnit unit;
			unit.lhs = roleID;
			unit.rhs = -1;
			struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
			if (player != NULL) {
				unit.lName = PlayerEntity_Component(player)->baseAtt->name();
				PlayerEntity_ToSceneData(PlayerEntity_Att(player), &unit.lAtt);
			}
			unit.rAtt.mutable_att()->mutable_baseAtt()->set_roleID(-1);
			unit.room = MapPool_Gen(Config_PlayOffMap(), true, overTime);
			unit.playoff = playoff;
			unit.day = day;
			unit.pass = pass;
			unit.lWin = 0;
			unit.rWin = 0;
			unit.overTime = overTime;
			unit.total = turn;
			pool.playoff.push_back(unit);
			return &all[all.size() - 1];
		}

		for (size_t i = 0; i < all.size(); i++) {
			struct PlayOffUnit *unit = &all[i];
			if (unit->lhs == -1 || unit->rhs == -1) {
				if (unit->lhs == -1) {
					unit->lhs = roleID;
					struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
					if (player != NULL) {
						unit->lName = PlayerEntity_Component(player)->baseAtt->name();
						PlayerEntity_ToSceneData(PlayerEntity_Att(player), &unit->lAtt);
					}
				} else {
					unit->rhs = roleID;
					struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
					if (player != NULL) {
						unit->rName = PlayerEntity_Component(player)->baseAtt->name();
						PlayerEntity_ToSceneData(PlayerEntity_Att(player), &unit->rAtt);
					}
				}
				return unit;
			}
		}
	}

	return NULL;
}

int32_t MapPool_FactionBoss(const std::string &faction) {
	map<string, pair<int32_t, bool> >::iterator it = pool.factionBoss.find(faction);
	if (it == pool.factionBoss.end())
		return -1;
	else
		return it->second.second ? -2 : it->second.first;
}

const char * MapPool_FactionOfBoss(int32_t room) {
	for (map<string, pair<int32_t, bool> >::iterator it = pool.factionBoss.begin(); it != pool.factionBoss.end(); it++) {
		if (it->second.first == room)
			return it->first.c_str();
	}
	return NULL;
}

int32_t MapPool_GenFactionBoss(const std::string &faction) {
	int32_t room = MapPool_FactionBoss(faction);
	if (room >= 0)
		return room;

	room = MapPool_Gen(Config_FactionBossMap(), true);
	if (room == -1) {
		DEBUG_LOGERROR("Failed to gen faction boss map");
	}
	pool.factionBoss[faction] = make_pair(room, false);
	return room;
}

int32_t MapPool_Gen(int32_t info, bool multipleRoom, int overTime) {
	const MapInfo *mapInfo = MapInfoManager_MapInfo(info);
	if (mapInfo == NULL)
		return -1;

	if (pool.idles == NULL) {
		ExpandIdles();
		assert(pool.idles != NULL);
	}

	int32_t ret = -1;
	if (mapInfo->mapType() == MapInfo::PEACE
			|| mapInfo->mapType() == MapInfo::SINGLE
			|| (mapInfo->mapType() == MapInfo::ROOM && !multipleRoom)) {
		if (info >= pool.max)
			return -1;

		ret = info;
	} else if (mapInfo->mapType() == MapInfo::ONLY_ROOM
			|| mapInfo->mapType() == MapInfo::PRACTICE
			|| mapInfo->mapType() == MapInfo::PVP
			|| mapInfo->mapType() == MapInfo::HELL
			|| (mapInfo->mapType() == MapInfo::ROOM && multipleRoom)) {
		for (int i = MAP_START; i < pool.max; i++) {
			if (pool.maps[i] != NULL)
				continue;

			ret = i;
			DEBUG_LOG("Gen room: %d", ret);
			break;
		}
	} else {
		assert(0);
	}

	if (ret == -1)
		return -1;

	pool.maps[ret] = pool.idles;
	pool.maps[ret]->info = info;
	pool.maps[ret]->group = 0;
	pool.maps[ret]->pass = 0;
	pool.maps[ret]->passTime = -1;
	pool.idles = pool.idles->next;
	pool.maps[ret]->v = 0;

	for (int i = 0; i < MAX_ROOM_PLAYERS; i++)
		pool.maps[ret]->link[i] = NULL;

	if (mapInfo->mapType() == MapInfo::PEACE) {
		const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
		for (int i = 0; i < unit->npcs_size(); i++)
			GenNPC(ret, &unit->npcs(i));
	} else if (mapInfo->id() == Config_SurviveMap()) {
		if (multipleRoom) {
			const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
			GenNPC(ret, &unit->npcs(0));
		}
	}

	if (mapInfo->id() == Config_WorldBossMap()) {
		pool.worldBoss = ret;

		pthread_mutex_lock(&pool.worldBossNumLock);
		pool.worldBossNum = WorldBossNum();
		pthread_mutex_unlock(&pool.worldBossNumLock);

		const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
		for (int i = 0; i < unit->npcs_size(); i++)
			GenNPC(ret, &unit->npcs(i));
	} else if (mapInfo->id() == Config_FactionWarMap()) {
		pool.factionWar = ret;

		pthread_mutex_lock(&pool.worldBossNumLock);
		pool.factionWarNum = FactionWarNum();
		pthread_mutex_unlock(&pool.worldBossNumLock);

		const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
		for (int i = 0; i < unit->npcs_size(); i++)
			GenNPC(ret, &unit->npcs(i));

		if (MapPool_FactionWarNum() != PlayerPool_CurFactionFight()) {
			PlayerPool_ClearFactionFightInfo();
		}
	} else if (mapInfo->id() == Config_FactionBossMap()) {
		const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
		for (int i = 0; i < unit->npcs_size(); i++)
			GenNPC(ret, &unit->npcs(i));
	} else if (mapInfo->mapType() == MapInfo::HELL) {
		pool.hell.push_back(ret);
	} else if (mapInfo->id() == Config_PlayOffMap()) {
		pool.maps[ret]->passTime = overTime;
	}

	return ret;
}

static void ReleaseMap(int32_t map) {
	if (map < 0 || map >= pool.max)
		return;
	if (pool.maps[map] == NULL)
		return;

	MapPool_ReleaseNPCs(map);

	pool.maps[map]->next = pool.idles;
	pool.idles = pool.maps[map];
	pool.maps[map] = NULL;

	DEBUG_LOG("Release room: %d", map);
}

void MapPool_Release(int32_t map) {
	if (map < 0 || map >= pool.max)
		return;
	if (pool.maps[map] == NULL)
		return;

	const MapInfo *mapInfo = MapInfoManager_MapInfo(pool.maps[map]->info);
	assert(mapInfo != NULL);

	if (mapInfo->mapType() == MapInfo::PEACE || MapPool_IsSingle(map)) {
		return;
	} else if (mapInfo->id() == Config_WorldBossMap()
			|| mapInfo->id() == Config_FactionWarMap()
			|| mapInfo->id() == Config_PlayOffMap()
			|| mapInfo->id() == Config_FactionBossMap()
			|| mapInfo->mapType() == MapInfo::HELL) {
		if (pool.maps[map]->passTime == -2) {
			int count = PlayerPool_Count(map, -1);
			if (count <= 0) {
				bool empty = true;
				for (int i = 0; i < MAX_ROOM_PLAYERS; i++) {
					if (pool.maps[map]->link[i] != NULL) {
						empty = false;
						break;
					}
				}
				if (empty)
					ReleaseMap(map);
			}
		}
	} else if (mapInfo->mapType() == MapInfo::ONLY_ROOM
			|| mapInfo->mapType() == MapInfo::PRACTICE
			|| mapInfo->mapType() == MapInfo::PVP
			|| mapInfo->mapType() == MapInfo::ROOM) {
		int count = PlayerPool_Count(map, 0);
		if (count <= 0) {
			bool empty = true;
			for (int i = 0; i < MAX_ROOM_PLAYERS; i++) {
				if (pool.maps[map]->link[i] != NULL) {
					empty = false;
					break;
				}
			}
			if (empty) {
				if (map != PlayerPool_ReservationMap())
					ReleaseMap(map);
			}
		}
	} else {
		assert(0);
	}
}

void MapPool_Link(int32_t room, struct Movement *movement) {
	if (movement == NULL)
		return;

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(room));
	if (info == NULL)
		return;

	if ((info->mapType() == MapInfo::PEACE)
			|| (info->mapType() == MapInfo::SINGLE)
			|| (info->mapType() == MapInfo::ROOM && MapPool_IsSingle(room)))
		return;

	for (int i = 0; i < MAX_ROOM_PLAYERS; i++) {
		if (pool.maps[room]->link[i] == NULL) {
			pool.maps[room]->link[i] = movement;
			break;
		}
	}
}

void MapPool_Unlink(int32_t room, struct Movement *movement) {
	if (movement == NULL)
		return;

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(room));
	if (info == NULL)
		return;

	if ((info->mapType() == MapInfo::PEACE)
			|| (info->mapType() == MapInfo::SINGLE)
			|| (info->mapType() == MapInfo::ROOM && MapPool_IsSingle(room)))
		return;

	for (int i = 0; i < MAX_ROOM_PLAYERS; i++) {
		if (pool.maps[room]->link[i] == movement) {
			pool.maps[room]->link[i] = NULL;
			break;
		}
	}
}

int MapPool_Linkers(int32_t room, struct Movement **movements, size_t size) {
	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(room));
	if (info == NULL)
		return -1;

	if ((info->mapType() == MapInfo::PEACE)
			|| (info->mapType() == MapInfo::SINGLE)
			|| (info->mapType() == MapInfo::ROOM && MapPool_IsSingle(room)))
		return -1;

	int count = 0;
	for (int i = 0; i < MAX_ROOM_PLAYERS; i++) {
		if (pool.maps[room]->link[i] != NULL) {
			if (count >= (int)size)
				return -1;
			movements[count++] = pool.maps[room]->link[i];
		}
	}
	return count;
}

int MapPool_LinkersCount(int32_t room) {
	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(room));
	if (info == NULL)
		return -1;

	if ((info->mapType() == MapInfo::PEACE)
			|| (info->mapType() == MapInfo::SINGLE)
			|| (info->mapType() == MapInfo::ROOM && MapPool_IsSingle(room)))
		return -1;

	int count = 0;
	for (int i = 0; i < MAX_ROOM_PLAYERS; i++) {
		if (pool.maps[room]->link[i] != NULL) {
			count++;
		}
	}
	return count;
}

int32_t MapPool_MapInfo(int32_t map) {
	if (map < 0 || map >= pool.max)
		return -1;
	if (map < MAP_START)
		return map;
	if (pool.maps[map] == NULL)
		return -1;

	return pool.maps[map]->info;
}

bool MapPool_IsSingle(int32_t map) {
	int32_t real = MapPool_MapInfo(map);
	if (real == -1) {
		return false;
	} else if (real < MAP_START) {
		return false;
	} else {
		const MapInfo *mapInfo = MapInfoManager_MapInfo(real);
		if (mapInfo == NULL)
			return false;

		if (mapInfo->mapType() == MapInfo::SINGLE)
			return true;
		if (mapInfo->id() == Config_GodMap())
			return true;
		// for debug
		else if (mapInfo->mapType() == MapInfo::ROOM /*&& map == real*/)
			return false;
		else
			return false;
	}
}

static inline void LogicCoordByIndex(const MapInfo *mapInfo, int index, Vector2i *logicCoord) {
	assert(index >= 0 && index < Config_MaxSizeOfMap());
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	logicCoord->set_x(index % blockInfo->logicWidth());
	logicCoord->set_y(index / blockInfo->logicWidth());
}

static inline int IndexByLogicCoord(const MapInfo *mapInfo, const Vector2i *logicCoord) {
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	int index = logicCoord->y() * blockInfo->logicWidth() + logicCoord->x();
	assert(index >= 0 && index < Config_MaxSizeOfMap());
	return index;
}

/*
   static void UpdateEntity(const Vector2i *center, struct Movement *movement, bool add) {
   const MovementAtt *att = Movement_Att(movement);
   if (pool.maps[att->mapID()] == NULL)
   return;

   const MapInfo *mapInfo = MapInfoManager_MapInfo(pool.maps[att->mapID()]->info);
   if (mapInfo == NULL)
   return;
   struct Map *container = pool.maps[att->mapID()];
   static Vector2i coord;

   for (int radius = 0; radius <= att->radius(); radius++) {
   for (int x = center->x() - radius; x <= center->x() + radius; x += radius * 2) {
   for (int y = center->y() - radius; y <= center->y() + radius; y++) {
   coord.set_x(x);
   coord.set_y(y);
   if (!MapInfoManager_IsInScene(mapInfo, &coord))
   continue;

   int index = IndexByLogicCoord(mapInfo, &coord);
   if (add) {
// for debug
// bool find = false;
// for (set<struct Movement *>::iterator it = container->entities[index].begin(); it != container->entities[index].end(); it++) {
// 	if (*it == movement)
// 		find = true;
// }
// assert(!find);
container->entities[index].insert(movement);
}
else {
// for debug
// bool find = false;
// for (set<struct Movement *>::iterator it = container->entities[index].begin(); it != container->entities[index].end(); it++) {
// 	if (*it == movement)
// 		find = true;
// }
// assert(find);
container->entities[index].erase(movement);
}
}
if (radius == 0)
break;
}
for (int y = center->y() - radius; y <= center->y() + radius; y += radius * 2) {
for (int x = center->x() - radius + 1; x <= center->x() + radius - 1; x++) {
coord.set_x(x);
coord.set_y(y);
if (!MapInfoManager_IsInScene(mapInfo, &coord))
continue;

int index = IndexByLogicCoord(mapInfo, &coord);
if (add) {
// for debug
// bool find = false;
// for (set<struct Movement *>::iterator it = container->entities[index].begin(); it != container->entities[index].end(); it++) {
// 	if (*it == movement)
// 		find = true;
// }
// assert(!find);
container->entities[index].insert(movement);
}
else {
// for debug
// bool find = false;
// for (set<struct Movement *>::iterator it = container->entities[index].begin(); it != container->entities[index].end(); it++) {
// 	if (*it == movement)
// 		find = true;
// }
// assert(find);
container->entities[index].erase(movement);
}
}
if (radius == 0)
	break;
	}
}
}
*/

void MapPool_UpdateCoord(const Vector2i *prevCoord, const Vector2i *curCoord, struct Movement *movement) {
	/*
	   if (movement == NULL)
	   return;

	   const MovementAtt *att = Movement_Att(movement);

	   if (prevCoord != NULL && att->mapID() >= MAP_START) {
	   UpdateEntity(prevCoord, movement, false);
	   }

	   if (curCoord != NULL && att->mapID() >= MAP_START) {
	   UpdateEntity(curCoord, movement, true);
	   }
	   */
}

int MapPool_Entities(int32_t mapID, const Vector2i *coord, struct PlayerEntity *self, struct Movement **entities, size_t size) {
	if (mapID < 0 || mapID >= pool.max || pool.maps[mapID] == NULL || coord == NULL || entities == NULL)
		return -1;

	/*
	   const MapInfo *info = MapInfoManager_MapInfo(pool.maps[mapID]->info);
	   if (info == NULL)
	   return -1;

	   int index = IndexByLogicCoord(info, coord);
	   set<struct Movement *> &container = pool.maps[mapID]->entities[index];

	   int i = 0;
	   for (set<struct Movement *>::iterator it = container.begin(); it != container.end(); it++) {
	   if (i >= (int)size)
	   break;

	   entities[i++] = *it;
	   }

	   return i;
	   */

	int total = 0;
	int num = 0;
	static vector<struct PlayerEntity *> players;
	players.clear();
	if (self == NULL)
		num = PlayerPool_Players(mapID, &players);
	else
		num = PlayerPool_Players(mapID, self, &players);
	for (int i = 0; i < num; i++) {
		struct Component *component = PlayerEntity_Component(players[i]);
		if (component == NULL)
			continue;
		if (total >= (int)size)
			return -1;
		const MovementAtt *att = Movement_Att(component->movement);
		if (att->logicCoord().x() == coord->x() && att->logicCoord().y() == coord->y())
			entities[total++] = component->movement;
	}
	struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
	num = NPCPool_NPCs(mapID, npcs, CONFIG_FIXEDARRAY);
	for (int i = 0; i < num; i++) {
		struct Component *component = NPCEntity_Component(npcs[i]);
		if (component == NULL)
			continue;
		if (total >= (int)size)
			return -1;
		const MovementAtt *att = Movement_Att(component->movement);
		if (att->logicCoord().x() == coord->x() && att->logicCoord().y() == coord->y())
			entities[total++] = component->movement;
	}
	return total;
}

int MapPool_SearchSquareArea(const Vector2i *center, int radius, int mapID, struct PlayerEntity *self, struct Movement **entities, size_t size, int max) {
	if (mapID < 0 || mapID >= pool.max || pool.maps[mapID] == NULL)
		return -1;

	/*
	   const MapInfo *mapInfo = MapInfoManager_MapInfo(pool.maps[mapID]->info);
	   if (mapInfo == NULL)
	   return -1;

	   static Vector2i coord;
	   int total = 0;

	   for (int x = center->x() - radius; x <= center->x() + radius; x++) {
	   for (int y = center->y() - radius; y <= center->y() + radius; y++) {
	   coord.set_x(x);
	   coord.set_y(y);
	   if (!MapInfoManager_IsInScene(mapInfo, &coord))
	   continue;

	   int count = MapPool_Entities(mapID, &coord, entities + total, size - total);
	   if (count > 0) {
	   total += count;
	   if (total > (int)size)
	   return -1;
	   if (total >= max)
	   return max;
	   }
	   }
	   }

	   return total;
	   */

	int leftX = std::max(center->x() - radius, 0);
	int rightX = center->x() + radius;
	int upY = center->y() + radius;
	int bottomY = std::max(center->y() - radius, 0);
	int total = 0;
	static vector<struct PlayerEntity *> players;
	players.clear();
	int num = 0;
	if (self == NULL)
		num = PlayerPool_Players(mapID, &players);
	else
		num = PlayerPool_Players(mapID, self, &players);
	for (int i = 0; i < num; i++) {
		struct Component *component = PlayerEntity_Component(players[i]);
		if (component == NULL)
			continue;
		if (total >= (int)size)
			return -1;
		if (total >= max)
			return total;
		const MovementAtt *att = Movement_Att(component->movement);
		if (att->logicCoord().x() >= leftX && att->logicCoord().x() <= rightX && att->logicCoord().y() >= bottomY && att->logicCoord().y() <= upY)
			entities[total++] = component->movement;
	}
	struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
	num = NPCPool_NPCs(mapID, npcs, CONFIG_FIXEDARRAY);
	for (int i = 0; i < num; i++) {
		struct Component *component = NPCEntity_Component(npcs[i]);
		if (component == NULL)
			continue;
		if (total >= (int)size)
			return -1;
		if (total >= max)
			return total;
		const MovementAtt *att = Movement_Att(component->movement);
		if (att->logicCoord().x() >= leftX && att->logicCoord().x() <= rightX && att->logicCoord().y() >= bottomY && att->logicCoord().y() <= upY)
			entities[total++] = component->movement;
	}
	return total;
}

struct Comp{
	bool operator()(struct Movement *lhs, struct Movement *rhs) {
		const MovementAtt *att1 = Movement_Att(lhs);
		if (att1 == NULL)
			return false;
		const MovementAtt *att2 = Movement_Att(rhs);
		if (att2 == NULL)
			return true;
		return (fabs(att1->logicCoord().x() - center.x()) + fabs(att1->logicCoord().y() - center.y())) < (fabs(att2->logicCoord().x() - center.x()) + fabs(att2->logicCoord().y() - center.y()));
	}
	Vector2i center;
};

int MapPool_SearchWithMinDist(const Vector2i *center, int radius, int mapID, struct PlayerEntity *self, struct Movement **entities, size_t size, int max) {
	/*
	   if (mapID < 0 || mapID >= pool.max || pool.maps[mapID] == NULL)
	   return -1;

	   const MapInfo *mapInfo = MapInfoManager_MapInfo(pool.maps[mapID]->info);
	   if (mapInfo == NULL)
	   return -1;

	   static Vector2i coord;
	   int total = 0;

	   for (int i = 0; i <= radius; i++) {
	   for (int x = center->x() - i; x <= center->x() + i; x += i * 2) {
	   for (int y = center->y() - i; y <= center->y() + i; y++) {
	   coord.set_x(x);
	   coord.set_y(y);
	   if (!MapInfoManager_IsInScene(mapInfo, &coord))
	   continue;

	   int count = MapPool_Entities(mapID, &coord, entities + total, size - total);
	   if (count > 0) {
	   total += count;
	   if (total > (int)size)
	   return -1;
	   if (total >= max)
	   return max;
	   }
	   }
	   if (i == 0)
	   break;
	   }
	   for (int y = center->y() - i; y <= center->y() + i; y += i * 2) {
	   for (int x = center->x() - i + 1; x <= center->x() + i - 1; x++) {
	   coord.set_x(x);
	   coord.set_y(y);
	   if (!MapInfoManager_IsInScene(mapInfo, &coord))
	   continue;

	   int count = MapPool_Entities(mapID, &coord, entities + total, size - total);
	   if (count > 0) {
	   total += count;
	   if (total > (int)size)
	   return -1;
	   if (total >= max)
	   return max;
	   }
	   }
	   if (i == 0)
	   break;
	   }
	   }

	   return total;
	   */

	int total = MapPool_SearchSquareArea(center, radius, mapID, self, entities, size, (int)size);
	if (total > 0) {
		struct Comp comp;
		comp.center = *center;
		sort(entities, entities + total, comp);
	}
	return min(total, max);
}

void MapPool_ClearFactionWar() {
	if (MapPool_FactionWar() < 0)
		return;

	const map<string, int64_t> *table = PlayerPool_GetFactionFightInfo();

	multimap<int64_t, string> rank;
	for (map<string, int64_t>::const_iterator it = table->begin(); it != table->end(); it++)
		rank.insert(make_pair(it->second, it->first));

	string winnerName, prevName;
	int64_t winnerHurt, prevHurt;
	if (rank.size() > 0) {
		multimap<int64_t, string>::reverse_iterator winner = rank.rbegin();
		winnerName = winner->second;
		winnerHurt = winner->first;

		PlayerPool_GetWinFactionFight(MapPool_FactionWarNum() - 1, &prevName, &prevHurt);

		AwardInfo::Type type;
		int words;
		if (prevName == winner->second) {
			type = AwardInfo::FACTION_WAR_CONTINUE;
			words = 77;
		} else {
			type = AwardInfo::FACTION_WAR_WIN;
			words = 76;
		}

		const AwardInfo *award = AwardInfoManager_AwardInfo(type, 1);
		if (award != NULL) {
			vector<int64_t> all;
			FactionPool_GetRolesFromFaction(winner->second.c_str(), 1 | 2 | 4 | 8, &all);
			for (size_t i = 0; i < all.size(); i++) {
				struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(all[i]);
				if (entity != NULL) {
					PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), Config_Words(words));
				} else {
					static DCProto_SendMail dsm;
					dsm.Clear();
					dsm.set_id(-1);
					dsm.mutable_sm()->set_receiver(all[i]);
					dsm.mutable_sm()->mutable_mail()->set_title(award->name());
					dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
					dsm.mutable_sm()->mutable_mail()->set_content(Config_Words(words));
					dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
					dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
					dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
					GCAgent_SendProtoToDCAgent(&dsm);
				}
			}
		}

		award = AwardInfoManager_AwardInfo(type, 0);
		if (award != NULL) {
			vector<int64_t> boss;
			FactionPool_GetRolesFromFaction(winner->second.c_str(), 1, &boss);
			if (boss.size() == 1) {
				struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(boss[0]);
				if (entity != NULL) {
					PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), Config_Words(words));
				} else {
					static DCProto_SendMail dsm;
					dsm.Clear();
					dsm.set_id(-1);
					dsm.mutable_sm()->set_receiver(boss[0]);
					dsm.mutable_sm()->mutable_mail()->set_title(award->name());
					dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
					dsm.mutable_sm()->mutable_mail()->set_content(Config_Words(words));
					dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
					dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
					dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
					GCAgent_SendProtoToDCAgent(&dsm);
				}
			}
		}
	}

	vector<struct PlayerEntity *> players;
	int count = PlayerPool_Players(MapPool_FactionWar(), &players);
	for (int i = 0; i < count; i++) {
		struct Component *component = PlayerEntity_Component(players[i]);
		if (component == NULL)
			continue;
		Fight_ClearRoom(component->fight, winnerName == component->playerAtt->faction(), 0);
	}

	if (Config_InActiveTime(14)) {
		if (!prevName.empty() && prevName != winnerName) {
			vector<int64_t> boss;
			FactionPool_GetRolesFromFaction(prevName.c_str(), 1, &boss);
			if (boss.size() == 1) {
				struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(boss[0]);
				if (entity != NULL)
					Event_CheckFactionWarWinnerDesignation(entity, true);
			}
		}
	}
	if (!winnerName.empty()) {
		PlayerPool_SaveFactionFightInfo(winnerName.c_str(), MapPool_FactionWarNum(), winnerHurt);
	}

	MapPool_ReleaseNPCs(MapPool_FactionWar(), -1, false);
	pool.factionWarOver = true;
}

void MapPool_ClearRoom(int32_t room, bool success) {
	if (room < MAP_START || room >= pool.max)
		return;
	struct Map *map = pool.maps[room];
	if (map == NULL)
		return;

	const MapInfo *info = MapInfoManager_MapInfo(map->info);
	assert(info != NULL);

	if (info->mapType() == MapInfo::HELL) {
		static vector<struct PlayerEntity *> players;
		players.clear();
		int count = PlayerPool_Players(room, NULL, &players);
		for (int i = 0; i < count; i++) {
			Fight_ClearRoom(PlayerEntity_Component(players[i])->fight, success, map->group);
		}
	} else {
		MapPool_ReleaseNPCs(room, -1, false);

		static vector<struct PlayerEntity *> players;
		players.clear();
		int count = PlayerPool_Players(room, &players);
		for (int i = 0; i < count; i++) {
			static NetProto_ClearRoom clearRoom;
			clearRoom.Clear();
			if (Fight_ClearRoom(PlayerEntity_Component(players[i])->fight, success, map->group, &clearRoom) == 0) {
				clearRoom.set_totalTime(map->group);
				int32_t id = PlayerEntity_ID(players[i]);
				GCAgent_SendProtoToClients(&id, 1, &clearRoom);
			}
		}

		const MapUnit *unit = MapInfoManager_MapUnit(info);
		map->group = unit->npcGroups_size();
		if (info->id() == Config_WorldBossMap()) {
			pool.worldBossOver = true;
		} else if (info->id() == Config_FactionBossMap()) {
			for (std::map<string, pair<int32_t, bool> >::iterator it = pool.factionBoss.begin(); it != pool.factionBoss.end(); it++) {
				if (it->second.first == room) {
					it->second.second = true;
					break;
				}
			}
		}
	}
}

int MapPool_GroupCount(int32_t room) {
	if (room < MAP_START || room >= pool.max)
		return 0;
	struct Map *map = pool.maps[room];
	if (map == NULL)
		return 0;
	return map->group;
}

int MapPool_PassCount(int32_t room) {
	if (room < MAP_START || room >= pool.max)
		return 0;
	struct Map *map = pool.maps[room];
	if (map == NULL)
		return 0;
	return map->pass;
}

bool MapPool_IsObstacleOpen(int map, int id)
{
	if (id < 1 || id > 32) {
		DEBUG_LOG("ERROR MapPool_IsObstacleOpen :id  %d is over the max", id);
		return false;
	}
	struct Map *room = pool.maps[map];
	if (room == NULL)
		return false;
	return room->v & (1 << (id - 1));
}

