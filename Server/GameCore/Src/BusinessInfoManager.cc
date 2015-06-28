#include "BusinessInfoManager.hpp"
#include "BusinessInfo.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllBusiness pool;
	int size;
}package;

void BusinessInfoManager_Init() {
	string src = Config_DataPath() + string("/Business.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	AllBusiness all;
	if (!all.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
	package.pool = all;
	package.size = all.business_size();
}

void BusinessInfoManager_Reload() {
	package.pool.Clear();
	package.size = 0;
	BusinessInfoManager_Init();
}

const BusinessInfo * BusinessInfoManager_BusinessInfo(int32_t id) {
	if (id < 0 || id >= package.size)
		return NULL;

	if (package.pool.business(id).id() == -1)
		return NULL;
	return &package.pool.business(id);
}
