#include "BoxInfoManager.hpp"
#include "BoxInfo.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllBoxes allBoxes;
}package;

void BoxInfoManager_Init() {
	string src = Config_DataPath() + string("/Boxes.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.allBoxes.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
	DEBUG_LOG("boxsize:%d", package.allBoxes.boxes_size());
	const BoxInfo *info = &package.allBoxes.boxes(1013);
	DEBUG_LOG("infoid:%d", info->id());
}

const BoxInfo * BoxInfoManager_BoxInfo(int32_t id) {
	if (id < 0 || id >= package.allBoxes.boxes_size())
		return NULL;

	const BoxInfo *info = &package.allBoxes.boxes(id);
	return info->id() == -1 ? NULL : info;
}
