#include "AwardInfoManager.hpp"
#include "Award.pb.h"
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
	vector<AllAwardInfo> awards;
}package;

void AwardInfoManager_Init() {
	string src = Config_DataPath() + string("/Award.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	AllAwardInfo allAwardInfo;
	if (!allAwardInfo.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	package.awards.resize(AwardInfo::Type_ARRAYSIZE);
	for (int i = 0; i < allAwardInfo.awardInfo_size(); i++) {
		*package.awards[(int)allAwardInfo.awardInfo(i).type()].add_awardInfo() = allAwardInfo.awardInfo(i);
	}
}

const AwardInfo * AwardInfoManager_AwardInfo(AwardInfo::Type type, int32_t index) {
	if (!AwardInfo::Type_IsValid(type))
		return NULL;

	const AllAwardInfo *all = &package.awards[(int)type];
	if (index < 0 || index >= all->awardInfo_size())
		return NULL;

	const AwardInfo *info = &all->awardInfo(index);
	return info->id() == -1 ? NULL : info;
}

const AwardInfo * AwardInfoManager_AwardInfo(AwardInfo::Type type, int32_t level, int32_t* index) {
	if (!AwardInfo::Type_IsValid(type))
		return NULL;

	const AllAwardInfo *all = &package.awards[(int)type];
	for (int i = 0; i < all->awardInfo_size(); ++i) {
		if (level == all->awardInfo(i).arg()) {
			*index = i;
			const AwardInfo *info = &all->awardInfo(i);
			return info->id() == -1 ? NULL : info;
		}
	}
	
	return NULL;
}

const AwardInfo * AwardInfoManager_AwardInfo(AwardInfo::Type type, int32_t arg, int32_t index) {
	if (!AwardInfo::Type_IsValid(type))
		return NULL;

	const AllAwardInfo *all = &package.awards[(int)type];
	int n = 0;
	for (int i = 0; i < all->awardInfo_size(); ++i) {
		const AwardInfo *info = &all->awardInfo(i);
		if (info->arg() == arg) {
			if (n++ == index) {
				return info->id() == -1 ? NULL : info;
			}
		}
	}
	return NULL;
}

const vector<const AwardInfo *> *  AwardInfoManager_AllAwardInfo(AwardInfo::Type type, int32_t argBegin, int32_t argDelta) {
	if (!AwardInfo::Type_IsValid(type))
		return NULL;

	static vector<const AwardInfo *> vecAwardInfo;
	vecAwardInfo.clear();

	const AllAwardInfo *all = &package.awards[(int)type];

	for (int i = 0; i < all->awardInfo_size(); ++i) {
		if (all->awardInfo(i).arg() > argBegin && all->awardInfo(i).arg() <= argBegin + argDelta) {
			const AwardInfo *info = &all->awardInfo(i);
			if (info->id() != -1) {
				vecAwardInfo.push_back(info);
			}
		}
	}
	return &vecAwardInfo;
}



