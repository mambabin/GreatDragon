#ifndef _SKILL_INFO_MANAGER_HPP_
#define _SKILL_INFO_MANAGER_HPP_

#include "SkillInfo.pb.h"
#include <sys/types.h>

void SkillInfoManager_Init();

const SkillInfo * SkillInfoManager_SkillInfo(int32_t id, int level);
int SkillInfoManager_MaxLevel(int32_t id);

#endif
