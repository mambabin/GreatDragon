#include "Equipment.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "EquipmentInfo.hpp"
#include "IDGenerator.hpp"
#include "BaseInfo.hpp"
#include "ProfessionInfo.hpp"
#include "ProfessionInfoManager.hpp"
#include "EquipmentInfoManager.hpp"
#include "Item.hpp"
#include "PlayerPool.hpp"
#include "FightInfo.hpp"
#include "GoodsInfoManager.hpp"
#include "FashionInfoManager.hpp"
#include "Status.hpp"
#include "StatusEntity.hpp"
#include "StatusInfo.pb.h"
#include "StatusInfoManager.hpp"
#include "GodShipInfoManager.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

struct Equipment{
	int32_t id;
	EquipmentAtt *att;
	struct Component *component;
};

struct StrongGroup{
	int level;
	FightAtt::PropertyType type[STRONG_GROUP_ATT];
	int att[STRONG_GROUP_ATT];
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct Equipment *> pool;
	vector<struct StrongGroup> strongGroup;
}package;


void Equipment_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
	{
		package.strongGroup.clear();
		string name = Config_DataPath() + string("/StrongGroup.txt");
		FILE *file = fopen(name.c_str(), "r");
		if (file == NULL) {
			DEBUG_LOGERROR("Failed to open file: %s", name.c_str());
			exit(EXIT_FAILURE);
		}
		for (int i = 1; i <= PlayerEntity_MaxStrongLevel(); i++) {
			char key[16];
			SNPRINTF1(key, "%d", i);
			char line[CONFIG_FIXEDARRAY];
			if (!ConfigUtil_ReadStr(file, key, line, CONFIG_FIXEDARRAY))
				continue;

			char *tokens[CONFIG_FIXEDARRAY];
			int count = ConfigUtil_ExtraToken(line, tokens, CONFIG_FIXEDARRAY, ",");
			if (count == -1) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}
			struct StrongGroup strongGroup;
			strongGroup.level = i;
			for (int j = 0; j < STRONG_GROUP_ATT; j++) {
				strongGroup.type[j] = (FightAtt::PropertyType)atoi(tokens[j * STRONG_GROUP_ATT]);
				strongGroup.att[j] = atoi(tokens[j * STRONG_GROUP_ATT + 1]);
			}
			package.strongGroup.push_back(strongGroup);
		}
		fclose(file);
	}
}

static void ModifyAtt(struct Equipment *equipment, const EquipmentInfo *info, EquipAsset *asset, bool add) {
	float strongFactor = 0;
	for (int i = 0; i < asset->strongLevel(); i++) {
		const struct StrongTable *strongTable = PlayerEntity_StrongTable(i);
		assert(strongTable != NULL);
		strongFactor += strongTable->att;
	}
	for (int i = 0; i < info->baseDelta_size(); i++) {
		if (info->baseDelta(i) == 0)
			continue;

		FightAtt::PropertyType type = (FightAtt::PropertyType)info->baseType(i);
		int32_t delta = add ? info->baseDelta(i) : -info->baseDelta(i);
		delta = (int32_t)(delta * (1.0f + strongFactor));
		Fight_ModifyPropertyDelta(equipment->component->fight, type, delta, 0);
	}
	for (int i = 0; i < asset->enhanceDelta_size(); i++) {
		if (asset->enhanceDelta(i) == 0)
			continue;

		FightAtt::PropertyType type = (FightAtt::PropertyType)info->enhanceType(i);
		int32_t delta = add ? asset->enhanceDelta(i) : -asset->enhanceDelta(i);
		Fight_ModifyPropertyDelta(equipment->component->fight, type, delta, 0);
	}
	for (int i = 0; i < asset->randomType_size(); i++) {
		if (asset->randomType(i) == -1)
			continue;

		FightAtt::PropertyType type = (FightAtt::PropertyType)asset->randomType(i);
		Fight_ModifyPropertyDelta(equipment->component->fight, type, asset->randomDelta(i), 0);
	}
	for (int i = 0; i < asset->gemModel_size(); i++) {
		if (asset->gemModel(i) < 0)
			continue;

		const GoodsInfo *gem = GoodsInfoManager_GoodsInfo(asset->gemModel(i));
		assert(gem != NULL);
		Fight_ModifyPropertyDelta(equipment->component->fight, (FightAtt::PropertyType)gem->arg(0), add ? gem->arg(1) : -gem->arg(1), 0);
	}
	/*
	   DEBUG_LOG("strongFactor: %f", strongFactor);
	   for (int i = 1; i < 2; i++)
	   DEBUG_LOG("%d: %d", i, Fight_FinalProperty(equipment->component->fight, (FightAtt::PropertyType)i));
	   */
}

