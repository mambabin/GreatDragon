#ifndef _ITEM_HPP_
#define _ITEM_HPP_

#include "Component.hpp"
#include "ItemInfo.hpp"
#include "EquipmentInfo.hpp"
#include "BloodInfoManager.hpp"
#include <sys/types.h>

struct Item;

void Item_Init();

struct Item * Item_Create(ItemPackage *itemPackage, ALT *alt, struct Component *component);
bool Item_IsValid(struct Item *item);
void Item_Destroy(struct Item *item);

void Item_Prepare(struct Item *item);

const ItemPackage * Item_Package(struct Item *item);
const ALT * Item_ALT(struct Item *item);

struct Component * Item_Component(struct Item *item);

void Item_ModifyMoney(struct Item *item, int64_t delta);
void Item_ModifyVIP(struct Item *item, int32_t delta, int expire = 0);
void Item_SetVIP(struct Item *item, int32_t delta);

// reason
// -1: recharge
// -2: invest
// -3: use goods
// -4: open box
// -5: mission
// -6: mail
// -7: login obt rmb
// -8: god history
// -9: top up obt rmb
// -10: month card
// 0: none
// 1: add one blood node, arg1: blood node
// 2: add all blood nodes, arg1: first blood node
// 3: quick fight, arg1: room
// 4: invest, arg1: id
// 5: buy goods, arg1: id, arg2: count
// 6: recover durability
// 7: unlock package, arg1: count
// 8: explore
// 9: inspire, arg1: type
// 10: mail, arg1: target
// 11: gen gem, arg1: level, arg2: type
// 12: inherit
// 13: strong base wing, arg1: level, arg2: degree
// 14: buy wing, arg1: id, arg2: forever
// 15: buy fashion, arg1: id, arg2: forever
// 16: pet inherit, arg1: idPre, arg2: idAfter
// 17: revive
// 18: buy durability arg1: index
// 19: active1:
// 20: buy equip, arg1: id, arg2: count
// 21: faction
// 22: reset count, arg1: type
// 23: Treasure
// 24: money tree
// 25: buy vip box
// 26: random return rmb
// 27: blessing
// 28: use goods, arg1: id
// 29: tne gift
// 30 luck 
// 31: buy month card
// 32: buy goods gift
// 33: cat gift
// 34: group purchase
// 35: rides high train
// 36: reservation
// 37: survive
// 38: hero
// 39: active fiben
// 40: tower
// 41: clear quick fight time
void Item_ModifyRMB(struct Item *item, int64_t delta, bool recharge, int reason, int32_t arg1 = 0, int32_t arg2 = 0);

// way:
// 1: rmb
// 2: subrmb
// 3: above
bool Item_HasRMB(struct Item *item, int64_t v, int way = 3);

void Item_ModifySoul(struct Item *item, int64_t delta);
void Item_ModifySoulJade(struct Item *item, ExploreInfo::SoulJadeType type, int64_t delta);
void Item_ModifyHonor(struct Item *fight, int32_t delta);
void Item_ModifyDurability(struct Item *fight, int32_t delta, bool beyond = true);
void Item_ModifySoulStone(struct Item *item, int64_t delta);
void Item_ModifyPKScore(struct Item *item, int64_t delta);
void Item_ModifyGodScore(struct Item *item, int32_t delta);
void Item_ModifyLovePoint(struct Item *item, int32_t delta);
void Item_ModifyBlessScore(struct Item *item, int32_t delta);
void Item_ModifyOpenServerScore(struct Item *item, int32_t delta);

bool Item_IsValidPos(struct Item *item, int32_t pos);

const ItemInfo * Item_PackageItem(struct Item *item, int32_t pos);
// id: pos in package
// count -- -1: all
void Item_DelFromPackage(struct Item *item, int32_t pos, int count, bool delEquip = true, bool notice = false);
void Item_DelFromPackage(struct Item *item, ItemInfo::Type type, int64_t id, int count, bool notice = true);
void Item_DelFromPackage(struct Item *item, int32_t select, int64_t id);
// -1: error
// -2: exchange
// >= 0: items is not combined
int Item_AddToPackage(struct Item *item, const ItemInfo *cur, int32_t pos, ItemInfo *prev);
// if type is WING or FASHION
// count:
// 0: not forever
// 1: forever
int64_t Item_AddToPackage(struct Item *item, ItemInfo::Type type, int64_t id, int count, NetProto_GetRes *getRes = NULL);

int Item_UseGoods(struct Item *item, int32_t pos, bool all, int32_t *results, size_t size);

int Item_UnlockPackage(struct Item *item, ItemPackage::Begin begin, int count, bool free = false);

