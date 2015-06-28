#include "MapInfoManager.hpp"
#include "Config.hpp"
#include "MapInfo.pb.h"
#include "Math.hpp"
#include "MathUtil.hpp"
#include "Movement.hpp"
#include "MovementInfo.hpp"
#include "MapPool.hpp"
#include "NPCInfoManager.hpp"
#include "GCAgent.hpp"
#include "Time.hpp"
#include "Debug.hpp"
#include "DCProto.pb.h"
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <set>
#include <string>

using namespace std;

static struct{
	vector<MapUnit> units;
	AllMapInfo maps;
	vector<BlockInfo> blocks;
	google::protobuf::RepeatedPtrField<RecordInfo> singleRecord;
	map<int, vector<int32_t> > chapterMap;
}pool;

static void GenMapUnit(int id) {
	{
		char src[128];
		SNPRINTF1(src, "%s/MapInfo/MapUnit-%d.bytes", Config_DataPath(), id);

		fstream in(src, ios_base::in | ios_base::binary);
		if (!in.fail()) {
			MapUnit *info = &pool.units[id];
			if (!info->ParseFromIstream(&in)) {
				DEBUG_LOGERROR("Failed to parse %s", src);
				exit(EXIT_FAILURE);
			}
		}
	}
	{
		char src[128];
		SNPRINTF1(src, "%s/MapInfo/BlockInfo-%d.bytes", Config_DataPath(), id);

		fstream in(src, ios_base::in | ios_base::binary);
		if (!in.fail()) {
			BlockInfo *info = &pool.blocks[id];
			if (!info->ParseFromIstream(&in)) {
				DEBUG_LOGERROR("Failed to parse %s", src);
				exit(EXIT_FAILURE);
			}
		}
	}
}

static void InitJumpTable() {
	JumpPointInfo *jumpPoint = pool.units[MAP_CREATE].mutable_jumpTable()->add_table();
	jumpPoint->set_nextMap(MAP_BEGINNER);
	jumpPoint->set_nextPoint(0);
}

static void InitNormalMaps() {
	for (int i = MAP_START; i < pool.maps.mapInfo_size(); i++) {
		const MapInfo *mapInfo = &pool.maps.mapInfo(i);
		if (mapInfo->id() == -1)
			continue;

		if (mapInfo->mapType() == MapInfo::PEACE
				|| mapInfo->mapType() == MapInfo::SINGLE
				|| mapInfo->mapType() == MapInfo::ROOM) {
			int32_t ret = MapPool_Gen(mapInfo->id(), false);
			assert(ret == mapInfo->id());
		}
	}
}

void MapInfoManager_Init() {
	MapUnit mapUnit;
	mapUnit.set_id(-1);
	pool.units.resize(Config_MaxMaps(), mapUnit);
	BlockInfo blockInfo;
	blockInfo.set_id(-1);
	pool.blocks.resize(Config_MaxMaps(), blockInfo);

	for (size_t i = MAP_START; i < pool.units.size(); i++)
		GenMapUnit(i);

	string src = Config_DataPath() + string("/MapInfo.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}
	if (!pool.maps.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < pool.maps.mapInfo_size(); i++) {
		const MapInfo *info = MapInfoManager_MapInfo(i);
		if (info != NULL) {
			if ((info->id() >= Config_SingleBegin() && info->id() <= Config_SingleEnd())
					|| (info->id() >= Config_SingleEnhanceBegin() && info->id() <= Config_SingleEnhanceEnd())) {
				pool.chapterMap[info->parent()].push_back(info->id());
			}
		}
	}
	for (int i = 0; i < pool.maps.mapInfo_size(); i++) {
		const MapInfo *info = MapInfoManager_MapInfo(i);
		if (info != NULL) {
			const MapUnit *unit = MapInfoManager_MapUnit(info);
			if (unit == NULL) {
				DEBUG_LOGERROR("map unit is none, id: %d", info->id());
				assert(unit != NULL);
			}
		}
	}

	InitJumpTable();

	pool.singleRecord.Clear();
}

void MapInfoManager_Prepare() {
	InitNormalMaps();
}

void MapInfoManager_SetSingleRecord(const google::protobuf::RepeatedPtrField<RecordInfo> *record) {
	if (record == NULL)
		return;
	pool.singleRecord = *record;

	DEBUG_LOG("SINGLE RECORD");
	for (int i = 0; i < record->size(); i++) {
		const RecordInfo *info = &record->Get(i);
		if (info->role().roleID() == -1)
			continue;
		DEBUG_LOG("mapID: %d, roleID: %lld, name: %s, value: %lld", i, info->role().roleID(), info->role().name().c_str(), (long long)info->arg1());
	}
	DEBUG_LOG("-----------------------------");
}

void MapInfoManager_UpdateSingleRecord(int32_t map, int64_t score, struct PlayerEntity *player) {
	if (map < 0 || !PlayerEntity_IsValid(player))
		return;

	for (int i = pool.singleRecord.size(); i <= map; i++)
		pool.singleRecord.Add()->mutable_role()->set_roleID(-1);

	if (score > pool.singleRecord.Get(map).arg1()) {
		RecordInfo *recordInfo = pool.singleRecord.Mutable(map);
		recordInfo->mutable_role()->set_roleID(PlayerEntity_RoleID(player));
		struct Component *component = PlayerEntity_Component(player);
		recordInfo->mutable_role()->set_name(component->baseAtt->name());
		recordInfo->mutable_role()->set_professionType((PB_ProfessionInfo::Type)component->baseAtt->professionType());
		recordInfo->set_arg1(score);

		DCProto_SaveSingleRecord ssr;
		ssr.set_mapID(map);
		*ssr.mutable_record() = *recordInfo;
		GCAgent_SendProtoToDCAgent(&ssr);
	}
}

const RecordInfo * MapInfoManager_SingleRecord(int32_t map) {
	if (map < 0 || map >= pool.singleRecord.size())
		return NULL;

	const RecordInfo *ret = &pool.singleRecord.Get(map);
	return ret->role().roleID() == -1 ? NULL : ret;
}

void MapInfoManager_SetupNPCMission(const MissionInfo *info) {
	if (info == NULL || info->id() == -1)
		return;

	if (info->type() == MissionInfo::MAJOR) {
		const MissionPort *out = &info->out();
		switch(out->type()) {
			case MissionPort::NPC:
				{
					int32_t map = out->arg(0);
					int32_t npc = out->arg(1);
					const MapInfo *mapInfo = MapInfoManager_MapInfo(map);
					if (mapInfo == NULL)
						DEBUG_LOGERROR("Failed to setup npc mission, map: %d, npc: %d", map, npc);
					assert(mapInfo != NULL);
					const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);

					for (int i = 0; i < mapUnit->npcs_size(); i++) {
						const NPCSceneUnit *unit = &mapUnit->npcs(i);
						if (npc == unit->id()) {
							NPCInfoManager_SetupNPCMission(unit->resID(), info->id());
							break;
						}
					}
				}
				break;
			default:
				break;
		}
	}
}

