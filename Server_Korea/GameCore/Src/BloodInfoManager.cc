#include "BloodInfoManager.hpp"
#include "BloodInfo.pb.h"
#include "Time.hpp"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllBloodInfo allBloodInfo;
	AllBloodNodeInfo allBloodNodeInfo;
	AllExploreInfo allExploreInfo;
	int *normalRate;
	int *normalMap;
	int normalCount;
	int *highRate;
	int *highMap;
	int highCount;
}package;

void BloodInfoManager_Init() {
	{
		string src = Config_DataPath() + string("/Blood.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.allBloodInfo.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		string src = Config_DataPath() + string("/BloodNodes.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.allBloodNodeInfo.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		string src = Config_DataPath() + string("/Explore.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.allExploreInfo.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		package.normalCount = 0;
		package.highCount = 0;
		for (int i = 0; i < package.allExploreInfo.exploreInfo_size(); i++) {
			const ExploreInfo *info = &package.allExploreInfo.exploreInfo(i);
			if (info->type() == ExploreInfo::NORMAL)
				package.normalCount++;
			else if (info->type() == ExploreInfo::HIGH)
				package.highCount++;
			else
				assert(0);
		}
		assert(package.normalCount > 0 && package.highCount > 0);
		package.normalRate = new int[package.normalCount];
		package.normalMap = new int[package.normalCount];
		package.highRate = new int[package.highCount];
		package.highMap = new int[package.highCount];
		for (int i = 0, normal = 0, high = 0; i < package.allExploreInfo.exploreInfo_size(); i++) {
			const ExploreInfo *info = &package.allExploreInfo.exploreInfo(i);
			if (info->type() == ExploreInfo::NORMAL) {
				package.normalRate[normal] = info->rate();
				package.normalMap[normal] = i;
				normal++;
			} else if (info->type() == ExploreInfo::HIGH) {
				package.highRate[high] = info->rate();
				package.highMap[high] = i;
				high++;
			} else {
				assert(0);
			}
		}
	}
}

const BloodInfo * BloodInfoManager_BloodInfo(int32_t id) {
	if (id < 0 || id >= package.allBloodInfo.bloodInfo_size())
		return NULL;

	const BloodInfo *info = &package.allBloodInfo.bloodInfo(id);
	return info->level() == -1 ? NULL : info;
}

const BloodNodeInfo * BloodInfoManager_BloodNodeInfo(int32_t id) {
	if (id < 0 || id >= package.allBloodNodeInfo.bloodNodeInfo_size())
		return NULL;

	const BloodNodeInfo *info = &package.allBloodNodeInfo.bloodNodeInfo(id);
	return info->id() == -1 ? NULL : info;
}

const ExploreInfo * BloodInfoManager_ExploreInfo(int32_t id) {
	if (id < 0 || id >= package.allExploreInfo.exploreInfo_size())
		return NULL;

	const ExploreInfo *info = &package.allExploreInfo.exploreInfo(id);
	return info->id() == -1 ? NULL : info;
}

int32_t BloodInfoManager_RandomExploreInfo(ExploreInfo::Type type) {
	if (type == ExploreInfo::NORMAL)
		return package.normalMap[Time_RandomSelect(package.normalRate, package.normalCount)];
	else if (type == ExploreInfo::HIGH)
		return package.highMap[Time_RandomSelect(package.highRate, package.highCount)];
	return -1;
}

int BloodInfoManager_MaxBloodLevel() {
	return package.allBloodInfo.bloodInfo_size();
}

int BloodInfoManager_MaxBloodNode() {
	return package.allBloodNodeInfo.bloodNodeInfo_size();
}

int BloodInfoManager_MaxExplore() {
	return package.allExploreInfo.exploreInfo_size();
}