bool Item_HasItem(struct Item *item, ItemInfo::Type type, int64_t id, int min);
int32_t Item_EmptyPos(struct Item *item, ItemPackage::Begin begin, int64_t id);
bool Item_IsEnough(struct Item *item, ItemInfo::Type type, int64_t id, int count);
const ItemInfo * Item_Find(struct Item *item, ItemInfo::Type type, int64_t id);
int Item_Count(struct Item *item, ItemInfo::Type type, int64_t id);

void Item_SetCantUseGoods(struct Item *item, bool value);

const ItemInfo * Item_ALTItem(struct Item *item, int32_t id);
bool Item_HasALT(struct Item *item, ItemInfo::Type type, int64_t id);
void Item_DelAllFromALT(struct Item *item, ItemInfo::Type type, int64_t id);
void Item_DelFromALT(struct Item *item, int32_t id);
void Item_AddToALT(struct Item *item, const ItemInfo *cur, int32_t id);
int32_t Item_EmptyALT(struct Item *item);

int32_t Item_AddEquip(struct Item *item, const EquipmentInfo *info);
int32_t Item_AddEquip(PB_ItemPackage *package, const EquipmentInfo *info);
void Item_DelEquip(struct Item *item, int32_t id);
void Item_DelEquip(PB_ItemPackage *package, int32_t id);
EquipAsset * Item_EquipAsset(struct Item *item, int32_t id);
const EquipmentInfo * Item_EquipInfo(struct Item *item, int32_t id);

int32_t Item_Explore(struct Item *item, ExploreInfo::Type type, bool lucky);

int Item_Lottery(struct Item *item, int32_t box, int32_t *results, size_t size, bool notice = true, NetProto_GetRes *getRes = NULL);
int Item_ExchangeGoods(struct Item *item, int index, bool all);

// from: 0: room; 1: lottery; 2: goods
void Item_NoticeBox(struct Item *item, int32_t box, int32_t *results, int size, int from);

int Item_GetGift(struct Item *item, AwardInfo::Type type, int index, int arg);

// -1: error
// 0: arranged
// 1: nothing
int Item_Arrange(struct Item *item, ItemPackage::Begin begin);

// id: goods id
void Item_GenEquip(struct Item *item, int32_t id);

void Item_GenGem(struct Item *item, int level, int type, NetProto_GenGem::Way way);

void Item_UseBaseWing(struct Item *item, bool use);
void Item_UseWing(struct Item *item, int32_t id, bool use);
int Item_BuyWing(struct Item *item, int32_t id, bool forever, float discount = 1.0f, BusinessInfo::CurrencyType type = BusinessInfo::RMB);

void Item_UseFashion(struct Item *item, int32_t id, bool use);
int Item_BuyFashion(struct Item *item, int32_t id, bool forever, float discount = 1.0f, BusinessInfo::CurrencyType type = BusinessInfo::RMB);

bool Item_MakeFashionHole(struct Item *item, int32_t id, int32_t index);

bool Item_FashionInlay(struct Item *item, int32_t id, int32_t index, int32_t runeid, bool flag = true);

bool Item_FashionUnInlay(struct Item *item, int32_t id, int32_t index);

bool Item_BuyDurability(struct Item *item, int32_t  index);
int Item_FlyPlan(struct Item *item, NetProto_FlyPlan* info);
int Item_TransformLevelUp(struct Item* item, int id, bool activation = false);

int Item_PetHaloAttributeIncrease(struct Item* item, bool flag);

RidesAsset * Item_RidesAsset(struct Item *item, int32_t id);

void Item_AddToGodPackage(struct Item *item, int64_t id);
int32_t Item_UseGodShip(struct Item *item, int32_t index);
int32_t Item_UnUseGodShip(struct Item *item, int32_t index);
void Item_Swallow(struct Item *item, int32_t index, int32_t select);
void Item_SingleGodShip(struct Item *item, int32_t index, int32_t *eat, int32_t size);
void Item_ArrangeGodShip(struct Item *item);
void Item_GodShipAtt(struct Item *item, int32_t index, bool add);
int Item_RidesTrain(struct Item *item, int32_t index, bool high);

int Item_RidesInherit(struct Item *item, NetProto_RidesInherit proto);

int Item_RidesConfirmTrain(struct Item *item, NetProto_RidesConfirmTrain proto);

int Item_RidesUP(struct Item *item, NetProto_RidesUP proto);

int Item_RidesLockAtt(struct Item *item, NetProto_RidesLockAtt proto);

int32_t Item_RidesEmptyCount(struct Item *item);

int Item_GenRides_Whole(struct Item *item, int32_t id, int32_t count, NetProto_GetRes *gr);
void Item_ResetPvpScore(struct Item *item);
void Item_ModifyPKStatus(struct Item *item);

#endif
