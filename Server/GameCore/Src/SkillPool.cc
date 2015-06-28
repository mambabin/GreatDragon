#include "SkillPool.hpp"
#include "Config.hpp"
#include "SkillEntity.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "Timer.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <vector>
#include <set>

using namespace std;

static struct{
	int maxMaps;
	vector<set<struct SkillEntity *> > pool;
}package;


void SkillPool_Init() {
	package.maxMaps = Config_MaxMaps();
	package.pool.resize(package.maxMaps);
}

static int Skills(int32_t map, struct SkillEntity *skills[], size_t size) {
	set<struct SkillEntity *> &container = package.pool[map];
	if (container.size() > size)
		return -1;

	int count = 0;
	for (set<struct SkillEntity *>::iterator it = container.begin(); it != container.end(); it++)
		skills[count++] = *it;

	return count;
}

static void UpdateSkill(void *arg) {
	for (size_t i = MAP_START; i < package.pool.size(); i++) {
		struct SkillEntity *skills[CONFIG_FIXEDARRAY];
		int count = Skills(i, skills, CONFIG_FIXEDARRAY);
		assert(count != -1);

		for (int j = 0; j < count; j++) {
			SkillEntity_Update(skills[j]);
		}
	}
}

void SkillPool_Prepare() {
	Timer_AddEvent(0, 0, UpdateSkill, NULL, "UpdateSkill");
}

void SkillPool_Add(struct SkillEntity *entity) {
	if (entity == NULL)
		return;

	int32_t map = SkillEntity_MapID(entity);
	if (map < 0 || map >= (int32_t)package.pool.size())
		return;

	const pair<set<struct SkillEntity *>::iterator, bool> &ret = package.pool[map].insert(entity);
	assert(ret.second);
}

void SkillPool_Del(struct SkillEntity *entity) {
	if (entity == NULL)
		return;

	int32_t mapID = SkillEntity_MapID(entity);
	if (mapID < 0 || mapID >= (int32_t)package.pool.size())
		return;

	size_t ret = package.pool[mapID].erase(entity);
	assert(ret == 1);
}

