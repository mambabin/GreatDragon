#include "Mission.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "MissionInfo.hpp"
#include "IDGenerator.hpp"
#include "MapPool.hpp"
#include "MissionInfoManager.hpp"
#include "GoodsInfoManager.hpp"
#include "EquipmentInfoManager.hpp"
#include "Item.hpp"
#include "Event.hpp"
#include "ScribeClient.hpp"
#include "AccountPool.hpp"
#include "RidesInfoManager.hpp"
#include "NetProto.pb.h"
#include "GCAgent.hpp"
#include "Debug.hpp"
#include <cstdarg>
#include <sys/types.h>
#include <cstdio>
#include <vector>

using namespace std;

struct Mission{
	int32_t id;
	MissionAllRecord *record;
	struct Component *component;
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct Mission *> pool;
}package;


void Mission_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
}

struct Mission * Mission_Create(MissionAllRecord *record, struct Component *component) {
	if (record == NULL || component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct Mission *mission = SearchBlock(package.pool, id);
	mission->id = id;
	mission->record = record;
	mission->component = component;

	return mission;
}

void Mission_Prepare(struct Mission *mission) {
	if (!Mission_IsValid(mission))
		return;

	for (int i = 0; i < mission->record->cur_size(); i++) {
		int32_t id = mission->record->cur(i);
		if (id == -1)
			continue;

		const MissionInfo *info = MissionInfoManager_MissionInfo(id);
		if (info == NULL)
			Mission_GiveUp(mission, id);
	}
}

bool Mission_IsValid(struct Mission *mission) {
	return mission != NULL && IDGenerator_IsValid(package.idGenerator, mission->id);
}

const MissionAllRecord * Mission_AllRecord(struct Mission *mission) {
	if (!Mission_IsValid(mission))
		return NULL;

	return mission->record;
}

void Mission_Destroy(struct Mission *mission) {
	if (!Mission_IsValid(mission))
		return;

	IDGenerator_Release(package.idGenerator, mission->id);
}

struct Component * Mission_Component(struct Mission *mission) {
	if (!Mission_IsValid(mission))
		return NULL;

	return mission->component;
}

static void AddToCur(MissionAllRecord *records, int32_t id) {
	for (int i = 0; i < records->cur_size(); i++) {
		if (records->cur(i) == -1) {
			records->set_cur(i, id);
			break;
		}
	}
}

void Mission_Add(struct Mission *mission, int32_t id) {
	if (!Mission_IsValid(mission))
		return;

	MissionAllRecord *records = mission->record;
	if (id < 0 || id >= records->records_size())
		return;

	const MissionInfo *info = MissionInfoManager_MissionInfo(id);
	assert(info != NULL);

	MissionRecord *record = records->mutable_records(id);

	for (int i = 0; i < record->target_size(); i++) {
		MissionTargetRecord *targetRecord = record->mutable_target(i);
		for (int j = 0; j < targetRecord->arg_size(); j++)
			targetRecord->set_arg(i, 0);
	}

	if (info->type() == MissionInfo::DAILY)
		records->mutable_records(id)->set_count(0);

	AddToCur(records, id);

	int32_t player = PlayerEntity_ID(mission->component->player);
	char arg1[32];
	SNPRINTF1(arg1, "{\"id\":\"%d\"}", id);
	ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "StartTask", arg1));

	DEBUG_LOG("Mission_Add: %d", id);
}

int Mission_GiveUp(struct Mission *mission, int32_t id) {
	if (!Mission_IsValid(mission))
		return -1;

	MissionAllRecord *records = mission->record;
	if (id < 0 || id >= records->records_size())
		return -1;

	for (int i = 0; i < records->cur_size(); i++) {
		if (records->cur(i) == id) {
			records->set_cur(i, -1);

			for (int j = 0; j < records->records(id).target_size(); j++)
				for (int n = 0; n < records->records(id).target(j).arg_size(); n++)
					records->mutable_records(id)->mutable_target(j)->set_arg(n, 0);

			return 0;
		}
	}

	return -1;
}

