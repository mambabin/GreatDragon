#include "VIPInfoManager.hpp"
#include "VIPInfo.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllVIPInfo allVIP;
}package;

void VIPInfoManager_Init() {
	string src = Config_DataPath() + string("/VIP.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.allVIP.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
}

const VIPInfo * VIPInfoManager_VIPInfo(int32_t id) {
	if (id < 0 || id >= package.allVIP.vipInfo_size())
		return NULL;

	const VIPInfo *info = &package.allVIP.vipInfo(id);
	return info->level() == -1 ? NULL : info;
}

int32_t VIPInfoManager_MatchVIP(int64_t totalRMB) {
	for (int i = 1; i < package.allVIP.vipInfo_size(); i++) {
		const VIPInfo *info = &package.allVIP.vipInfo(i);
		if (info->level() == -1)
			continue;
		if (info->rmb() > totalRMB)
			return i - 1;
	}
	return package.allVIP.vipInfo(package.allVIP.vipInfo_size() - 1).level();
}
