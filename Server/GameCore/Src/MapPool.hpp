#ifndef _MAP_POOL_HPP_
#define _MAP_POOL_HPP_

#include "Math.hpp"
#include "Movement.hpp"
#include "NPCEntity.hpp"
#include "PlayerEntity.hpp"
#include <sys/types.h>
#include <string>

#define MAP_CREATE 2
#define MAP_CHANGING 0
#define MAP_START 10
#define MAP_BEGINNER 20
//#define MAP_FIRSTROOM 13
#define MAP_FIRSTROOM 20

struct PlayOffUnit{
	int64_t lhs;
	int64_t rhs;
	PB_PlayerAtt lAtt;
	PB_PlayerAtt rAtt;
	std::string lName;
	std::string rName;
	int32_t room;
	int32_t playoff;
	int day;
	int pass;
	int lWin;
	int rWin;
	int overTime;
	int total;
};

void MapPool_Init();
void MapPool_Prepare();

void MapPool_UpdateCoord(const Vector2i *prevCoord, const Vector2i *curCoord, struct Movement *movement);

// -1: ready
// -2: done
// >= 0: doing
int32_t MapPool_WorldBoss();
int MapPool_WorldBossNum();

// -1: ready
// -2: done
// >= 0: doing
int32_t MapPool_FactionWar();
int MapPool_FactionWarNum();

int32_t MapPool_Hell();
bool MapPool_IsHellValid(int32_t room);

struct PlayOffUnit * MapPool_SelectPlayOff(int64_t roleID, int32_t playoff = -1, int day = -1, int pass = -1, int turn = -1, int overTime = -1);

// -1: none
// -2: done
// >= 0: doing
int32_t MapPool_FactionBoss(const std::string &faction);
const char * MapPool_FactionOfBoss(int32_t room);
int32_t MapPool_GenFactionBoss(const std::string &faction);

void MapPool_GenNPC(int32_t mapID, int id, NPCAtt *att, const Vector2i *coord, struct Component *master = NULL, bool notice = false);
int32_t MapPool_Gen(int32_t info, bool multipleRoom, int overTime = 0);

// This api does not clear the movement, you must call MapPool_UpdateCoord(NULL... on all players and npcs in this scene before release the map.
void MapPool_Release(int32_t map);
void MapPool_ReleaseNPCs(int32_t map, int32_t id = -1, bool notice = false);

void MapPool_Link(int32_t room, struct Movement *movement);
void MapPool_Unlink(int32_t room, struct Movement *movement);
int MapPool_Linkers(int32_t room, struct Movement **movements, size_t size);
int MapPool_LinkersCount(int32_t room);

bool MapPool_IsSingle(int32_t map);

int32_t MapPool_MapInfo(int32_t map);

int MapPool_Entities(int32_t mapID, const Vector2i *coord, struct PlayerEntity *self, struct Movement **entities, size_t size);

int MapPool_SearchSquareArea(const Vector2i *center, int radius, int mapID, struct PlayerEntity *self, struct Movement **entities, size_t size, int max);

int MapPool_SearchWithMinDist(const Vector2i *center, int radius, int mapID, struct PlayerEntity *self, struct Movement **entities, size_t size, int max);

void MapPool_ClearRoom(int32_t map, bool success = true);
void MapPool_ClearFactionWar();
int MapPool_GroupCount(int32_t room);
int MapPool_PassCount(int32_t room);

bool MapPool_IsObstacleOpen(int map, int id);

#endif