static void ModifyGroupAtt(struct Equipment *equipment, bool add) {
	int min = -1;
	for (int i = 0; i < equipment->att->equipments_size(); i++) {
		int64_t equip = equipment->att->equipments(i);
		if (equip == -1)
			return;
		EquipAsset *info = Item_EquipAsset(equipment->component->item, equip);
		assert(info != NULL);
		if (min == -1)
			min = info->strongLevel();
		else
			min = min < info->strongLevel() ? min : info->strongLevel();
	}
	for (size_t i = 0; i < package.strongGroup.size(); i++) {
		const struct StrongGroup *strongGroup = &package.strongGroup[i];
		if (min >= strongGroup->level) {
			for (int j = 0; j < STRONG_GROUP_ATT; j++) {
				Fight_ModifyPropertyDelta(equipment->component->fight, strongGroup->type[j], add ? strongGroup->att[j] : -strongGroup->att[j], 0);
			}
			DEBUG_LOG("%s group att, level: %d", add ? "add" : "dec", strongGroup->level);
		}
	}
}

struct Equipment * Equipment_Create(EquipmentAtt *att, struct Component *component) {
	if (att == NULL || component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct Equipment *equipment = SearchBlock(package.pool, id);
	equipment->id = id;
	equipment->att = att;
	equipment->component = component;

	return equipment;
}

bool Equipment_IsValid(struct Equipment *equipment) {
	return equipment != NULL && IDGenerator_IsValid(package.idGenerator, equipment->id);
}

const EquipmentAtt * Equipment_Att(struct Equipment *equipment) {
	if (!Equipment_IsValid(equipment))
		return NULL;

	return equipment->att;
}

void Equipment_Destroy(struct Equipment *equipment) {
	if (!Equipment_IsValid(equipment))
		return;

	IDGenerator_Release(package.idGenerator, equipment->id);
}

void Equipment_Prepare(struct Equipment *equipment) {
	if (!Equipment_IsValid(equipment))
		return;

	if (equipment->component->player != NULL) {
		int64_t equips[EquipmentAtt::EQUIPMENTS_SIZE];
		for (int i = 0; i < equipment->att->equipments_size(); i++)
			equips[i] = equipment->att->equipments(i);
		for (int i = 0; i < equipment->att->equipments_size(); i++)
			equipment->att->set_equipments(i, -1);

		for (int i = 0; i < equipment->att->equipments_size(); i++) {
			int64_t equip = equips[i];
			if (equip == -1)
				continue;
			Equipment_Wear(equipment, equip);
		}
	}
}

struct Component * Equipment_Component(struct Equipment *equipment) {
	if (!Equipment_IsValid(equipment))
		return NULL;

