#include "SkillInfoManager.hpp"
#include "SkillInfo.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllSkills skills;
	vector<int> size;
}pool;

static void LoadRes() {
	string src = Config_DataPath() + string("/Skills.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!pool.skills.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	pool.size.resize(pool.skills.skills_size(), 0);
	for (int i = 0; i < pool.skills.skills_size(); i++) {
		pool.size[i] = pool.skills.skills(i).skills_size();
	}
}

void SkillInfoManager_Init() {
	LoadRes();
}

const SkillInfo * SkillInfoManager_SkillInfo(int32_t id, int level) {
	if (id < 0 || id >= (int32_t)pool.size.size())
		return NULL;
	if (level < 0 || level >= pool.size[id])
		return NULL;

	const SkillFamily *family = &pool.skills.skills(id);
	return &family->skills(level);
}

int SkillInfoManager_MaxLevel(int32_t id) {
	if (id < 0 || id >= (int32_t)pool.size.size())
		return -1;

	return pool.size[id] <= 0 ? -1 : pool.size[id] - 1;
}
