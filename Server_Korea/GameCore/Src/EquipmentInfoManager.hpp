#ifndef _EQUIPMENT_INFO_MANAGER_HPP_
#define _EQUIPMENT_INFO_MANAGER_HPP_

#include "EquipmentInfo.hpp"
#include "EquipmentInfo.pb.h"
#include "EquipmentRandom.pb.h"
#include <sys/types.h>

void EquipmentInfoManager_Init();

const EquipmentInfo * EquipmentInfoManager_EquipmentInfo(int64_t id);
const EquipRecipe * EquipmentInfoManager_EquipRecipe(int32_t id);

const BaseWing * EquipmentInfoManager_BaseWing(int level, int degree);
int EquipmentInfoManager_BaseWingMaxLevel();
const Wing * EquipmentInfoManager_Wing(int32_t id);

void EquipmentInfoManager_GetRadomData(int level,
		EquipmentInfo_ColorType color,
		EquipmentInfo_Type pos,
		int* randomType,
		int* randomData);

int EquipmentInfoManager_GetRadomEffect(int id);

#endif