const MapInfo * MapInfoManager_MapInfo(int32_t id) {
	if (id < 0 || id >= pool.maps.mapInfo_size())
		return NULL;

	const MapInfo *ret = &pool.maps.mapInfo(id);
	if (ret->id() == -1)
		return NULL;

	return ret;
}

const MapUnit * MapInfoManager_MapUnit(const MapInfo *info) {
	if (info == NULL)
		return NULL;
	int32_t id = info->id();
	if (id < 0 || id >= (int32_t)pool.units.size())
		return NULL;

	MapUnit *ret = &pool.units[id];
	if (ret->id() == -1)
		return NULL;

	return ret;
}

const BlockInfo *MapInfoManager_BlockInfo(const MapInfo *mapInfo) {
	if (mapInfo == NULL)
		return NULL;
	const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
	int32_t id = unit->resID();
	if (id < 0 || id >= (int32_t)pool.blocks.size())
		return NULL;

	BlockInfo *ret = &pool.blocks[id];
	if (ret->id() == -1)
		return NULL;

	return ret;
}

const std::vector<int32_t> * MapInfoManager_ChapterMap(int chapter) {
	map<int, vector<int32_t> >::iterator it = pool.chapterMap.find(chapter);
	return it == pool.chapterMap.end() ? NULL : &it->second;
}

static inline int Extra(float v) {
	return (int)(v - (int)v + 0.5f);
}

int MapInfoManager_LogicByReal(float v) {
	return (int)v * 2 + Extra(v);
}

float MapInfoManager_RealByLogic(int v) {
	return v / 2 + 0.25f + 0.5f * (v % 2);
}

bool MapInfoManager_IsInScene(const MapInfo *mapInfo, const Vector2i *logicCoord) {
	if (mapInfo == NULL || logicCoord == NULL)
		return false;
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	if (logicCoord->x() < 0 || logicCoord->x() >= blockInfo->logicWidth()
			|| logicCoord->y() < 0 || logicCoord->y() >= blockInfo->logicLength())
		return false;
	return true;
}

void MapInfoManager_LogicCoordByPosition(const Vector3f *position, Vector2i *logicCoord) {
	logicCoord->set_x((int)position->x() * 2 + Extra(position->x()));
	logicCoord->set_y((int)position->z() * 2 + Extra(position->z()));
}

void MapInfoManager_PositionByLogicCoord(const Vector2i *logicCoord, Vector3f *position) {
	position->set_x(logicCoord->x() / 2 + 0.25f + 0.5f * (logicCoord->x() % 2));
	position->set_z(logicCoord->y() / 2 + 0.25f + 0.5f * (logicCoord->y() % 2));
}

