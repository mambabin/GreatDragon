#ifndef _NPC_ENTITY_HPP_
#define _NPC_ENTITY_HPP_

#include "Component.hpp"
#include "NPCAtt.hpp"
#include "SkillInfo.pb.h"
#include "Math.hpp"
#include <sys/types.h>

struct NPCEntity;

void NPCEntity_Init();

// Use MapPool_GenNPC instead of this.
struct NPCEntity * NPCEntity_Create(int32_t id, const NPCAtt *att, struct Component *master);
bool NPCEntity_IsValid(struct NPCEntity *entity);
void NPCEntity_Destroy(struct NPCEntity *entity);
void NPCEntity_Finalize(struct NPCEntity *entity);

int32_t NPCEntity_ID(struct NPCEntity *entity);
int32_t NPCEntity_ResID(struct NPCEntity *entity);

struct Component * NPCEntity_Master(struct NPCEntity *entity);

const NPCAtt * NPCEntity_Att(struct NPCEntity *entity);

struct Component * NPCEntity_Component(struct NPCEntity *entity);

void NPCEntity_ToSceneData(const NPCAtt *src, PB_NPCAtt *dest);

void NPCEntity_Update(struct NPCEntity *entity);

int NPCEntity_PetHaloAttributeIncrease(struct NPCEntity *entity, bool flag);

#endif
