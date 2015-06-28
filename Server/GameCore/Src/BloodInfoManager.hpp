#ifndef _BLOOD_INFO_MANAGER_HPP_
#define _BLOOD_INFO_MANAGER_HPP_

#include "BloodInfo.pb.h"
#include <sys/types.h>

void BloodInfoManager_Init();

const BloodInfo * BloodInfoManager_BloodInfo(int32_t id);
const BloodNodeInfo * BloodInfoManager_BloodNodeInfo(int32_t id);
const ExploreInfo * BloodInfoManager_ExploreInfo(int32_t id);
int32_t BloodInfoManager_RandomExploreInfo(ExploreInfo::Type type);

int BloodInfoManager_MaxBloodLevel();
int BloodInfoManager_MaxBloodNode();
int BloodInfoManager_MaxExplore();

#endif
