#ifndef _MISSION_HPP_
#define _MISSION_HPP_

#include "Component.hpp"
#include "MissionInfo.hpp"
#include <sys/types.h>

struct Mission;

void Mission_Init();
void Mission_Prepare(struct Mission *mission);

struct Mission * Mission_Create(MissionAllRecord *record, struct Component *component);
bool Mission_IsValid(struct Mission *mission);
void Mission_Destroy(struct Mission *mission);

const MissionAllRecord * Mission_AllRecord(struct Mission *mission);

struct Component * Mission_Component(struct Mission *mission);

void Mission_Add(struct Mission *mission, int32_t id);

int Mission_GiveUp(struct Mission *mission, int32_t id);

bool Mission_CanApply(struct Mission *mission, int32_t id);
bool Mission_CanShow(struct Mission *mission, int32_t id);

void Mission_CompleteTarget(struct Mission *mission, MissionTarget::Type type, int32_t arg1, int32_t arg2);

bool Mission_IsMissionCompleted(struct Mission *mission, int32_t id);

int Mission_MissionCompleteCount(struct Mission *mission, int32_t id);

// It does not ensure completion of the mission, check that using Mission_IsMissionCompleted.
void Mission_CompleteMission(struct Mission *mission, int32_t id);

#endif