bool Mission_CanApply(struct Mission *mission, int32_t id) {
	if (!Mission_IsValid(mission))
		return false;

	MissionAllRecord *records = mission->record;
	if (id < 0 || id >= records->records_size())
		return false;

	const MissionInfo *info = MissionInfoManager_MissionInfo(id);
	if (info == NULL)
		return false;

	if (info->maxCount() != -1 && records->records(id).count() >= info->maxCount())
		return false;

	for (int i = 0; i < records->cur_size(); i++) {
		if (records->cur(i) == id)
			return false;
	}

	const MissionOpen *open = &info->open();
	for (int i = 0; i < open->type_size(); i++) {
		MissionOpen::Type type = open->type(i);
		if (type == MissionOpen::NONE)
			continue;

		const MissionArg *arg = &open->arg(i);

		switch(type) {
			case MissionOpen::LEVEL:
				{
					int32_t level = Fight_Att(mission->component->fight)->level();
					if (level < arg->arg(0) || level > arg->arg(1))
						return false;
				}
				break;

			case MissionOpen::COMPLETE_MISSION:
				{
					int32_t id = arg->arg(0);
					assert(id < records->records_size());
					if (records->records(id).count() <= 0)
						return false;
				}
				break;

			case MissionOpen::HASGOODS:
				{
					int32_t id = arg->arg(0);
					int32_t min = arg->arg(1);
					if (!Item_HasItem(mission->component->item, ItemInfo::GOODS, id, min))
						return false;
				}
				break;

			case MissionOpen::APPLY_MISSION:
				{
					int32_t id = arg->arg(0);
					assert(id < records->records_size());
					bool find = false;
					for (int i = 0; i < records->cur_size(); i++) {
						if (records->cur(i) == id) {
							find = true;
							break;
						}
					}
					if (!find)
						return false;
				}
				break;
			case MissionOpen::OPENSERVERDATE:
				{
					if ( !( Time_TodayZeroTime(Config_OpenTime(NULL)) < Time_TimeStamp() && Time_TimeStamp() < (Time_TodayZeroTime(Config_OpenTime(NULL)) + 3600 * 24 * 14)) ) {
						return false;
					}
				}
				break;
			default:
				return false;
		}
	}

	return true;
}

bool Mission_CanShow(struct Mission *mission, int32_t id) {
	if (!Mission_IsValid(mission))
		return false;

	MissionAllRecord *records = mission->record;
	if (id < 0 || id >= records->records_size())
		return false;

	const MissionInfo *info = MissionInfoManager_MissionInfo(id);
	if (info == NULL)
		return false;

	if (info->maxCount() != -1 && records->records(id).count() >= info->maxCount())
		return false;

	const MissionOpen *open = &info->open();
	for (int i = 0; i < open->type_size(); i++) {
		MissionOpen::Type type = open->type(i);
		if (type == MissionOpen::NONE)
			continue;

		const MissionArg *arg = &open->arg(i);

		switch(type) {
			case MissionOpen::LEVEL:
				{
					int32_t level = Fight_Att(mission->component->fight)->level();
					if (level < arg->arg(0) || level > arg->arg(1))
						return false;
				}
				break;

			case MissionOpen::COMPLETE_MISSION:
				{
					int32_t id = arg->arg(0);
					assert(id < records->records_size());
					if (records->records(id).count() <= 0)
						return false;
				}
				break;

			case MissionOpen::HASGOODS:
				{
					int32_t id = arg->arg(0);
					int32_t min = arg->arg(1);
					if (!Item_HasItem(mission->component->item, ItemInfo::GOODS, id, min))
						return false;
				}
				break;

			case MissionOpen::APPLY_MISSION:
				{
					int32_t id = arg->arg(0);
					assert(id < records->records_size());
					bool find = false;
					for (int i = 0; i < records->cur_size(); i++) {
						if (records->cur(i) == id) {
							find = true;
							break;
						}
					}
					if (!find)
						return false;
				}
				break;

			default:
				return false;
		}
	}

	return true;
}

