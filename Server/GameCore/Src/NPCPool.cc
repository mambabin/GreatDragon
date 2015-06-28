#include "NPCPool.hpp"
#include "Config.hpp"
#include "RoleAtt.hpp"
#include "NPCEntity.hpp"
#include "MapInfo.pb.h"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "ProfessionInfo.hpp"
#include "MovementInfo.hpp"
#include "EquipmentInfo.hpp"
#include "FightInfo.hpp"
#include "MathUtil.hpp"
#include "Debug.hpp"
#include "Time.hpp"
#include "Timer.hpp"
#include <sys/types.h>
#include <vector>
#include <fstream>

using namespace std;

static struct{
	int maxMaps;
	vector<vector<struct NPCEntity *> > ids;
	vector<int> count;
	int totalCount;
}pool;


void NPCPool_Init() {
	pool.maxMaps = Config_MaxMaps();
	pool.ids.resize(pool.maxMaps);
	pool.count.resize(pool.maxMaps, 0);
	pool.totalCount = 0;
}

static void UpdateNPC(void *arg) {
	for (size_t i = MAP_START; i < pool.ids.size(); i++) {
		vector<struct NPCEntity *> &container = pool.ids[i];
		for (size_t j = 0; j < container.size(); j++) {
			if (container[j] == NULL)
				continue;
			Component_Update(NPCEntity_Component(container[j]));
		}
	}
}

void NPCPool_Prepare() {
	Timer_AddEvent(0, 0, UpdateNPC, NULL, "UpdateNPC");
}

int NPCPool_Add(struct NPCEntity *entity) {
	if (entity == NULL)
		return -1;

	int32_t mapID = NPCEntity_Att(entity)->att().movementAtt().mapID();
	if (mapID < 0 || mapID >= (int32_t)pool.ids.size())
		return -1;

	int32_t id = NPCEntity_ID(entity);
	if (id < 0)
		return -1;

	if (id >= (int32_t)pool.ids[mapID].size())
		pool.ids[mapID].resize(id + 1, NULL);

	if (pool.ids[mapID][id] != NULL)
		return -1;

	pool.ids[mapID][id] = entity;
	pool.count[mapID]++;
	pool.totalCount++;

	return 0;
}

void NPCPool_Del(struct NPCEntity *entity) {
	if (entity == NULL) {
		return;
	}

	int32_t mapID = NPCEntity_Att(entity)->att().movementAtt().mapID();
	if (mapID < 0 || mapID >= (int32_t)pool.ids.size()) {
		return;
	}

	int32_t id = NPCEntity_ID(entity);
	if (id < 0 || id >= (int32_t)pool.ids[mapID].size()) {
		return;
	}

	if (pool.ids[mapID][id] == NULL)
		return;
	pool.ids[mapID][id] = NULL;
	pool.count[mapID]--;
	assert(pool.count[mapID] >= 0);
	pool.totalCount--;
}

struct NPCEntity * NPCPool_NPC(int32_t mapID, int32_t id) {
	if (mapID < 0 || mapID >= (int32_t)pool.ids.size())
		return NULL;
	if (id < 0 || id >= (int32_t)pool.ids[mapID].size())
		return NULL;

	return pool.ids[mapID][id];
}

int NPCPool_Count(int32_t mapID) {
	if (mapID < 0 || mapID >= (int32_t)pool.ids.size())
		return -1;

	return pool.count[mapID];
}

int NPCPool_MonsterCount(int32_t mapID) {
	struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
	int total = NPCPool_NPCs(mapID, npcs, CONFIG_FIXEDARRAY);
	int count = 0;
	for (int i = 0; i < total; i++) {
		if (NPCEntity_Master(npcs[i]) == NULL) {
			struct Component *component = NPCEntity_Component(npcs[i]);
			if (component == NULL)
				continue;
			if (component->roleAtt->fightAtt().selfFaction() != FRIENDLY_NPC_FACTION)
				count++;
		}
	}
	return count;
}

