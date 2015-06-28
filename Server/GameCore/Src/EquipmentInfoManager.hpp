#ifndef _EQUIPMENT_INFO_MANAGER_HPP_
#define _EQUIPMENT_INFO_MANAGER_HPP_

#include "EquipmentInfo.hpp"
#include <sys/types.h>

void EquipmentInfoManager_Init();

const EquipmentInfo * EquipmentInfoManager_EquipmentInfo(int64_t id);
const EquipRecipe * EquipmentInfoManager_EquipRecipe(int32_t id);

const BaseWing * EquipmentInfoManager_BaseWing(int level, int degree);
int EquipmentInfoManager_BaseWingMaxLevel();
const Wing * EquipmentInfoManager_Wing(int32_t id);

#endif