void Mission_CompleteTarget(struct Mission *mission, MissionTarget::Type type, int32_t arg1, int32_t arg2) {
	if (!Mission_IsValid(mission))
		return;

	MissionAllRecord *records = mission->record;
	for (int i = 0; i < records->cur_size(); i++) {
		if (records->cur(i) == -1)
			continue;

		const MissionInfo *info = MissionInfoManager_MissionInfo(records->cur(i));
		const MissionTarget *target = &info->target();
		for (int j = 0; j < target->type_size(); j++) {
			if (target->type(j) == MissionTarget::NONE || target->type(j) != type)
				continue;

			const MissionArg *arg = &target->arg(j);
			MissionTargetRecord *targetRecord = records->mutable_records(info->id())->mutable_target(j);

			switch(type) {
				case MissionTarget::KILLNPC:
					{
						int32_t npc = arg1;
						if (npc != arg->arg(0))
							break;

						int prev = targetRecord->arg(0);
						if (prev < arg->arg(1)) {
							targetRecord->set_arg(0, prev + 1);
						}
					}
					break;
				case MissionTarget::TALK:
					{
						int32_t map = MapPool_MapInfo(Movement_Att(mission->component->movement)->mapID());
						int32_t npc = arg1;
						// DEBUG_LOG("map: %d npc: %d | map: %d npc: %d", map, npc, arg->arg(0), arg->arg(1));
						if (map != arg->arg(0) || npc != arg->arg(1))
							break;

						targetRecord->set_arg(0, 1);
					}
					break;
				case MissionTarget::GETGOODS:
					{
						int32_t goods = arg1;
						if (goods != arg->arg(0))
							break;
						int32_t count = arg2;

						int prev = targetRecord->arg(0);
						if (prev < arg->arg(1)) {
							targetRecord->set_arg(0, prev + count);
							if (targetRecord->arg(0) > arg->arg(1))
								targetRecord->set_arg(0, arg->arg(1));
						}
					}
					break;
				case MissionTarget::CLEAR_ROOM:
					{
						if (info->type() == MissionInfo::DAILY) {
							targetRecord->set_arg(0, targetRecord->arg(0) + 1);
							return;
						} else {
							int32_t map = MapPool_MapInfo(Movement_Att(mission->component->movement)->mapID());
							if (map != arg->arg(0))
								break;
							targetRecord->set_arg(0, 1);
						}
					}
					break;
				case MissionTarget::CLEAR_ONLYROOM:
				case MissionTarget::CLEAR_TOWER:
				case MissionTarget::CLEAR_SURVIVE:
				case MissionTarget::CLEAR_HERO:
				case MissionTarget::STRONG:
				case MissionTarget::UNLOCK_BLOODNODE:
				case MissionTarget::PVP:
				case MissionTarget::HELL_KILL:
					{
						targetRecord->set_arg(0, targetRecord->arg(0) + 1);
					}
					break;
				case MissionTarget::ENHANCE:
					{
						targetRecord->set_arg(0, targetRecord->arg(0) + arg1);
					}
					break;
				case MissionTarget::ENTER_ANY_ROOM:
					{
						int32_t map = arg1;
						if (map != arg->arg(0))
							break;
						targetRecord->set_arg(0, targetRecord->arg(0) + 1);
					}
					break;
				case MissionTarget::SURVIVE_TO:
					{
						int cur = targetRecord->arg(0);
						if (cur < arg1)
							targetRecord->set_arg(0, arg1);
					}
					break;
				default:
					return;
			}	
			/*
			char arg1[128];
			SNPRINTF1(arg1, "{\"id:\"%d\",\"type\":\"%d\"}", info->id(), type);
			int32_t player = PlayerEntity_ID(mission->component->player);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "CompleteTarget", arg1));
			*/
		}
	}
}

