#ifndef _GLOBAL_MISSION_MANAGER_HPP_
#define _GLOBAL_MISSION_MANAGER_HPP_

#include "Movement.hpp"
#include "MissionInfo.hpp"
#include <sys/types.h>

void GlobalMissionManager_Init();

void GlobalMissionManager_Add(const MissionInfo *missionInfo);

void GlobalMissionManager_ApplySceneMission(struct Movement *movement);
void GlobalMissionManager_ApplyLoginMission(struct Movement *movement);
void GlobalMissionManager_ApplyDailyMission(struct Movement *movement);

#endif
