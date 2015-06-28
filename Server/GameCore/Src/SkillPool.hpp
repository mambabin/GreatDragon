#ifndef _SKILL_POOL_HPP_
#define _SKILL_POOL_HPP_

#include "SkillEntity.hpp"
#include <sys/types.h>

void SkillPool_Init();
void SkillPool_Prepare();

void SkillPool_Add(struct SkillEntity *entity);
void SkillPool_Del(struct SkillEntity *entity);

#endif
