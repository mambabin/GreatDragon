#ifndef _MAP_INFO_MANAGER_HPP_
#define _MAP_INFO_MANAGER_HPP_

#include "MapInfo.pb.h"
#include "Math.hpp"
#include "Movement.hpp"
#include "Mission.hpp"
#include <vector>
#include <sys/types.h>

#define MAP_BLOCKSIZE 0.5f

void MapInfoManager_Init();
void MapInfoManager_Prepare();

void MapInfoManager_SetSingleRecord(const google::protobuf::RepeatedPtrField<RecordInfo> *record);
void MapInfoManager_UpdateSingleRecord(int32_t map, int64_t score, struct PlayerEntity *player);
const RecordInfo * MapInfoManager_SingleRecord(int32_t map);

// Generally you should convert your map id to info id using MapPool_MapInfo.
const MapInfo * MapInfoManager_MapInfo(int32_t id);
const MapUnit * MapInfoManager_MapUnit(const MapInfo *info);
const BlockInfo * MapInfoManager_BlockInfo(const MapInfo *mapInfo);

const std::vector<int32_t> * MapInfoManager_ChapterMap(int chapter);

void MapInfoManager_SetupNPCMission(const MissionInfo *info);

int MapInfoManager_LogicByReal(float v);
float MapInfoManager_RealByLogic(int v);

bool MapInfoManager_IsInScene(const MapInfo *mapInfo, const Vector2i *logicCoord);
void MapInfoManager_LogicCoordByPosition(const Vector3f *position, Vector2i *logicCoord);
void MapInfoManager_PositionByLogicCoord(const Vector2i *logicCoord, Vector3f *position);

BlockInfo::BlockType MapInfoManager_BlockType(const MapInfo *mapInfo, const Vector2i *logicCoord);
int MapInfoManager_Obstacle(const MapInfo *mapInfo, const Vector2i *logicCoord);
bool MapInfoManager_Unwalkable(const MapInfo *mapInfo, const Vector2i *logicCoord, int32_t map);
bool MapInfoManager_Unskill(const MapInfo *mapInfo, const Vector2i *logicCoord);

// Generally you should convert your map id to info id using MapPool_MapInfo.
// -1: Failure.
// >= 0: Next map.
int32_t MapInfoManager_Next(int32_t mapID, int32_t jump, Vector2i *coord);

bool MapInfoManager_EnterCoord(const MapInfo *mapInfo, int32_t jumpPoint, Vector2i *coord);
int MapInfoManager_RandomEnterCoord(const MapInfo *mapInfo);

const JumpPointInfo * MapInfoManager_FirstEnter(const MapInfo *mapInfo);

#endif
