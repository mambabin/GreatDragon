#include "GodShipInfoManager.hpp"
#include "GodShip.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllGodShips allGodShips;
}package;

void GodShipInfoManager_Init() {
	string src = Config_DataPath() + string("/GodShip.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.allGodShips.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
}

const GodShip * GodShipInfoManager_GodShipInfo(int32_t id) {
	// MUST BE THREAD SAFETY
	DEBUG_LOG("size %d", package.allGodShips.godShips_size());
	if (id < 0 || id >= package.allGodShips.godShips_size())
		return NULL;

	const GodShip *info = &package.allGodShips.godShips(id);
	return info->id() == -1 ? NULL : info;
}