	return equipment->component;
}

int64_t Equipment_Wear(struct Equipment *equipment, int32_t id) {
	if (!Equipment_IsValid(equipment))
		return -2;

	if (equipment->component->player == NULL)
		return -2;

	const EquipmentInfo *info = Item_EquipInfo(equipment->component->item, id);
	EquipAsset *asset = Item_EquipAsset(equipment->component->item, id);
	if (info == NULL || asset == NULL)
		return -2;

	if (info->professionType() != -1) {
		if (equipment->component->baseAtt->professionType() != (ProfessionInfo::Type)info->professionType())
			return -2;
	}
	if (Fight_Att(equipment->component->fight)->level() < info->requiredLevel())
		return -2;

	switch(info->type()) {
		default:
			{
				int64_t prev = equipment->att->equipments(info->type());
				if (prev != -1) {
					ModifyAtt(equipment, Item_EquipInfo(equipment->component->item, prev), Item_EquipAsset(equipment->component->item, prev), false);
					ModifyGroupAtt(equipment, false);
				}
				equipment->att->set_equipments(info->type(), id);
				ModifyAtt(equipment, info, asset, true);
				ModifyGroupAtt(equipment, true);

				return prev;
			}
			break;
	}

	return -1;
}

int64_t Equipment_Unwear(struct Equipment *equipment, EquipmentInfo::Type pos) {
	if (!Equipment_IsValid(equipment))
		return -2;
	if (!EquipmentInfo::Type_IsValid(pos))
		return -2;

	int64_t prev = equipment->att->equipments(pos);
	if (prev != -1) {
		ModifyGroupAtt(equipment, false);
		ModifyAtt(equipment, Item_EquipInfo(equipment->component->item, prev), Item_EquipAsset(equipment->component->item, prev), false);
		equipment->att->set_equipments(pos, -1);
	}

	return prev;
}

int32_t Equipment_WearFashion(struct Equipment *equipment, int32_t id) {
	if (!Equipment_IsValid(equipment))
		return -2;

	const FashionInfo *info = FashionInfoManager_FashionInfo(id);
	if (info == NULL)
		return -2;

	const ItemPackage *package = Item_Package(equipment->component->item);
	if (id < 0 || id >= package->fashions_size())
		return -2;
	const FashionAsset *asset = &package->fashions(id);
	int v = asset->v();
	if (v == -1 || v == 0)
		return -2;

	int32_t prev = equipment->att->fashion();
	equipment->att->set_fashion(id);
	if (prev > 0) {
		Equipment_DelStatusToBody(equipment, prev);
	}

	Equipment_AddStatusToBody(equipment, id, StatusInfo::ROOM_EXP);
	return prev;
}

int32_t Equipment_UnwearFashion(struct Equipment *equipment) {
	if (!Equipment_IsValid(equipment))
		return -2;

	int32_t prev = equipment->att->fashion();
	equipment->att->set_fashion(-1);

	Equipment_DelStatusToBody(equipment, prev);

	return prev;
}

int32_t Equipment_WearWing(struct Equipment *equipment, int32_t id, bool baseWing) {
	if (!Equipment_IsValid(equipment))
		return -2;

	int32_t prev = equipment->att->wing();
	if (!baseWing) {
		const ItemPackage *package = Item_Package(equipment->component->item);
		if (id < 0 || id >= package->wings_size())
			return -2;
	
		int v = package->wings(id);
		if (v == -1 || v == 0)
			return -2;
	}else {
		const FightAtt* fightAtt = Fight_Att(equipment->component->fight);
		if (fightAtt == NULL) {
			return -2;
		}

		if (id > (fightAtt->baseWingLevel() * Config_MaxBaseWingDegree() + fightAtt->baseWingDegree())) {
			return -2;
		}	
	}

	equipment->att->set_wing(id);
	equipment->att->set_baseWing(baseWing);
	return prev;
}

int32_t Equipment_UnwearWing(struct Equipment *equipment) {
	if (!Equipment_IsValid(equipment))
		return -2;

	int32_t prev = equipment->att->wing();
	equipment->att->set_wing(-1);

	return prev;
}

bool Equipment_AddStatusToBody(struct Equipment *equipment, int32_t id, StatusInfo::StatusType type) {
	if (!Equipment_IsValid(equipment)) {
		return false;
	}
	const ItemPackage *package = Item_Package(equipment->component->item);
	if (id < 0 || id >= package->fashions_size()) {
		return false;
	}
	const FashionAsset *asset = &package->fashions(id);

	for (int i = 0; i < asset->runes_size(); ++i) {
		if (asset->runes(i) < 0) {
			continue;
		}
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(asset->runes(i));
		if (goods == NULL) {
			DEBUG_LOG("Equipment_AddStatusToBody fail ( goods == NULL )");
			continue;
		}
		int statusID = goods->arg(2);
		const StatusInfo *statusInfo = StatusInfoManager_StatusInfo(statusID);
		if (statusInfo != NULL && statusInfo->statusType() == type) {
			bool resistControl;
			struct StatusEntity *status;
			if (-1 == StatusEntity_Create(equipment->component->status, equipment->component->status, NULL, StatusInfo::ATTACKER, statusInfo, NULL, &status, 1, 0, &resistControl)) {
				DEBUG_LOG("Equipment_AddStatusToBody fail (StatusEntity_create return -1)");
				continue;
			}
		}
	}

	return true;
}


bool Equipment_DelStatusToBody(struct Equipment *equipment, int32_t id) {
	if (!Equipment_IsValid(equipment)) {
		return false;
	}
	const ItemPackage *package = Item_Package(equipment->component->item);
	if (id < 0 || id >= package->fashions_size()) {
		return false;
	}
	const FashionAsset *asset = &package->fashions(id);

	struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
	int count = Status_GetType(equipment->component->status, StatusInfo::ROOM_EXP, statuses, CONFIG_FIXEDARRAY);

	for (int i = 0; i < asset->runes_size(); ++i) {
		if (asset->runes(i) < 0) {
			continue;
		}
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(asset->runes(i));
		if (goods == NULL) {
			DEBUG_LOG("Equipment_DelStatusToBody fail ( goods == NULL )");
			continue;
		}

		for (int j = 0; j < count; ++j) {
			const StatusInfo *info = StatusEntity_Info(statuses[j]);
			if (NULL == info) {
				continue;
			}

			if (goods->arg(2) == info->id()) {
				StatusEntity_Destroy(statuses[i]);
				break;
			}
		}
	}

	return true;
}


bool Equipment_EquipIsInBody(struct Equipment *equipment, int32_t id, ItemInfo::Type type) {
	if (type == ItemInfo::FASHION) {
		if (id == equipment->att->fashion()) {
			return true;
		}
	}
	return false;
}

static void ModifyAtt_RideAsset(struct Equipment *equipment, struct RidesAsset *rideAsset, bool add)
{
	int px = 1;
	if(!add)
		px = -1;

	/*
	Fight_ModifyPropertyDelta(equipment->component->fight, FightAtt::ATK, rideAsset->atk() * px, 0);
	Fight_ModifyPropertyDelta(equipment->component->fight, FightAtt::DEF, rideAsset->def() * px, 0);
	Fight_ModifyPropertyDelta(equipment->component->fight, FightAtt::MAXHP, rideAsset->maxHP() * px, 0);
	Fight_ModifyPropertyDelta(equipment->component->fight, FightAtt::CRIT, rideAsset->crit() * px, 0);
	Fight_ModifyPropertyDelta(equipment->component->fight, FightAtt::ACCURACY, rideAsset->accuracy() * px, 0);
	Fight_ModifyPropertyDelta(equipment->component->fight, FightAtt::DODGE, rideAsset->dodge() * px, 0);
	*/
	for(int i = 0, count = rideAsset->att_size(); i < count; i++)
		Fight_ModifyPropertyDelta(equipment->component->fight, (FightAtt::PropertyType)i, rideAsset->att(i) * px, 0);
}

int Equipment_Ride(struct Equipment *equipment, int32_t id, bool ride)
{
	if(!Equipment_IsValid(equipment))
		return -1;

	int32_t curRide = equipment->att->rides();
	if(ride && id == curRide)
		return -2;

	if(curRide >= 0)
	{
		RidesAsset *curAsset = Item_RidesAsset(equipment->component->item, curRide);
		if(curAsset == NULL)
			return -3;
		ModifyAtt_RideAsset(equipment, curAsset, false);
	}

	if(!ride)
	{
		equipment->att->set_rides(-1);
		return 0;
	}

	RidesAsset *newAsset = Item_RidesAsset(equipment->component->item, id);
	if(newAsset == NULL)
		return -4;

	ModifyAtt_RideAsset(equipment, newAsset, true);
	equipment->att->set_rides(id);

	return 0;
}

int32_t Equipment_WearGodShip(struct Equipment *equipment, int playerLevel, int32_t type, int32_t index) {
	if (!Equipment_IsValid(equipment))
		return -7;

	for (int i = 0; i < equipment->att->godShips_size(); i++) {
		if (equipment->att->godShips(i) != -1) {
			int j = equipment->att->godShips(i);
			const ItemPackage *package = Item_Package(equipment->component->item);
			if (package == NULL)
				return -9;

			const GodShipAsset *asset = &package->godShips(j);
			if (asset == NULL)
				return -11;

			const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
			if (info == NULL)
				return -12;
			if (info->Type() == type)
				return -8;
		}
	}
	for (int i = 0; i < equipment->att->godShips_size(); i++) {
		if (equipment->att->godShips(i) == -1) {
			if (playerLevel / 10 > i) {
				equipment->att->set_godShips(i, index);
				return i;
			} else {
				return -10;
			}
		}
	}
	return -1;
}

int32_t Equipment_UnWearGodShip(struct Equipment *equipment, int32_t index) {
	if (!Equipment_IsValid(equipment))
		return -1;
	for (int i = 0; i < equipment->att->godShips_size(); i++) {
		if (equipment->att->godShips(i) == index) {
			equipment->att->set_godShips(i, -1);
			return 1;
		}
	}
	return -1;
}
