#ifndef _NPC_INFO_MANAGER_HPP_
#define _NPC_INFO_MANAGER_HPP_

#include "PetHalo.pb.h"
#include "NPCAtt.hpp"
#include <sys/types.h>
#include <map>

void NPCInfoManager_Init();

const NPCAtt * NPCInfoManager_NPC(int32_t id);
const NPCAtt * NPCInfoManager_Pet(int32_t id, int32_t quality, int32_t level);

void NPCInfoManager_SetupNPCMission(int32_t id, int32_t mission);

const std::map<int, std::map<int, PB_PetHaloInfo> >* NPCInfoManager_PetHaloInfo();
const PB_PetHaloInfo* NPCInfoManager_PetHaloInfo(int32_t quality, int32_t level);

#endif
