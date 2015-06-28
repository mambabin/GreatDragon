#ifndef _COMPONENT_HPP_
#define _COMPONENT_HPP_

#include "PlayerAtt.hpp"
#include "NPCAtt.hpp"
#include "PlayerEntity.hpp"
#include "NPCEntity.hpp"
#include "BaseInfo.hpp"
#include "RoleAtt.hpp"
#include "Movement.hpp"
#include "Equipment.hpp"
#include "Fight.hpp"
#include "Status.hpp"
#include "AI.hpp"
#include "ItemInfo.hpp"
#include "Func.hpp"
#include "Mission.hpp"
#include "Item.hpp"

struct Component{
	struct PlayerEntity *player;
	struct NPCEntity *npc;
	const BaseAtt *baseAtt;
	const RoleAtt *roleAtt;
	const PlayerAtt *playerAtt;
	const NPCAtt *npcAtt;
	struct Movement *movement;
	struct Equipment *equipment;
	struct Fight *fight;
	struct Status *status;
	struct AI *ai;
	struct Func *func;
	struct Mission *mission;
	struct Item *item;
};

bool Component_IsValid(struct Component *component);

void Component_Destroy(struct Component *component);

void Component_Update(struct Component *component);

#endif
