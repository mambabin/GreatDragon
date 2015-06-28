#include "NPCEntity.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "NPCPool.hpp"
#include "IDGenerator.hpp"
#include "Movement.hpp"
#include "Equipment.hpp"
#include "Fight.hpp"
#include "Status.hpp"
#include "AI.hpp"
#include "Func.hpp"
#include "ProfessionInfo.hpp"
#include "GCAgent.hpp"
#include "MapInfoManager.hpp"
#include "SkillInfo.pb.h"
#include "Time.hpp"
#include "NPCAtt.hpp"
#include "NetProto.pb.h"
#include "Math.hpp"
#include "ItemInfo.hpp"
#include "NPCAtt.hpp"
#include "NPCInfoManager.hpp"
#include <sys/types.h>
#include <map>
#include <iostream>
#include <vector>

using namespace std;

struct NPCEntity{
	int32_t index;
	int32_t id;
	NPCAtt att;
	struct Component *master;
	struct Component component;
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct NPCEntity *> pool;
}package;

void NPCEntity_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
}

struct NPCEntity * NPCEntity_Create(int32_t id, const NPCAtt *att, struct Component *master) {
	if (id < 0 || att == NULL) {
		return NULL;
	}

	int32_t index = IDGenerator_Gen(package.idGenerator);
	if (index == -1)
		return NULL;

	struct NPCEntity *entity = SearchBlock(package.pool, index);
	entity->index = index;
	entity->id = id;
	entity->att = *att;
	entity->master = master;
	entity->component.player = NULL;
	entity->component.npc = entity;
	entity->component.playerAtt = NULL;
	entity->component.npcAtt = &entity->att;
	entity->component.baseAtt = entity->att.mutable_att()->mutable_baseAtt();
	entity->component.roleAtt = entity->att.mutable_att();
	entity->component.movement = Movement_Create(entity->att.mutable_att()->mutable_movementAtt(), &entity->component);
	entity->component.equipment = Equipment_Create(entity->att.mutable_att()->mutable_equipmentAtt(), &entity->component);
	entity->component.fight = Fight_Create(entity->att.mutable_att()->mutable_fightAtt(), &entity->component);
	entity->component.status = Status_Create(&entity->component);
	entity->component.ai = AI_Create(entity->att.mutable_att()->mutable_aiAtt(), &entity->component);
	entity->component.func = Func_Create(entity->att.mutable_funcAtt(), &entity->component);
	entity->component.mission = NULL;
	entity->component.item = NULL;

	AI_Prepare(entity->component.ai);
	Fight_Prepare(entity->component.fight);
	NPCEntity_PetHaloAttributeIncrease(entity, true);

	NPCPool_Add(entity);

	return entity;
}

bool NPCEntity_IsValid(struct NPCEntity *entity) {
	return entity != NULL && IDGenerator_IsValid(package.idGenerator, entity->index);
}

void NPCEntity_Destroy(struct NPCEntity *entity) {
	if (!NPCEntity_IsValid(entity))
		return;

	DEBUG_LOG("Destroy npc, NPCIndex: %d, id: %d, mapid: %d", entity->index, entity->id, entity->att.att().movementAtt().mapID());
	IDGenerator_Release(package.idGenerator, entity->index);
}

void NPCEntity_Finalize(struct NPCEntity *entity) {
	if (!NPCEntity_IsValid(entity))
		return;

	NPCPool_Del(entity);
}

int32_t NPCEntity_ID(struct NPCEntity *entity) {
	if (!NPCEntity_IsValid(entity))
		return -1;

	return entity->id;
}

int32_t NPCEntity_ResID(struct NPCEntity *entity) {
	if (!NPCEntity_IsValid(entity))
		return -1;

	return entity->att.id();
}

struct Component * NPCEntity_Master(struct NPCEntity *entity) {
	if (!NPCEntity_IsValid(entity))
		return NULL;
	return entity->master;
}

const NPCAtt * NPCEntity_Att(struct NPCEntity *entity) {
	if (!NPCEntity_IsValid(entity)) {
		DEBUG_LOG("NPCIndex: %d, id: %d, mapid: %d", entity->index, entity->id, entity->att.att().movementAtt().mapID());
		return NULL;
	}

	return &entity->att;
}

struct Component * NPCEntity_Component(struct NPCEntity *entity) {
	if (!NPCEntity_IsValid(entity))
		return NULL;

