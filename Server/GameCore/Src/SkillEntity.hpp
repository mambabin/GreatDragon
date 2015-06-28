#ifndef _SKILL_ENTITY_HPP_
#define _SKILL_ENTITY_HPP_

#include "Component.hpp"
#include "SkillInfo.pb.h"
#include "Fight.hpp"
#include "Math.hpp"
#include <sys/types.h>

struct SkillEntity;

void SkillEntity_Init();

int SkillEntity_Create(const SkillInfo *skill, struct Fight *owner, struct Fight *tFight, const Vector3f *tPos, int32_t *ids, size_t size, const int32_t *skills, size_t count);
void SkillEntity_Destroy(struct SkillEntity *entity);

struct SkillEntity * SkillEntity_Skill(int32_t id);
int32_t SkillEntity_ID(struct SkillEntity *entity);

SkillInfo::StatusType SkillEntity_Status(struct SkillEntity *entity);

struct Fight * SkillEntity_Owner(struct SkillEntity *entity);

int32_t SkillEntity_MapID(struct SkillEntity *entity);

const SkillInfo * SkillEntity_Info(struct SkillEntity *entity);

void SkillEntity_Update(struct SkillEntity *entity);

#endif
