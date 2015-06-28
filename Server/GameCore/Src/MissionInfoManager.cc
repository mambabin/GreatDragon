#include "MissionInfoManager.hpp"
#include "MissionInfo.hpp"
#include "GlobalMissionManager.hpp"
#include "MapInfoManager.hpp"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllMissions pool;
	int size;
}package;

void MissionInfoManager_Init() {
	string src = Config_DataPath() + string("/Missions.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.pool.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	package.size = package.pool.missions_size();
}

void MissionInfoManager_Prepare() {
	for (int i = 0; i < package.size; i++) {
		const MissionInfo *info = &package.pool.missions(i);
		if (info->id() == -1)
			continue;

		GlobalMissionManager_Add(info);
		MapInfoManager_SetupNPCMission(info);
	}
}

const MissionInfo * MissionInfoManager_MissionInfo(int32_t id) {
	if (id < 0 || id >= package.size)
		return NULL;

	if (package.pool.missions(id).id() == -1)
		return NULL;
	return &package.pool.missions(id);
}
