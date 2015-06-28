#ifndef _STATUS_ENTITY_HPP_
#define _STATUS_ENTITY_HPP_

#include "StatusInfo.pb.h"
#include "Status.hpp"
#include "NetProto.pb.h"
#include "Math.hpp"
#include "SkillEntity.hpp"

struct StatusEntity;

void StatusEntity_Init();
void StatusEntity_Prepare();

// resistControl: If any status is resisted, this param will be true, otherwise nothing.
int StatusEntity_Create(struct Status *attacker, struct Status *defender, const Vector3f *skillPos, StatusInfo::TargetType targetType, const StatusInfo *info, const SkillInfo *skill, struct StatusEntity **statuses, size_t size, int zero, bool *resistControl);
void StatusEntity_Destroy(struct StatusEntity *entity);

const StatusInfo * StatusEntity_Info(struct StatusEntity *entity);

bool StatusEntity_ToAddStatus(struct StatusEntity *entity, NetProto_AddStatus *addStatus);

void StatusEntity_Update(struct StatusEntity *entity);

int StatusEntity_Critdef(struct StatusEntity *entity);
int StatusEntity_Critdamage(struct StatusEntity *entity);

#endif
