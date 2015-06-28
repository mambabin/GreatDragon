#include "FashionInfoManager.hpp"
#include "Fashion.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllFashions allFashions;
}package;

void FashionInfoManager_Init() {
	string src = Config_DataPath() + string("/Fashion.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.allFashions.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
}

const FashionInfo * FashionInfoManager_FashionInfo(int32_t id) {
	// MUST BE THREAD SAFETY
	if (id < 0 || id >= package.allFashions.fashions_size())
		return NULL;

	const FashionInfo *info = &package.allFashions.fashions(id);
	return info->id() == -1 ? NULL : info;
}

