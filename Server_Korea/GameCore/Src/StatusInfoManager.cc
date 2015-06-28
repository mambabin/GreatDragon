#include "StatusInfoManager.hpp"
#include "StatusInfo.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllStatuses statuses;
	int size;
}pool;

static void LoadRes() {
	string src = Config_DataPath() + string("/Statuses.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!pool.statuses.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	pool.size = pool.statuses.statuses_size();
}

void StatusInfoManager_Init() {
	LoadRes();
}

const StatusInfo * StatusInfoManager_StatusInfo(int32_t id) {
	if (id < 0 || id >= pool.size)
		return NULL;

	if (pool.statuses.statuses(id).id() == -1)
		return NULL;
	return &pool.statuses.statuses(id);
}