	return &entity->component;
}

void NPCEntity_ToSceneData(const NPCAtt *src, PB_NPCAtt *dest) {
	if (src == NULL || dest == NULL)
		return;

	dest->mutable_att()->mutable_baseAtt()->set_name(src->att().baseAtt().name());
	dest->mutable_att()->mutable_baseAtt()->set_professionType((PB_ProfessionInfo::Type)src->att().baseAtt().professionType());
	dest->mutable_att()->mutable_baseAtt()->set_male(src->att().baseAtt().male());
	dest->mutable_att()->mutable_baseAtt()->set_roleID(src->att().baseAtt().roleID());
	dest->mutable_att()->mutable_baseAtt()->set_height(src->att().baseAtt().height());

	src->att().movementAtt().ToPB(dest->mutable_att()->mutable_movementAtt());

	src->att().equipmentAtt().ToPB(dest->mutable_att()->mutable_equipmentAtt());

	dest->mutable_att()->mutable_fightAtt()->set_selfFaction(src->att().fightAtt().selfFaction());
	dest->mutable_att()->mutable_fightAtt()->set_friendlyFaction(src->att().fightAtt().friendlyFaction());
	dest->mutable_att()->mutable_fightAtt()->set_hp(src->att().fightAtt().hp());
	dest->mutable_att()->mutable_fightAtt()->set_mana(src->att().fightAtt().mana());
	for (int i = 0; i < src->att().fightAtt().properties_size(); i++) {
		dest->mutable_att()->mutable_fightAtt()->add_properties(src->att().fightAtt().properties(i));
		src->att().fightAtt().propertiesDelta(i).ToPB(dest->mutable_att()->mutable_fightAtt()->add_propertiesDelta());
	}
	dest->mutable_att()->mutable_fightAtt()->set_level(src->att().fightAtt().level());
	dest->mutable_att()->mutable_fightAtt()->set_energy(src->att().fightAtt().energy());
	dest->mutable_att()->mutable_fightAtt()->set_reviveTime(src->att().fightAtt().reviveTime());
	if (src->att().baseAtt().professionType() != ProfessionInfo::NPC) {
		for (int i = 0; i < src->equips_size(); i++) {
			src->equips(i).ToPB(dest->add_equips());
		}
	}
	/*
	for (int i = 0; i < src->att().fightAtt().skills_size(); i++)
		src->att().fightAtt().skills(i).ToPB(dest->mutable_att()->mutable_fightAtt()->add_skills());
		*/
}

void NPCEntity_Update(struct NPCEntity *entity) {
	assert(NPCEntity_IsValid(entity));
}

int NPCEntity_PetHaloAttributeIncrease(struct NPCEntity *entity, bool flag) {
	if (!NPCEntity_IsValid(entity))
		return -1;

	if(!(entity->component.npc != NULL && NPCEntity_Master(entity->component.npc) != NULL)) {
		return -2;
	}

	const std::map<int, map<int, PB_PetHaloInfo> >* info = NPCInfoManager_PetHaloInfo();
	if (info == NULL)
		return -2;

	if (entity->master == NULL) 
		return -3;

	const PlayerAtt* att = PlayerEntity_Att(entity->master->player);
	if (att == NULL)
		return -3;

	int value[PB_FightAtt_PropertyType_PropertyType_ARRAYSIZE] = {0};
	for (int i = 0; i < att->haloLevel_size() && i < att->haloValue_size(); ++i) {
		map<int, map<int, PB_PetHaloInfo> >::const_iterator itGroup = info->find(i);
		if (itGroup == info->end()) {
			continue;
		}

		map<int, PB_PetHaloInfo>::const_iterator itUnit = itGroup->second.find(att->haloLevel(i));
		if (itUnit == itGroup->second.end()) {
			continue;
		}

		value[i] = itUnit->second.propertyValue() * (1 + (float)(att->haloValue(i)) / 10000.0);
	}

	for (int i = 0; i < (int)(sizeof(value) / sizeof(int)); i++) {
		if (flag) {
			Fight_ModifyProperty(entity->component.fight, (FightAtt::PropertyType)i, value[i]);
		}else {
			Fight_ModifyProperty(entity->component.fight, (FightAtt::PropertyType)i, -value[i]);
		}
	}
	return 0;
}