bool Mission_IsMissionCompleted(struct Mission *mission, int32_t id) {
	if (!Mission_IsValid(mission))
		return false;
	MissionAllRecord *records = mission->record;
	if (id < 0 || id >= records->records_size())
		return false;

	const MissionInfo *info = MissionInfoManager_MissionInfo(id);
	if (info == NULL)
		return false;
	
	const MissionOpen *open = &info->open();
	if (open == NULL)
		return false;
	
	MissionOpen::Type type = open->type(0);
	if (type == MissionOpen::OPENSERVERDATE) {
		int day= open->arg(0).arg(0);
		
		if (Mission_CanApply(mission, id)) {
			bool add = true;
			for (int i = 0; i < records->cur_size(); ++i) {
				if (records->cur(i) == id)
					add = false;
			}
			if (add) {
				if ((Time_TodayZeroTime(Config_OpenTime(NULL)) + 3600 * 24 * (day - 1)) < Time_TimeStamp() && Time_TimeStamp() < (Time_TodayZeroTime(Config_OpenTime(NULL)) + 3600 * 24 * 14)) {
					Mission_Add(mission, id);
				}
			}
		}

		if ( !( (Time_TodayZeroTime(Config_OpenTime(NULL)) + 3600 * 24 * (day - 1)) < Time_TimeStamp() && Time_TimeStamp() < (Time_TodayZeroTime(Config_OpenTime(NULL)) + 3600 * 24 * 14)) ) {
			return false;
		}
	}
	
	const MissionTarget *target = &info->target();

	for (int i = 0; i < records->cur_size(); i++) {
DEBUG_LOG("FFFFFFFFFFFFFF %d", records->cur(i));
		if (records->cur(i) != id)
			continue;
DEBUG_LOG("FFFFFFFFFFFFFF");

		for (int j = 0; j < target->type_size(); j++) {
			if (target->type(j) == MissionTarget::NONE)
				continue;

			MissionTarget::Type type = target->type(j);
			const MissionArg *arg = &target->arg(j);
			MissionTargetRecord *targetRecord = records->mutable_records(id)->mutable_target(j);

			switch(type) {
				case MissionTarget::KILLNPC:
					{
						int target = arg->arg(1);
						int cur = targetRecord->arg(0);
						if (cur < target)
							return false;
					}
					break;
				case MissionTarget::TALK:
					{
						if (targetRecord->arg(0) <= 0)
							return false;
					}
					break;
				case MissionTarget::GETGOODS:
					{
						int target = arg->arg(1);
						int cur = targetRecord->arg(0);
						if (cur < target)
							return false;
					}
					break;
				case MissionTarget::CLEAR_ROOM:
					{
						if (info->type() == MissionInfo::DAILY) {
							int target = arg->arg(0);
							if (targetRecord->arg(0) < target)
								return false;
						} else {
							if (targetRecord->arg(0) <= 0)
								return false;
						}
					}
					break;
				case MissionTarget::CLEAR_ONLYROOM:
				case MissionTarget::CLEAR_TOWER:
				case MissionTarget::CLEAR_SURVIVE:
				case MissionTarget::CLEAR_HERO:
				case MissionTarget::STRONG:
				case MissionTarget::UNLOCK_BLOODNODE:
				case MissionTarget::PVP:
				case MissionTarget::ENHANCE:
				case MissionTarget::HELL_KILL:
				case MissionTarget::SURVIVE_TO:
					{
						int target = arg->arg(0);
						if (targetRecord->arg(0) < target)
							return false;
					}
					break;
				case MissionTarget::LEVEL_TO:
					{
						int target = arg->arg(0);
						if (mission->component->roleAtt->fightAtt().level() < target)
							return false;
					}
					break;
				case MissionTarget::STRONG_TO:
					{
						int pos = arg->arg(0);
						int target = arg->arg(1);
						bool done = false;
						int64_t equip = mission->component->roleAtt->equipmentAtt().equipments(pos);
						if (equip >= 0) {
							EquipAsset *asset = Item_EquipAsset(mission->component->item, equip);
							if (asset != NULL)
								done = asset->strongLevel() >= target;
						}
						if (!done) {
							const ItemPackage *package = &mission->component->playerAtt->itemPackage();
							for (int i = ItemPackage::EQUIPMENT; i < ItemPackage::EQUIPMENT + package->validNumEquipment(); i++) {
								const ItemInfo *item = &package->items(i);
								if (item->type() == ItemInfo::EQUIPMENT) {
									EquipAsset *asset = Item_EquipAsset(mission->component->item, item->id());
									if (asset != NULL) {
										done = asset->strongLevel() >= target;
										if (done)
											break;
									}
								}
							}
						}
						if (!done)
							return false;
					}
					break;
				case MissionTarget::MOUNT:
					{
						int level = arg->arg(0);
						int count = arg->arg(1);
						int cur = 0;
						for (int i = 0; i < mission->component->roleAtt->equipmentAtt().equipments_size(); i++) {
							int64_t equip = mission->component->roleAtt->equipmentAtt().equipments(i);
							if (equip < 0)
								continue;
							EquipAsset *asset = Item_EquipAsset(mission->component->item, equip);
							if (asset == NULL)
								continue;
							for (int j = 0; j < asset->gemModel_size(); j++) {
								int gem = asset->gemModel(j);
								if (gem < 0)
									continue;
								const GoodsInfo *info = GoodsInfoManager_GoodsInfo(gem);
								if (info->arg(3) >= level) {
									cur++;
									if (cur >= count)
										break;
								}
							}
							if (cur >= count)
								break;
						}
						if (cur < count)
							return false;
					}
					break;
				case MissionTarget::STRONG_WING_TO:
					{
						int level = arg->arg(0);
						int degree = arg->arg(1);
						if (mission->component->roleAtt->fightAtt().baseWingLevel() < level)
							return false;
						else if (mission->component->roleAtt->fightAtt().baseWingLevel() == level && mission->component->roleAtt->fightAtt().baseWingDegree() < degree)
							return false;
					}
					break;
				case MissionTarget::UNLOCK_BLOODNODE_TO:
					{
						int target = arg->arg(0);
						if (mission->component->roleAtt->fightAtt().bloodNode() < target)
							return false;
					}
					break;
				case MissionTarget::SKILL_LEVEL_TO:
					{
						int count = arg->arg(0);
						int level = arg->arg(1);
						int cur = 0;
						for (int i = 0; i < mission->component->roleAtt->fightAtt().skills_size(); i++) {
							const Skill *skill = &mission->component->roleAtt->fightAtt().skills(i);
							if (skill->id() < 0)
								continue;
							if (skill->level() >= level) {
								cur++;
								if (cur >= count)
									break;
							}
						}
						if (cur < count)
							return false;
					}
					break;
				case MissionTarget::ENTER_ANY_ROOM:
					{
						int target = arg->arg(1);
						if (targetRecord->arg(0) < target)
							return false;
					}
					break;
				case MissionTarget::ADD_FRIEND:
					{
						int target = arg->arg(0);
						if (PlayerEntity_FriendsCount(mission->component->player) < target)
							return false;
					}
					break;
				case MissionTarget::BE_ADDED_FRIEND:
					{
						int target = arg->arg(0);
						if (PlayerEntity_FansCount(mission->component->player) < target)
							return false;
					}
					break;
				case MissionTarget::PETHALO_COUNT:
					{
						int count = arg->arg(1);
						const PlayerAtt *att = PlayerEntity_Att(mission->component->player);
						if (att == NULL)
							break;
						if (att->haloCount(arg->arg(0)) < count)
							return false;
					}
					break;
				case MissionTarget::PETHALO_LEVEL:
					{
						DEBUG_LOG("id == %d", id);
						DEBUG_LOG("arg0 == %d", arg->arg(0));
						DEBUG_LOG("arg1 == %d", arg->arg(1));
						int level = arg->arg(1);
						DEBUG_LOG("targetLevel == %d", level);
						const PlayerAtt *att = PlayerEntity_Att(mission->component->player);
						if (att == NULL)
							break;
						DEBUG_LOG("attLevel == %d", att->haloLevel(arg->arg(0)));
						if (att->haloLevel(arg->arg(0)) < level)
							return false;
					}
					break;
				case MissionTarget::RECHARGE_OPEN:
					{
						const PlayerAtt *att = PlayerEntity_Att(mission->component->player);
						if (att == NULL)
							break;
						if (mission->component->playerAtt->dayEvent(113) < arg->arg(0))
							return false;
					}
					break;
				case MissionTarget::STRONGALL_OPEN:
					{
						int32_t limit = arg->arg(0);
						const PlayerAtt *att = PlayerEntity_Att(mission->component->player);
						if (att == NULL)
							break;

						for (int i = 0; i < mission->component->roleAtt->equipmentAtt().equipments_size(); ++i) {
							int64_t equip = mission->component->roleAtt->equipmentAtt().equipments(i);
							if (equip < 0)
								return false;
							EquipAsset *asset = Item_EquipAsset(mission->component->item, equip);
							if (asset == NULL)
								return false;
							
							if (asset->strongLevel() < limit) 
								return false;
						}
					}
					break;
				case MissionTarget::TOWER_OPEN:
					{
						const PlayerAtt *att = PlayerEntity_Att(mission->component->player);
						if (att == NULL)
							break;
						int32_t maxTower = Fight_Att(mission->component->fight)->maxTower();
						if (maxTower < arg->arg(0))
							return false;
					}
					break;
				case MissionTarget::OBTRIDE_OPEN:
					{
						const ItemPackage *package = &mission->component->playerAtt->itemPackage();
						if (package == NULL)
							return false;

						int grade = arg->arg(0);
						int count = arg->arg(1);
						for (int i = 0; i < (int)package->rides_size(); i++) {
							const RidesInfo *ride = RidesInfoManager_RidesInfo(package->rides(i).model(), package->rides(i).star());
							if (ride == NULL)
								continue;

							if ((int)ride->quality() == grade) 
								count--;
						}

						if (count > 0) 
							return false;
					}
					break;
				case MissionTarget::LEVELUPRIDE_OPEN:
					{
						const ItemPackage *package = &mission->component->playerAtt->itemPackage();
						if (package == NULL)
							return false;

						int count = arg->arg(0);
						int level = arg->arg(1);
						for (int i = 0; i < (int)package->rides_size(); i++) {
							if (package->rides(i).model() == -1)
								continue;

							if (package->rides(i).level() >= level) 
								count--;
						}

						if (count > 0) 
							return false;
					}
					break;
				case MissionTarget::TRAINRIDE_OPEN:
					{
						const ItemPackage *package = &mission->component->playerAtt->itemPackage();
						if (package == NULL)
							return false;

						int star = arg->arg(0);
						int count = arg->arg(1);
						for (int i = 0; i < (int)package->rides_size(); i++) {
							if (package->rides(i).model() == -1)
								continue;

							if (package->rides(i).star() >= star) 
								count--;
						}

						if (count > 0) 
							return false;
					}
					break;
				case MissionTarget::PETEVOLUTION_OPEN:
					{
						int id = arg->arg(0);
						int quality = arg->arg(1);
						int level = arg->arg(2);
						
						const PlayerAtt *att = PlayerEntity_Att(mission->component->player);
						if (att == NULL)
							break;

						int i = 0;
						for (; i < (int)att->pets_size(); ++i) {
							if (id == att->pets(i).id()) {
								if (att->pets(i).quality() > quality) {
									break;
								}else if (att->pets(i).quality() == quality) {
									if (att->pets(i).level() >= level) {
										break;
									}
								}
							}
						}
						if (i == (int)att->pets_size()) 
							return false;
					}
					break;
				case MissionTarget::OBTGODSHIP_OPEN:
					{
						int count = arg->arg(0);
						int quality = arg->arg(1);
						const ItemPackage *package = &mission->component->playerAtt->itemPackage();
						if (package == NULL)
							return false;

						vector<int32_t> vec;
						for (int i = 0; i < (int)package->godShipsPackage_size(); ++i) {
							if (-1 != package->godShipsPackage(i)) {
								vec.push_back(package->godShipsPackage(i));
							}
						}
						
						for (int i = 0; i < mission->component->roleAtt->equipmentAtt().godShips_size(); ++i)
							if (-1 != mission->component->roleAtt->equipmentAtt().godShips(i)) {
								vec.push_back(mission->component->roleAtt->equipmentAtt().godShips(i));
							}

						for (size_t i = 0; i < vec.size(); ++i) {
							if (package->godShips(vec[i]).id() != -1) {
								if (package->godShips(vec[i]).quality() == quality)
									count--;
							}
						}
					
						if (count > 0)
							return false;
					}
					break;
				case MissionTarget::LEVELUPGODSHIP_OPEN:
					{
						int level = arg->arg(0);
						int count = arg->arg(1);
						const ItemPackage *package = &mission->component->playerAtt->itemPackage();
						if (package == NULL)
							return false;

						vector<int32_t> vec;
						for (int i = 0; i < (int)package->godShipsPackage_size(); ++i)
							if (-1 != package->godShipsPackage(i)) {
								vec.push_back(package->godShipsPackage(i));
							}

						for (int i = 0; i < mission->component->roleAtt->equipmentAtt().godShips_size(); ++i)
							if (-1 != mission->component->roleAtt->equipmentAtt().godShips(i)) {
								vec.push_back(mission->component->roleAtt->equipmentAtt().godShips(i));
							}

						for (size_t i = 0; i < vec.size(); ++i) {
						DEBUG_LOG("DDDDDD %d", vec[i]);
							if (package->godShips(vec[i]).id() != -1) {
								if (package->godShips(vec[i]).level() == level)
									count--;
							}
						}
					
						if (count > 0)
							return false;
					}
					break;
				case MissionTarget::JINJIEAWAKEN_OPEN:
					{
						int id = arg->arg(0);
						int quality = arg->arg(1);
						int level = arg->arg(2);
						DEBUG_LOG("FFFFFFFFFFFFFF id = %d, qu = %d, level = %d", id, quality, level);
						
						const ItemPackage *package = &mission->component->playerAtt->itemPackage();
						if (package == NULL)
							return false;
						
						for (int i = 0; i < package->transforms_size(); ++i) {
							if (id == package->transforms(i).id()) {
								DEBUG_LOG("FFFFFFFFFFFFFF id = %d, qu = %d, level = %d", id, package->transforms(i).quality(),  package->transforms(i).level());
								if (quality < package->transforms(i).quality()) {
									break;
								}else if(quality == package->transforms(i).quality()) {
									if (level > package->transforms(i).level())
										return false;
								}else {
									return false;
								}
							}
						}
						DEBUG_LOG("FFFFFFFFFFFFFF id = %d, qu = %d, level = %d", id, quality, level);

					}
					break;
				default:
					return false;
			}
		}

		return true;
	}

	return false;
}

