#ifndef _EQUIPMENT_HPP_
#define _EQUIPMENT_HPP_

#include "Component.hpp"
#include "EquipmentInfo.hpp"
#include "BaseInfo.hpp"
#include "ProfessionInfo.hpp"
#include "Fashion.pb.h"
#include "StatusInfo.pb.h"
#include <sys/types.h>

struct Equipment;

void Equipment_Init();

struct Equipment * Equipment_Create(EquipmentAtt *att, struct Component *component);
bool Equipment_IsValid(struct Equipment *equipment);
void Equipment_Destroy(struct Equipment *equipment);

void Equipment_Prepare(struct Equipment *equipment);

const EquipmentAtt * Equipment_Att(struct Equipment *equipment);

struct Component * Equipment_Component(struct Equipment *equipment);

// -2: error
// > -2: prev
int64_t Equipment_Wear(struct Equipment *equipment, int32_t id);
// -2: error
// > -2: prev
int64_t Equipment_Unwear(struct Equipment *equipment, EquipmentInfo::Type pos);

// -2: error
// > -2: prev
int32_t Equipment_WearFashion(struct Equipment *equipment, int32_t id);
// -2: error
// > -2: prev
int32_t Equipment_UnwearFashion(struct Equipment *equipment);

// -2: error
// > -2: prev
int32_t Equipment_WearWing(struct Equipment *equipment, int32_t id, bool baseWing);
// -2: error
// > -2: prev
int32_t Equipment_UnwearWing(struct Equipment *equipment);

bool Equipment_AddStatusToBody(struct Equipment *equipment, int32_t id, StatusInfo::StatusType type);
bool Equipment_DelStatusToBody(struct Equipment *equipment, int32_t id);
bool Equipment_EquipIsInBody(struct Equipment *equipment, int32_t id, ItemInfo::Type type);

int Equipment_Ride(struct Equipment *equipment, int32_t id);
int32_t Equipment_WearGodShip(struct Equipment *equipment, int playerLevel, int32_t type, int32_t index);
int32_t Equipment_UnWearGodShip(struct Equipment *equipment, int32_t index);

#endif
