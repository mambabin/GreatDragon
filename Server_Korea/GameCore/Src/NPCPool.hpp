#ifndef _NPC_POOL_HPP_
#define _NPC_POOL_HPP_

#include <sys/types.h>
#include "NPCEntity.hpp"

void NPCPool_Init();
void NPCPool_Prepare();

// 0: Succeed.
// -1: Failure.
int NPCPool_Add(struct NPCEntity *entity);
void NPCPool_Del(struct NPCEntity *entity);

struct NPCEntity * NPCPool_NPC(int32_t mapID, int32_t id);

int NPCPool_Count(int32_t mapID);
int NPCPool_MonsterCount(int32_t mapID);
int NPCPool_PetCount(int32_t mapID);
int NPCPool_TotalCount();

// monster and pet
int NPCPool_NPCs(int32_t mapID, struct NPCEntity **npcs, size_t size);
int NPCPool_Monsters(int32_t mapID, struct NPCEntity **npcs, size_t size);
int NPCPool_Pets(int32_t mapID, struct NPCEntity **npcs, size_t size);
struct NPCEntity * NPCPool_Pet(int32_t mapID, struct Component *master);

int NPCPool_FreeFaction(int32_t mapID);
int32_t NPCPool_FreeID(int32_t mapID);

#endif