int NPCPool_Pets(int32_t mapID, struct NPCEntity **npcs, size_t size) {
	if (mapID < 0 || mapID >= (int32_t)pool.ids.size() || npcs == NULL)
		return -1;
	if (pool.ids[mapID].size() > size)
		return -1;

	vector<struct NPCEntity *> &container = pool.ids[mapID];
	int count = 0;
	for (size_t i = 0; i < container.size(); i++) {
		if (container[i] == NULL)
			continue;
		if (NPCEntity_Master(container[i]) == NULL)
			continue;
		npcs[count++] = container[i];
	}

	return count;
}

int NPCPool_Monsters(int32_t mapID, struct NPCEntity **npcs, size_t size) {
	if (mapID < 0 || mapID >= (int32_t)pool.ids.size() || npcs == NULL)
		return -1;
	if (pool.ids[mapID].size() > size)
		return -1;

	vector<struct NPCEntity *> &container = pool.ids[mapID];
	int count = 0;
	for (size_t i = 0; i < container.size(); i++) {
		if (container[i] == NULL)
			continue;
		if (NPCEntity_Master(container[i]) != NULL)
			continue;
		npcs[count++] = container[i];
	}

	return count;
}

struct NPCEntity * NPCPool_Pet(int32_t mapID, struct Component *master) {
	if (master == NULL)
		return NULL;

	if (mapID < 0 || mapID >= (int32_t)pool.ids.size())
		return NULL;

	vector<struct NPCEntity *> &container = pool.ids[mapID];
	for (size_t i = 0; i < container.size(); i++) {
		if (container[i] == NULL)
			continue;
		if (NPCEntity_Master(container[i]) == master)
			return container[i];
	}
	return NULL;
}

int NPCPool_PetCount(int32_t mapID) {
	struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
	int total = NPCPool_NPCs(mapID, npcs, CONFIG_FIXEDARRAY);
	int count = 0;
	for (int i = 0; i < total; i++) {
		if (NPCEntity_Master(npcs[i]) != NULL)
			count++;
	}
	return count;
}

int NPCPool_TotalCount() {
	return pool.totalCount;
}

int NPCPool_NPCs(int32_t mapID, struct NPCEntity **npcs, size_t size) {
	if (mapID < 0 || mapID >= (int32_t)pool.ids.size() || npcs == NULL)
		return -1;
	if (pool.ids[mapID].size() > size)
		return -1;

	vector<struct NPCEntity *> &container = pool.ids[mapID];
	int count = 0;
	for (size_t i = 0; i < container.size(); i++) {
		if (container[i] == NULL)
			continue;
		npcs[count++] = container[i];
	}

	return count;
}

int NPCPool_FreeFaction(int32_t mapID) {
	struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
	int count = NPCPool_NPCs(mapID, npcs, CONFIG_FIXEDARRAY);
	int faction = 0;
	for (int i = 0; i < count; i++) {
		struct Component *component = NPCEntity_Component(npcs[i]);
		if (component != NULL)
			faction |= component->roleAtt->fightAtt().selfFaction();
	}
	for (int i = 29; i >= 0; i--) {
		if ((faction & (1 << i)) == 0)
			return 1 << i;
	}
	return 1 << 29;
}

int32_t NPCPool_FreeID(int32_t mapID) {
	struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
	int count = NPCPool_NPCs(mapID, npcs, CONFIG_FIXEDARRAY);

	int32_t ids[CONFIG_FIXEDARRAY];
	for (int i = 0; i < count; i++) 
		ids[i] = NPCEntity_ID(npcs[i]);

	for (int i = 0; ; i++) {
		bool find = false;
		for (int j = 0; j < count; j++) {
			if (ids[j] == i) {
				find = true;
				break;
			}
		}
		if (!find)
			return i;
	}
	return 0;
}
