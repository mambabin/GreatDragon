#ifndef _MISSION_INFO_MANAGER_HPP_
#define _MISSION_INFO_MANAGER_HPP_

#include "MissionInfo.hpp"
#include <sys/types.h>

void MissionInfoManager_Init();

void MissionInfoManager_Prepare();

const MissionInfo * MissionInfoManager_MissionInfo(int32_t id);

#endif