int Mission_MissionCompleteCount(struct Mission *mission, int32_t id) {
	if (!Mission_IsValid(mission))
		return -1;

	MissionAllRecord *records = mission->record;
	if (id < 0 || id >= records->records_size())
		return -1;

	return records->records(id).count();
}

void Mission_CompleteMission(struct Mission *mission, int32_t id) {
	if (!Mission_IsValid(mission))
		return;

	MissionAllRecord *records = mission->record;
	if (id < 0 || id >= records->records_size())
		return;

	const MissionInfo *info = MissionInfoManager_MissionInfo(id);
	if (info == NULL)
		return;

	int n = 1;
	if (info->type() == MissionInfo::DAILY)
		n = Event_DailyMissionAward();

	const MissionAward *award = &info->award();

	for (int i = 0; i < records->cur_size(); i++) {
		if (records->cur(i) != id)
			continue;

		static NetProto_GetRes gr;
		gr.Clear();

		for (int j = 0; j < award->type_size(); j++) {
			MissionAward::Type type = award->type(j);
			if (type == MissionAward::NONE)
				continue;

			const MissionArg *arg = &award->arg(j);

			switch(type) {
				case MissionAward::EXP:
					{
						Fight_ModifyExp(mission->component->fight, arg->arg(0) * n);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::EXP);
						item->set_count(arg->arg(0));
					}
					break;

				case MissionAward::MONEY:
					{
						Item_ModifyMoney(mission->component->item, arg->arg(0) * n);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::MONEY);
						item->set_count(arg->arg(0));

						/*
						   if (mission->component->player != NULL) {
						   static NetProto_ModifyMoney modifyMoney;
						   modifyMoney.Clear();
						   modifyMoney.set_money(Item_Package(mission->component->item)->money());
						   int32_t player = PlayerEntity_ID(mission->component->player);
						   GCAgent_SendProtoToClients(&player, 1, &modifyMoney);
						   }
						   */
					}
					break;

				case MissionAward::DESIGNATION:
					{
						PlayerEntity_AddDesignation(mission->component->player, arg->arg(0));

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::DESIGNATION);
						item->set_id(arg->arg(0));
						item->set_count(1);
					}
					break;

				case MissionAward::RMB:
				case MissionAward::SUBRMB:
					{
						Item_ModifyRMB(mission->component->item, arg->arg(0) * n, false, -5, 0, 0);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::SUBRMB);
						item->set_count(arg->arg(0));
					}
					break;

				case MissionAward::SOUL:
					{
						Item_ModifySoul(mission->component->item, arg->arg(0) * n);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::SOUL);
						item->set_count(arg->arg(0));
					}
					break;

				case MissionAward::SOULJADE:
					{
						Item_ModifySoulJade(mission->component->item, ExploreInfo::SMALL, arg->arg(0) * n);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::SOULJADE);
						item->set_count(arg->arg(0));
					}
					break;

				case MissionAward::SOULSTONE:
					{
						Item_ModifySoulStone(mission->component->item, arg->arg(0) * n);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::SOULSTONE);
						item->set_count(arg->arg(0));
					}
					break;

				case MissionAward::HONOR:
					{
						Item_ModifyHonor(mission->component->item, arg->arg(0) * n);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::HONOR);
						item->set_count(arg->arg(0));
					}
					break;

				case MissionAward::DURABILITY:
					{
						Item_ModifyDurability(mission->component->item, arg->arg(0) * n);

						PB_ItemInfo *item = gr.add_items();
						item->set_type(PB_ItemInfo::DURABILITY);
						item->set_count(arg->arg(0));
					}
					break;

				case MissionAward::GOODS:
					{
						int32_t id = arg->arg(0);
						int32_t count = arg->arg(1);
						assert(GoodsInfoManager_GoodsInfo(id) != NULL && count > 0);
						Item_AddToPackage(mission->component->item, ItemInfo::GOODS, id, count * n, &gr);

						int64_t roleID = PlayerEntity_RoleID(mission->component->player);
						char ch[1024];
						SNPRINTF1(ch, "17-%d-%d", id, count * n);
						DCProto_SaveSingleRecord saveRecord;
						saveRecord.set_mapID(-23);
						saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
						saveRecord.mutable_record()->mutable_role()->set_name(ch);
						saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
						GCAgent_SendProtoToDCAgent(&saveRecord);
					}
					break;

				case MissionAward::EQUIPMENT:
					{
						int32_t id = arg->arg(0);
						const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(id);
						assert(equip != NULL);

						if (equip->professionType() != -1 && (ProfessionInfo::Type)equip->professionType() != mission->component->baseAtt->professionType())
							continue;

						Item_AddToPackage(mission->component->item, ItemInfo::EQUIPMENT, id, 1, &gr);

						int64_t roleID = PlayerEntity_RoleID(mission->component->player);
						char ch[1024];
						SNPRINTF1(ch, "18-%d-%d", id, 1);
						DCProto_SaveSingleRecord saveRecord;
						saveRecord.set_mapID(-23);
						saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
						saveRecord.mutable_record()->mutable_role()->set_name(ch);
						saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
						GCAgent_SendProtoToDCAgent(&saveRecord);
					}
					break;

				case MissionAward::ACTIVITY:
					{
						int32_t v = arg->arg(0);
						DEBUG_LOG("ACTIVITY: %d", v);
						if (v > 0) {
							PlayerEntity_SetDayEvent(mission->component->player, 69, mission->component->playerAtt->dayEvent(69) + v, true);
						}
					}
					break;

				case MissionAward::OPENSERVERSCORE:
					{
						int32_t v = arg->arg(0);
						if (v > 0) 
							Item_ModifyOpenServerScore(mission->component->item, v);
					}
					break;

				default:
					assert(0);
			}
		}

		int32_t player = PlayerEntity_ID(mission->component->player);
		if (gr.items_size() > 0)
			GCAgent_SendProtoToClients(&player, 1, &gr);

		if (n > 1 && info->type() == MissionInfo::DAILY) {
			static NetProto_Chat chat;
			chat.Clear();
			chat.set_channel(NetProto_Chat::SYSTEM);
			char buf[CONFIG_FIXEDARRAY];
			SNPRINTF1(buf, Config_Words(25), n - 1);
			chat.set_content(buf);
			GCAgent_SendProtoToClients(&player, 1, &chat);
		}

		records->mutable_records(id)->set_count(records->records(id).count() + 1);
		Mission_GiveUp(mission, id);

		char arg1[32];
		SNPRINTF1(arg1, "{\"id\":\"%d\"}", id);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "CommitTask", arg1));

		if (info->next() != -1) {
			if (Mission_CanApply(mission, info->next())) {
				Mission_Add(mission, info->next());

				static NetProto_ApplyMission applyMission;
				applyMission.Clear();

				applyMission.set_id(info->next());
				GCAgent_SendProtoToClients(&player, 1, &applyMission);
			}
			else {
				DEBUG_LOGERROR("Failed to apply next mission");
			}
		}

		return;
	}
}