/*
   static void LogicCoordByIndex(const MapInfo *mapInfo, int index, Vector2i *logicCoord) {
   const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
   assert(blockInfo != NULL);
   logicCoord->set_x(index % blockInfo->logicWidth());
   logicCoord->set_y(index / blockInfo->logicWidth());
   }
   */

static int IndexByLogicCoord(const MapInfo *mapInfo, const Vector2i *logicCoord) {
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	return logicCoord->y() * blockInfo->logicWidth() + logicCoord->x();
}

BlockInfo::BlockType MapInfoManager_BlockType(const MapInfo *mapInfo, const Vector2i *logicCoord) {
	if (mapInfo == NULL || logicCoord == NULL)
		return BlockInfo::UNWALKABLE_UNSKILL;
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	if (logicCoord->x() < 0 || logicCoord->x() >= blockInfo->logicWidth()
			|| logicCoord->y() < 0 || logicCoord->y() >= blockInfo->logicLength())
		return BlockInfo::UNWALKABLE_UNSKILL;

	int index = IndexByLogicCoord(mapInfo, logicCoord);
	return blockInfo->blockTypes(index);
}

int MapInfoManager_Obstacle(const MapInfo *mapInfo, const Vector2i *logicCoord) {
	if (mapInfo == NULL || logicCoord == NULL)
		return 0;
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	if (blockInfo->obstacles_size() <= 0)
		return 0;
	if (logicCoord->x() < 0 || logicCoord->x() >= blockInfo->logicWidth()
			|| logicCoord->y() < 0 || logicCoord->y() >= blockInfo->logicLength())
		return 0;

	int index = IndexByLogicCoord(mapInfo, logicCoord);
	return blockInfo->obstacles(index);
}

bool MapInfoManager_Unwalkable(const MapInfo *mapInfo, const Vector2i *logicCoord, int32_t map) {
	if (mapInfo == NULL || logicCoord == NULL)
		return true;
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	if (logicCoord->x() < 0 || logicCoord->x() >= blockInfo->logicWidth()
			|| logicCoord->y() < 0 || logicCoord->y() >= blockInfo->logicLength())
		return true;

	int index = IndexByLogicCoord(mapInfo, logicCoord);
	bool ret = blockInfo->blockTypes(index) == BlockInfo::UNWALKABLE
		|| blockInfo->blockTypes(index) == BlockInfo::UNWALKABLE_UNSKILL;
	if (ret)
		return ret;

	int obstacle = MapInfoManager_Obstacle(mapInfo, logicCoord);
	if (obstacle > 0)
		return MapPool_IsObstacleOpen(map, obstacle);
	return false;
}

bool MapInfoManager_Unskill(const MapInfo *mapInfo, const Vector2i *logicCoord) {
	if (mapInfo == NULL || logicCoord == NULL)
		return true;
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);
	if (logicCoord->x() < 0 || logicCoord->x() >= blockInfo->logicWidth()
			|| logicCoord->y() < 0 || logicCoord->y() >= blockInfo->logicLength())
		return true;

	int index = IndexByLogicCoord(mapInfo, logicCoord);
	return blockInfo->blockTypes(index) == BlockInfo::UNWALKABLE_UNSKILL;
}

int32_t MapInfoManager_Next(int32_t mapID, int32_t jump, Vector2i *coord) {
	if (mapID < 0 || mapID >= (int32_t)pool.units.size() || coord == NULL)
		return -1;

	const JumpTableInfo *jumpTable = &pool.units[mapID].jumpTable();
	if (jump < 0 || jump >= jumpTable->table_size())
		return -1;

	const JumpPointInfo *point = &jumpTable->table(jump);
	if (point->nextMap() == -1)
		return -1;

	const JumpPointInfo *next = &pool.units[point->nextMap()].jumpTable().table(point->nextPoint());
	coord->FromPB(&next->enterCoord());

	return point->nextMap();
}

bool MapInfoManager_EnterCoord(const MapInfo *mapInfo, int32_t jumpPoint, Vector2i *coord) {
	if (mapInfo == NULL || coord == NULL) {
		return false;
	}
	const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
	const JumpTableInfo *table = &unit->jumpTable();;
	if (jumpPoint < 0 || jumpPoint >= table->table_size())
		return false;

	const JumpPointInfo *point = &table->table(jumpPoint);
	coord->FromPB(&point->enterCoord());

	return true;
}

int MapInfoManager_RandomEnterCoord(const MapInfo *mapInfo) {
	if (mapInfo == NULL)
		return -1;

	const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
	const JumpTableInfo *table = &unit->jumpTable();;
	if (table->table_size() <= 0)
		return -1;

	return Time_Random(0, table->table_size());
}

const JumpPointInfo * MapInfoManager_FirstEnter(const MapInfo *mapInfo) {
	if (mapInfo == NULL)
		return NULL;

	const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
	const JumpTableInfo *table = &unit->jumpTable();
	for (int i = 0; i < table->table_size(); i++) {
		if (table->table(i).nextMap() == -1)
			return &table->table(i);
	}

	return NULL;
}
