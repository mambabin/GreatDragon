#include "Component.hpp"
#include "PlayerEntity.hpp"
#include "NPCEntity.hpp"
#include "BaseInfo.hpp"
#include "Movement.hpp"
#include "Equipment.hpp"
#include "Fight.hpp"
#include "Status.hpp"
#include "AI.hpp"
#include "ItemInfo.hpp"
#include "Func.hpp"
#include "Mission.hpp"
#include "Item.hpp"
#include "MapPool.hpp"
#include "Debug.hpp"

bool Component_IsValid(struct Component *component) {
	if (component == NULL) {
		return false;
	} else if (component->player != NULL) {
		return PlayerEntity_IsValid(component->player);
	} else if (component->npc != NULL) {
		return NPCEntity_IsValid(component->npc);
	} else {
		return false;
	}
}

static inline void Finalize(struct Component *component) {
	if (component->fight != NULL)
		Fight_Finalize(component->fight);
	if (component->movement != NULL)
		Movement_Finalize(component->movement);
	if (component->status != NULL)
		Status_Finalize(component->status);
	if (component->player != NULL) {
		PlayerEntity_Finalize(component->player);
	} else if (component->npc != NULL) {
		NPCEntity_Finalize(component->npc);
	} else
		assert(0);
}

void Component_Destroy(struct Component *component) {
	if (!Component_IsValid(component))
		return;

	Finalize(component);

	if (component->item != NULL) {
		Item_Destroy(component->item);
		component->item = NULL;
	}

	if (component->mission != NULL) {
		Mission_Destroy(component->mission);
		component->mission = NULL;
	}

	if (component->func != NULL) {
		Func_Destroy(component->func);
		component->func = NULL;
	}

	int32_t map = -1;
	if (component->movement != NULL) {
		map = Movement_Att(component->movement)->mapID();

		Movement_Destroy(component->movement);
		component->movement = NULL;
	}

	if (component->equipment != NULL) {
		Equipment_Destroy(component->equipment);
		component->equipment = NULL;
	}

	if (component->fight != NULL) {
		Fight_Destroy(component->fight);
		component->fight = NULL;
	}

	if (component->status != NULL) {
		Status_Destroy(component->status);
		component->status = NULL;
	}

	if (component->ai != NULL) {
		AI_Destroy(component->ai);
		component->ai = NULL;
	}

	if (component->player != NULL) {
		PlayerEntity_Destroy(component->player);
		component->player = NULL;

		if (map != -1)
			MapPool_Release(map);
	}
	else if (component->npc != NULL) {
		NPCEntity_Destroy(component->npc);
		component->npc = NULL;
	}
	else {
		assert(0);
	}

	component->baseAtt = NULL;
	component->roleAtt = NULL;
	component->playerAtt = NULL;
	component->npcAtt = NULL;
}

void Component_Update(struct Component *component) {
	if (!Component_IsValid(component))
		return;

	if (component->player != NULL)
		PlayerEntity_Update(component->player);
	else if (component->npc != NULL)
		NPCEntity_Update(component->npc);
	else
		assert(0);

	if (component->ai != NULL)
		AI_Update(component->ai);

	if (component->movement != NULL)
		Movement_Update(component->movement);

	if (component->fight != NULL)
		Fight_Update(component->fight);

	if (component->status != NULL)
		Status_Update(component->status);
}
