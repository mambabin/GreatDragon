#include "Item.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "RidesInfoManager.hpp"
#include "ItemInfo.hpp"
#include "IDGenerator.hpp"
#include "Time.hpp"
#include "EquipmentInfo.hpp"
#include "GoodsInfoManager.hpp"
#include "EquipmentInfoManager.hpp"
#include "BloodInfoManager.hpp"
#include "BoxInfoManager.hpp"
#include "AwardInfoManager.hpp"
#include "VIPInfoManager.hpp"
#include "MapPool.hpp"
#include "MapInfoManager.hpp"
#include "GCAgent.hpp"
#include "FashionInfoManager.hpp"
#include "AccountPool.hpp"
#include "FactionPool.hpp"
#include "ScribeClient.hpp"
#include "TransformInfoManager.hpp"
#include "DCProto.pb.h"
#include "NPCInfoManager.hpp"
#include "GodShipInfoManager.hpp"
#include <sys/types.h>
#include <cmath>
#include <algorithm>
#include <map>
#include <vector>
#include <cstdio>
using namespace std;

#define MEDICINE_CD (1000 * 60 * 1)
#define UNLOCK_PACKAGE_PRICE 50

struct EquipComp{
	static struct Item *item;
	static int Compare(const void *lhs_, const void *rhs_) {
		const ItemInfo &lhs = *(const ItemInfo *)lhs_;
		const ItemInfo &rhs = *(const ItemInfo *)rhs_;
		if (lhs.type() != ItemInfo::EQUIPMENT)
			return 1;
		if (rhs.type() != ItemInfo::EQUIPMENT)
			return -1;
		const EquipmentInfo *lhsInfo = Item_EquipInfo(item, lhs.id());
		const EquipAsset *lhsAsset = Item_EquipAsset(item, lhs.id());
		/*
		if (lhsInfo == NULL) {
			DEBUG_LOGERROR("Failed to get equipment, id: %lld", lhs.id());
			Debug_LogCallStack();
			return false;
		}
		*/
		const EquipmentInfo *rhsInfo = Item_EquipInfo(item, rhs.id());
		const EquipAsset *rhsAsset = Item_EquipAsset(item, rhs.id());
		/*
		if (rhsInfo == NULL) {
			DEBUG_LOGERROR("Failed to get equipment, id: %lld", rhs.id());
			Debug_LogCallStack();
			return true;
		}
		*/
		if ((int)lhsInfo->colorType() > (int)rhsInfo->colorType())
			return -1;
		if (lhsInfo->equipmentLevel() > rhsInfo->equipmentLevel())
			return -1;
		if (lhsAsset->strongLevel() > rhsAsset->strongLevel())
			return -1;
		return 1;
	}
};
struct Item* EquipComp::item = NULL;

struct GoodsComp{
	static int Compare(const void *lhs_, const void *rhs_) {
		const ItemInfo &lhs = *(const ItemInfo *)lhs_;
		const ItemInfo &rhs = *(const ItemInfo *)rhs_;
		if (lhs.type() != ItemInfo::GOODS)
			return 1;
		if (rhs.type() != ItemInfo::GOODS)
			return -1;
		const GoodsInfo *lhsInfo = GoodsInfoManager_GoodsInfo(lhs.id());
		assert(lhsInfo != NULL);
		const GoodsInfo *rhsInfo = GoodsInfoManager_GoodsInfo(rhs.id());
		assert(rhsInfo != NULL);
		if ((int)lhsInfo->type() < (int)rhsInfo->type())
			return -1;
		if (lhsInfo->rmb() > rhsInfo->rmb())
			return -1;
		if (lhsInfo->price() > rhsInfo->price())
			return -1;
		return 1;
	}
};
/*
struct GodShipComp{
	static struct Item *item;
	static int Compare(const void *lhs_, const void *rhs_) {
		int lhs = *(int *)lhs_;
		int rhs = *(int *)rhs_;
		const GodShipAsset &lhsAsset = *(const GodShipAsset *)(&item->package->mutable_godShips(lhs_));
		const GodShipAsset &rhsAsset = *(const GodShipAsset *)(&item->package->mutable_godShips(rhs_));
		if (lhsAsset.quality() < rhsAsset.quality()) 
			return -1;
		if (lhsAsset.level() < rhsAsset.level())  
			return -1;
		if (lhsAsset.id() < rhsAsset.id())  
			return -1;
		return 1;
	}

};
struct Item* GodShipComp::item = NULL;
*/
struct Item{
	int32_t id;
	ItemPackage *package;
	ALT *alt;
	struct Component *component;
	int64_t medicineCD;
	bool cantUseGoods;
	bool equipDirty;
	bool goodsDirty;
	bool gemDirty;
	int32_t ridesTrainDelta[6];
	int32_t ridesTrain;

	Item()
	{
		ridesTrain = -1;
	}
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct Item *> pool;
	vector<pair<BusinessInfo::CurrencyType, int> > numToCost;
}package;


void Item_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
	package.numToCost.clear();
	{
		string name = Config_DataPath() + string("/PackageCost.txt");
		FILE *file = fopen(name.c_str(), "r");
		if (file == NULL) {
			DEBUG_LOGERROR("Failed to open file: %s", name.c_str());
			exit(EXIT_FAILURE);
		}

		for (int i = 0; ; i++) {
			char key[16];
			SNPRINTF1(key, "%d", i);
			char line[CONFIG_FIXEDARRAY];
			if (!ConfigUtil_ReadStr(file, key, line, CONFIG_FIXEDARRAY)) {
				break;
			}

			char *tokens[CONFIG_FIXEDARRAY];
			int count = ConfigUtil_ExtraToken(line, tokens, CONFIG_FIXEDARRAY, ",");
			if (count == -1) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}

			int type = (int)atoi(tokens[0]);
			int cost = (int)atoi(tokens[1]);

			package.numToCost.push_back(make_pair((::BusinessInfo::CurrencyType)type, cost));

		}
		fclose(file);
	}
}

struct Item * Item_Create(ItemPackage *itemPackage, ALT *alt, struct Component *component) {
	if (itemPackage == NULL || alt == NULL || component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct Item *item = SearchBlock(package.pool, id);
	item->id = id;
	item->package = itemPackage;
	item->alt = alt;
	item->component = component;
	item->medicineCD = -MEDICINE_CD;
	item->cantUseGoods = false;
	item->equipDirty = true;
	item->goodsDirty = true;
	item->gemDirty = true;

	return item;
}

bool Item_IsValid(struct Item *item) {
	return item != NULL && IDGenerator_IsValid(package.idGenerator, item->id);
}


void Item_Prepare(struct Item *item) {
	if (!Item_IsValid(item))
		return;

	if (item->package->validNumGoods() < 0)
		item->package->set_validNumGoods(ItemPackage::LENGTH / 2);

	Item_UseBaseWing(item, true);
	for (int i = 0; i < item->package->wings_size(); i++)
		Item_UseWing(item, i, true);
	for (int i = 0; i < item->package->fashions_size(); i++)
		Item_UseFashion(item, i, true);

	Item_PetHaloAttributeIncrease(item, true);

	const EquipmentAtt *att =  Equipment_Att(item->component->equipment);
	if (att == NULL)
		return;

	for (int i = 0; i < att->godShips_size(); i++) {

		int index = att->godShips(i);
		if (index == -1)
			break;
		Item_GodShipAtt(item, index, true);
	}
}

const ItemPackage * Item_Package(struct Item *item) {
	if (!Item_IsValid(item))
		return NULL;

	return item->package;
}

const ALT * Item_ALT(struct Item *item) {
	if (!Item_IsValid(item))
		return NULL;

	return item->alt;
}

void Item_Destroy(struct Item *item) {
	if (!Item_IsValid(item))
		return;

	IDGenerator_Release(package.idGenerator, item->id);
}

struct Component * Item_Component(struct Item *item) {
	if (!Item_IsValid(item))
		return NULL;

	return item->component;
}

void Item_ModifyMoney(struct Item *item, int64_t delta) {
	if (!Item_IsValid(item))
		return;

	item->package->set_money(item->package->money() + delta);
	if (item->package->money() < 0)
		item->package->set_money(0);

	if (delta != 0) {
		static NetProto_ModifyMoney modifyMoney;
		modifyMoney.Clear();
		modifyMoney.set_money(item->package->money());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifyMoney);
	}

	if (Config_ScribeHost() != NULL) {
		if (delta != 0) {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;
			
			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"moneyChange\":\"%lld", (long long int)delta);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"money\":\"%lld", (long long int)item->package->money());
			index += strlen(index);
			SNPRINTF2(buf, index, "\"}");
			int32_t player = PlayerEntity_ID(item->component->player);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "MoneyChange", buf));
		}
	}
}

void Item_ModifyVIP(struct Item *item, int32_t delta, int expire) {
	if (!Item_IsValid(item))
		return;

	if (delta != 0) {
		item->package->set_vip(item->package->vip() + delta);
		if (item->package->vip() < 0)
			item->package->set_vip(0);

		static NetProto_ModifyVIP modifyVIP;
		modifyVIP.Clear();
		int32_t player = PlayerEntity_ID(item->component->player);
		modifyVIP.set_player(player);
		modifyVIP.set_vip(item->package->vip());
		GCAgent_SendProtoToClients(&player, 1, &modifyVIP);

		if (expire != item->component->playerAtt->fixedEvent(69)) {
			PlayerEntity_SetFixedEvent(item->component->player, 69, expire, true);
		}
	}
	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"vipLevel\":\"%d", item->package->vip());
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		int32_t player = PlayerEntity_ID(item->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "VipLevel", buf));
	}

}

void Item_SetVIP(struct Item *item, int32_t delta) {
	if (!Item_IsValid(item))
		return;

	item->package->set_vip(delta);
	if (item->package->vip() < 0) {
		item->package->set_vip(0);
	}

	static NetProto_ModifyVIP modifyVIP;
	modifyVIP.Clear();
	int32_t player = PlayerEntity_ID(item->component->player);
	modifyVIP.set_player(player);
	modifyVIP.set_vip(item->package->vip());
	GCAgent_SendProtoToClients(&player, 1, &modifyVIP);
}


static void ModifySubRMB(struct Item *item, int64_t delta) {
	item->package->set_subRMB(item->package->subRMB() + delta);
	if (item->package->subRMB() < 0) {
		item->package->set_subRMB(0);
	}

	if (delta != 0) {
		static NetProto_ModifySubRMB modifySubRMB;
		modifySubRMB.Clear();
		modifySubRMB.set_value(item->package->subRMB());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifySubRMB);
	}
}

static void RecordRMB(struct Item *item, int64_t rmbValue, int64_t subrmbValue, int reason, int32_t arg1, int32_t arg2) {
	const PlayerInfo *info = AccountPool_IDToAccount(PlayerEntity_ID(item->component->player));
	if (info == NULL)
		return;

	static DCProto_CostRecord cr;
	cr.Clear();
	cr.set_role(item->component->roleAtt->baseAtt().roleID());
	cr.set_level(item->component->roleAtt->fightAtt().level());
	*cr.mutable_info() = *info;
	cr.set_rmbValue(rmbValue);
	cr.set_subrmbValue(subrmbValue);
	cr.set_rmb(item->package->rmb());
	cr.set_subRMB(item->package->subRMB());
	switch(reason) {
		case 1:
			cr.set_reason("add one blood node");
			break;
		case 2:
			cr.set_reason("add all blood nodes");
			break;
		case 3:
			cr.set_reason("quick fight");
			break;
		case 4:
			cr.set_reason("invest");
			break;
		case 5:
			cr.set_reason("buy goods");
			break;
		case 6:
			cr.set_reason("recover durability");
			break;
		case 7:
			cr.set_reason("unlock package");
			break;
		case 8:
			cr.set_reason("explore");
			break;
		case 9:
			cr.set_reason("inspire");
			break;
		case 10:
			cr.set_reason("mail");
			break;
		case 11:
			cr.set_reason("gen gem");
			break;
		case 12:
			cr.set_reason("inherit");
			break;
		case 13:
			cr.set_reason("strong base wing");
			break;
		case 14:
			cr.set_reason("buy wing");
			break;
		case 15:
			cr.set_reason("buy fashion");
			break;
		case 16:
			cr.set_reason("pet inherit");
			break;
		case 17:
			cr.set_reason("revive");
			break;
		case 18:
			cr.set_reason("buy durability");
			break;
		case 19:
			cr.set_reason("active1");
			break;
		case 20:
			cr.set_reason("buy equip");
			break;
		case 21:
			cr.set_reason("faction");
			break;
		case 22:
			cr.set_reason("reset count");
			break;
		case 23:
			cr.set_reason("treasure");
			break;
		case 24:
			cr.set_reason("money tree");
			break;
		case 25:
			cr.set_reason("buy vip box");
			break;
		case 26:
			cr.set_reason("random return rmb");
			break;
		case 27:
			cr.set_reason("blessing");
			break;
		case 28:
			cr.set_reason("use goods");
			break;
		case 29:
			cr.set_reason("ten gift");
			break;
		case 30:
			cr.set_reason("luck");
			break;
		case 31:
			cr.set_reason("buy month card");
			break;
		case 32:
			cr.set_reason("buy goods gift");
			break;
		case 33:
			cr.set_reason("cat gift");
			break;
		case 34:
			cr.set_reason("group purchase");
			break;
		case 36:
			cr.set_reason("reservation");
			break;

		case -1:
			cr.set_reason("recharge");
			break;
		case -2:
			cr.set_reason("invest");
			break;
		case -3:
			cr.set_reason("use goods");
			break;
		case -4:
			cr.set_reason("open box");
			break;
		case -5:
			cr.set_reason("mission");
			break;
		case -6:
			cr.set_reason("mail");
			break;
		case -7:
			cr.set_reason("login obt rmb");
			break;
		case -8:
			cr.set_reason("god history");
			break;
		case -9:
			cr.set_reason("top up obt rmb");
			break;
		case -10:
			cr.set_reason("month card");
			break;

		default:
			assert(0);
	}
	cr.set_arg1(arg1);
	cr.set_arg2(arg2);
	GCAgent_SendProtoToDCAgent(&cr);
}

void Item_ModifyRMB(struct Item *item, int64_t delta, bool recharge, int reason, int32_t arg1, int32_t arg2) {
	if (!Item_IsValid(item))
		return;

	if (delta > 0) {
		if (recharge) {
			PlayerEntity_SetFixedEvent(item->component->player, 15, item->component->playerAtt->fixedEvent(15) | (1 << 1));
			if(Config_DoubleRecharge(delta) && !(item->component->playerAtt->fixedEvent(15) & (1 << Config_DoubleRechargeLevel(delta)))) {
				PlayerEntity_SetFixedEvent(item->component->player, 15, item->component->playerAtt->fixedEvent(15) | (1 << Config_DoubleRechargeLevel(delta)));
				Item_ModifyRMB(item, delta, false, -1, 0, 0);
			}	

			//Event_NewYearGift(item, delta);
			Event_AwardFromRecharge(item, delta);


			int64_t rmb = delta / Config_RMBToGem();
/*			const vector<MailGift> *mailGift = Config_MailGift(MailGift::RECHARGE);
			if (mailGift != NULL) {
				for (size_t i = 0; i < mailGift->size(); i++) {
					const MailGift *unit = &(*mailGift)[i];
					const PlayerAtt* att = PlayerEntity_Att(item->component->player);
					if (att == NULL)
						continue;
					DEBUG_LOG("createTime: %d, rmb: %lld, totalRMB: %lld", att->createTime(), rmb, item->package->totalRMB());
					DEBUG_LOG("shijianchuo: %d, recharge: %lld", unit->arg3(), unit->arg2());
					if (att->createTime() < unit->arg3() && unit->arg2() <= item->package->totalRMB() + rmb && unit->arg2() > item->package->totalRMB() ) {
						static PB_MailInfo mail;
						mail = unit->mail();
						int index = PlayerEntity_AddMail(item->component->player, &mail);
						if (index != -1) {
							static NetProto_SendMail sm;
							sm.Clear();
							sm.set_receiver(att->att().baseAtt().roleID());
							sm.set_pos(index);
							*sm.mutable_mail() = mail;
							int32_t player = PlayerEntity_ID(item->component->player);
							GCAgent_SendProtoToClients(&player, 1, &sm);
						}
					}
				}
			}
*/
			item->package->set_rmb(item->package->rmb() + delta);

			item->package->set_totalRMB(item->package->totalRMB() + rmb);
			// playoff
			Item_ModifyVIP(item, VIPInfoManager_MatchVIP(item->package->totalRMB() * Config_RMBToGem()) - item->package->vip());
			if (Config_InActiveTime(10))
				PlayerEntity_SetFixedEvent(item->component->player, 8, item->component->playerAtt->fixedEvent(8) + rmb, true);

			if (Config_InActiveTime(19))
				PlayerEntity_SetFixedEvent(item->component->player, 96, item->component->playerAtt->fixedEvent(96) + rmb, true);

			const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::UNIT_RECHARGE, 0);
			if (award != NULL) {
				if (rmb >= award->arg()) {
					if ((item->component->playerAtt->dayEvent(5) & (1 << 5)) == 0) {
						PlayerEntity_SetDayEvent(item->component->player, 5, item->component->playerAtt->dayEvent(5) | (1 << 4));
					}
				}
			}

			if (reason == -1) {
				const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
				if (fixedMap != NULL) {
					multimap<int, int64_t>::const_iterator it = fixedMap->lower_bound(-6);
					if (it != fixedMap->end()) {
						int endTime = item->component->playerAtt->fixedEvent(55);
						if (endTime > Time_TimeStamp()) {
							item->package->set_rmbActive(item->package->rmbActive() + delta);
						}else {
							PlayerEntity_SetFixedEvent(item->component->player, 55, int(it->second & 0xFFFFFFFF));
							item->package->set_rmbActive(delta);
						}
					} else {
						item->package->set_rmbActive(0);
					}
				}
				static NetProto_ModifyRMBActive modifyRMBActive;
				modifyRMBActive.Clear();
				modifyRMBActive.set_value(item->package->rmbActive());
				int32_t id = PlayerEntity_ID(item->component->player);
				GCAgent_SendProtoToClients(&id, 1, &modifyRMBActive);

				// 丘比特
				if (Config_InActiveTime(16)) {
					int yuanbao = item->component->playerAtt->fixedEvent(86);
					const vector<const AwardInfo *> * award = AwardInfoManager_AllAwardInfo(AwardInfo::QIUBITE, yuanbao, delta);
					if (!(award == NULL || award->size() == 0)) {
						for (int i = 0; i < (int)award->size(); ++i) {
							PlayerEntity_AddMail(item->component->player, ItemInfo::GOODS, (*award)[i]->award(), 1, (*award)[i]->name().c_str(), (*award)[i]->content().c_str());
						}
					}
					PlayerEntity_SetFixedEvent(item->component->player, 86, yuanbao + delta);
					PlayerEntity_SetFixedEvent(item->component->player, 87, Time_TimeStamp());
				}else {
					PlayerEntity_SetFixedEvent(item->component->player, 86, 0);
					PlayerEntity_SetFixedEvent(item->component->player, 87, 0);
				}
			}

			const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(item->component->roleAtt->movementAtt().mapID()));
			if (mapInfo != NULL && mapInfo->mapType() == MapInfo::PEACE) {
				static DCProto_SaveRoleData saveRoleData;
				saveRoleData.Clear();
				PlayerEntity_Save(item->component->player, &saveRoleData);
				GCAgent_SendProtoToDCAgent(&saveRoleData);
			}

			static NetProto_ModifyRMB modifyRMB;
			modifyRMB.Clear();
			modifyRMB.set_value(item->package->rmb());
			modifyRMB.set_total(item->package->totalRMB());
			modifyRMB.set_totalCost(item->package->totalCost());
			int32_t id = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&id, 1, &modifyRMB);

			if (Config_ScribeHost() != NULL) {
				static char buf[CONFIG_FIXEDARRAY];
				memset(buf, 0, sizeof(buf) / sizeof(char));
				char *index = buf;

				SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"deltaRecharge\":\"%lld", (long long int)delta);
				index += strlen(index);
				SNPRINTF2(buf, index, "\"}");
				int32_t player = PlayerEntity_ID(item->component->player);
				ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "DeltaRecharge", buf));
			}
			if (reason != 0 && delta != 0)
				RecordRMB(item, -delta, 0, reason, arg1, arg2);
		} else {
			ModifySubRMB(item, delta);
			if (reason != 0 && delta != 0)
				RecordRMB(item, 0, -delta, reason, arg1, arg2);
		}
	} else if (delta < 0) {
		if (recharge) {
			int32_t id = PlayerEntity_ID(item->component->player);
			const PlayerInfo *info = AccountPool_IDToAccount(id);
			if (info != NULL) {
				static DCProto_Cost cost;
				cost.set_v(-delta);
				*cost.mutable_info() = *info;
				GCAgent_SendProtoToDCAgent(&cost);
			}

			if (Config_InActiveTime(26)) {
				if (item->package->activeCostEndTime() > Time_TimeStamp()) {
					item->package->set_activeCost(item->package->activeCost() - delta);
				}else {
					item->package->set_activeCost(-delta);

					const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
					if (fixedMap != NULL) {
						multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-26);
						if (begin != fixedMap->end()) {
							item->package->set_activeCostEndTime(begin->second & 0xFFFFFFFF);
						}
					}
				}

				if (item->package->activeCost() > 300) {
					int count = item->package->activeCost() / 300;
					item->package->set_activeCost(item->package->activeCost() - 300 * count);
					PlayerPool_GrabRedEnvelope(item->component->roleAtt->baseAtt().roleID(), count * 1000);
				}
			}

			Event_AwardConsume(item, -delta);
			Event_AwardFromSpend(item, -delta);
			Event_AwardFromSky_Cost(item, -delta);
			item->package->set_rmb(item->package->rmb() + delta);
			if (item->package->rmb() < 0)
				item->package->set_rmb(0);

			item->package->set_totalCost(item->package->totalCost() - delta);

			static NetProto_ModifyRMB modifyRMB;
			modifyRMB.Clear();
			modifyRMB.set_value(item->package->rmb());
			modifyRMB.set_total(item->package->totalRMB());
			modifyRMB.set_totalCost(item->package->totalCost());
			GCAgent_SendProtoToClients(&id, 1, &modifyRMB);

			if (Config_ScribeHost() != NULL) {
				static char buf[CONFIG_FIXEDARRAY];
				memset(buf, 0, sizeof(buf) / sizeof(char));
				char *index = buf;

				SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"deltaSpendRMB\":\"%lld", (long long int)(-delta));
				index += strlen(index);
				SNPRINTF2(buf, index, "\"}");
				int32_t player = PlayerEntity_ID(item->component->player);
				ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "DeltaSpendRMB", buf));
			}
			if (reason != 0 && delta != 0)
				RecordRMB(item, -delta, 0, reason, arg1, arg2);
		} else {
			int64_t subRMB = (-delta) > item->package->subRMB() ? item->package->subRMB() : (-delta);
			int64_t rmb = (-delta) > item->package->subRMB() ? ((-delta) - item->package->subRMB()) : 0;
			ModifySubRMB(item, -subRMB);
			if (reason != 0 && subRMB != 0)
				RecordRMB(item, 0, subRMB, reason, arg1, arg2);
			if (rmb > 0) {
				Item_ModifyRMB(item, -rmb, true, 0, 0, 0);
			}

			if (Config_ScribeHost() != NULL) {
				static char buf[CONFIG_FIXEDARRAY];
				memset(buf, 0, sizeof(buf) / sizeof(char));
				char *index = buf;

				SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"deltaSpend\":\"%lld", (long long int)(-delta));
				index += strlen(index);
				SNPRINTF2(buf, index, "\"}");
				int32_t player = PlayerEntity_ID(item->component->player);
				ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "DeltaSpend", buf));
			}
		}
	}
}

bool Item_HasRMB(struct Item *item, int64_t v, int way) {
	if (!Item_IsValid(item))
		return false;
	if (v < 0)
		return false;

	int64_t total = 0;
	if (way & 1) {
		total += item->package->rmb();
	}
	if (way & 2) {
		total += item->package->subRMB();
	}
	return total >= v;
}

void Item_ModifySoul(struct Item *item, int64_t delta) {
	if (!Item_IsValid(item))
		return;

	item->package->set_soul(item->package->soul() + delta);
	if (item->package->soul() < 0)
		item->package->set_soul(0);

	if (delta != 0) {
		static NetProto_ModifySoul modifySoul;
		modifySoul.Clear();
		modifySoul.set_value(item->package->soul());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifySoul);
	}
}

void Item_ModifySoulJade(struct Item *item, ExploreInfo::SoulJadeType type, int64_t delta) {
	if (type == ExploreInfo::SMALL) {
		item->package->set_smallSoul(item->package->smallSoul() + delta);
		if (item->package->smallSoul() < 0)
			item->package->set_smallSoul(0);

		if (delta != 0) {
			static NetProto_ModifySoulJade modifySoulJade;
			modifySoulJade.Clear();
			modifySoulJade.set_value(item->package->smallSoul());
			int32_t id = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&id, 1, &modifySoulJade);
		}
	} else if (type == ExploreInfo::MEDIUM) {
		item->package->set_mediumSoul(item->package->mediumSoul() + delta);
		if (item->package->mediumSoul() < 0)
			item->package->set_mediumSoul(0);
	} else if (type == ExploreInfo::BIG) {
		item->package->set_bigSoul(item->package->bigSoul() + delta);
		if (item->package->bigSoul() < 0)
			item->package->set_bigSoul(0);
	} else if (type == ExploreInfo::PERFECT) {
		item->package->set_perfectSoul(item->package->perfectSoul() + delta);
		if (item->package->perfectSoul() < 0)
			item->package->set_perfectSoul(0);
	}
}

void Item_ModifyHonor(struct Item *item, int32_t delta) {
	if (!Item_IsValid(item))
		return;

	item->package->set_honor(item->package->honor() + delta);
	if (item->package->honor() < 0)
		item->package->set_honor(0);

	if (delta != 0) {
		static NetProto_ModifyHonor modifyHonor;
		modifyHonor.Clear();
		modifyHonor.set_value(item->package->honor());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifyHonor);
	}
}

void Item_ModifyDurability(struct Item *item, int32_t delta, bool beyond) {
	if (!Item_IsValid(item))
		return;

	int max = Config_MaxDurability(item->package->vip());
	int v = item->package->durability() + delta;
	if (v < 0) {
		v = 0;
	} else if (item->package->durability() == max && delta > 0) {
		if (!beyond)
			return;
	} else if (v > max && delta > 0) {
		if (!beyond) {
			delta = max - item->package->durability();
			if (delta <= 0) {
				return;
			} else {
				v = item->package->durability() + delta;
			}
		}
	}
	item->package->set_durability(v);

	if (delta != 0) {
		static NetProto_ModifyDurability modifyDurability;
		modifyDurability.Clear();
		modifyDurability.set_value(item->package->durability());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifyDurability);
	}
}

void Item_ModifySoulStone(struct Item *item, int64_t delta) {
	if (!Item_IsValid(item))
		return;

	item->package->set_soulStone(item->package->soulStone() + delta);
	if (item->package->soulStone() < 0)
		item->package->set_soulStone(0);

	if (delta != 0) {
		static NetProto_ModifySoulStone modifySoulStone;
		modifySoulStone.Clear();
		modifySoulStone.set_value(item->package->soulStone());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifySoulStone);
	}
}

void Item_ModifyPKScore(struct Item *item, int64_t delta) {
	if (!Item_IsValid(item))
		return;

	item->package->set_pkScore(item->package->pkScore() + delta);
	if (item->package->pkScore() < 0)
		item->package->set_pkScore(0);

	if (delta != 0) {
		static NetProto_ModifyPKScore modifyPKScore;
		modifyPKScore.Clear();
		modifyPKScore.set_value(item->package->pkScore());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifyPKScore);
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap != NULL) {
		multimap<int, int64_t>::const_iterator it = fixedMap->lower_bound(-5);
		if (it != fixedMap->end()) {
			int endTime = item->component->playerAtt->fixedEvent(54);
			if (endTime > Time_TimeStamp()) {
				item->package->set_pkScoreActive(item->package->pkScoreActive() + delta);
			}else {
				PlayerEntity_SetFixedEvent(item->component->player, 54, int(it->second & 0xFFFFFFFF));
				item->package->set_pkScoreActive(delta);
			}
		}else {
			item->package->set_pkScoreActive(0);
		}

		if (delta != 0) {
			static NetProto_ModifyPKScoreActive modifyPKScoreActive;
			modifyPKScoreActive.Clear();
			modifyPKScoreActive.set_value(item->package->pkScoreActive());
			int32_t id = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&id, 1, &modifyPKScoreActive);
		}
	}
}

void Item_ModifyGodScore(struct Item *item, int32_t delta) {
	if (!Item_IsValid(item))
		return;

	item->package->set_godScore(item->package->godScore() + delta);
	if (item->package->godScore() < 0)
		item->package->set_godScore(0);

	if (delta != 0) {
		static NetProto_ModifyGodScore modifyGodScore;
		modifyGodScore.Clear();
		modifyGodScore.set_value(item->package->godScore());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifyGodScore);
	}
}

bool Item_IsValidPos(struct Item *item, int32_t pos) {
	if (!Item_IsValid(item))
		return false;

	if (pos >= (int)item->package->EQUIPMENT && pos < (int)item->package->EQUIPMENT + (int)ItemPackage::LENGTH) {
		if (pos - (int)ItemPackage::EQUIPMENT >= item->package->validNumEquipment())
			return false;
	} else if (pos >= (int)item->package->GOODS && pos < (int)item->package->GOODS + (int)ItemPackage::LENGTH) {
		if (pos - (int)ItemPackage::GOODS >= item->package->validNumGoods())
			return false;
	} else if (pos >= (int)item->package->GEM && pos < (int)item->package->GEM + (int)ItemPackage::LENGTH) {
		if (pos - (int)ItemPackage::GEM >= item->package->validNumGem())
			return false;
	} else {
		return false;
	}

	return true;
}

const ItemInfo * Item_PackageItem(struct Item *item, int32_t pos) {
	if (!Item_IsValid(item))
		return NULL;
	if (!Item_IsValidPos(item, pos))
		return NULL;

	return &item->package->items(pos);
}

void Item_DelFromPackage(struct Item *item, int32_t pos, int count, bool delEquip, bool notice) {
	if (!Item_IsValid(item))
		return;
	if (!Item_IsValidPos(item, pos))
		return;
	if (count < -1)
		return;

	ItemInfo *info = item->package->mutable_items(pos);
	if (info->type() == ItemInfo::NONE)
		return;

	if (count == -1) {
		if (info->type() == ItemInfo::EQUIPMENT) {
			item->equipDirty = true;
			if (delEquip)
				Item_DelEquip(item, info->id());
		} else if (info->type() == ItemInfo::GOODS) {
			if (GoodsInfoManager_IsGem(info->id()))
				item->gemDirty = true;
			else
				item->goodsDirty = true;
		}

		info->set_type(ItemInfo::NONE);
	}
	else {
		info->set_count(info->count() - count);
		if (info->count() <= 0) {
			if (info->type() == ItemInfo::EQUIPMENT) {
				if (delEquip)
					Item_DelEquip(item, info->id());
				item->equipDirty = true;
			} else if (info->type() == ItemInfo::GOODS) {
				if (GoodsInfoManager_IsGem(info->id()))
					item->gemDirty = true;
				else
					item->goodsDirty = true;
			}

			info->set_type(ItemInfo::NONE);
		}
	}

	if (notice) {
		static NetProto_DelItem di;
		di.Clear();
		di.set_pos(pos);
		di.set_count(info->type() == ItemInfo::NONE ? 0 : info->count());
		int32_t player = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&player, 1, &di);
	}
}

void Item_DelFromPackage(struct Item *item, ItemInfo::Type type, int64_t id, int count, bool notice) {
	if (!Item_IsValid(item))
		return;
	if (count <= 0)
		return;

	int i = 0;
	int end = 0;
	if (type == ItemInfo::EQUIPMENT) {
		i = (int)ItemPackage::EQUIPMENT;
		end = item->package->validNumEquipment() + i;
	} else if (type == ItemInfo::GOODS) {
		if (GoodsInfoManager_IsGem(id)) {
			i = (int)ItemPackage::GEM;
			end = item->package->validNumGem() + i;
		} else {
			i = (int)ItemPackage::GOODS;
			end = item->package->validNumGoods() + i;
		}
	} else {
		return;
	}

	int total = 0;
	for (; i < end; i++) {
		ItemInfo *cur = item->package->mutable_items(i);
		if (cur->type() == type && cur->id() == id) {
			total += cur->count();
			if (total >= count) {
				cur->set_count(total - count);
				if (cur->count() <= 0) {
					if (cur->type() == ItemInfo::EQUIPMENT) {
						item->equipDirty = true;
						Item_DelEquip(item, cur->id());
					} else if (cur->type() == ItemInfo::GOODS) {
						if (GoodsInfoManager_IsGem(cur->id()))
							item->gemDirty = true;
						else
							item->goodsDirty = true;
					}

					cur->set_type(ItemInfo::NONE);
					cur->set_id(-1);
					cur->set_count(0);
				}

				if (notice) {
					static NetProto_DelItem di;
					di.Clear();
					di.set_pos(i);
					di.set_count(cur->count());
					int32_t player = PlayerEntity_ID(item->component->player);
					GCAgent_SendProtoToClients(&player, 1, &di);
				}
				return;
			} else {
				if (cur->type() == ItemInfo::EQUIPMENT) {
					item->equipDirty = true;
					Item_DelEquip(item, cur->id());
				} else if (cur->type() == ItemInfo::GOODS) {
					if (GoodsInfoManager_IsGem(cur->id()))
						item->gemDirty = true;
					else
						item->goodsDirty = true;
				}

				cur->set_type(ItemInfo::NONE);
				cur->set_id(-1);
				cur->set_count(0);

				if (notice) {
					static NetProto_DelItem di;
					di.Clear();
					di.set_pos(i);
					di.set_count(cur->count());
					int32_t player = PlayerEntity_ID(item->component->player);
					GCAgent_SendProtoToClients(&player, 1, &di);
				}
			}
		}
	}
}

int Item_AddToPackage(struct Item *item, const ItemInfo *cur, int32_t pos, ItemInfo *prev) {
	if (!Item_IsValid(item) || cur == NULL || cur->type() == ItemInfo::SKILL)
		return -1;
	if (!Item_IsValidPos(item, pos))
		return -1;

	if (pos >= (int)ItemPackage::GEM && pos < ((int)ItemPackage::GEM + item->package->validNumGem())) {
		if (cur->type() != ItemInfo::GOODS || !GoodsInfoManager_IsGem(cur->id()))
			return -1;
	}

	ItemInfo *prev_ = item->package->mutable_items(pos);

	if (cur->type() == ItemInfo::GOODS) {
		if (prev_->type() == ItemInfo::GOODS) {
			if (cur->id() == prev_->id()) {
				const GoodsInfo *info = GoodsInfoManager_GoodsInfo(prev_->id());
				if (prev_->count() < info->repeat()) {
					prev_->set_count(prev_->count() + cur->count());
					if (prev_->count() > info->repeat())
					{
						int ret = prev_->count() - info->repeat();
						prev_->set_count(info->repeat());
						return ret;
					}
					return 0;
				}
			}
		}
	}

	if (cur->type() == ItemInfo::EQUIPMENT) {
		item->equipDirty = true;
	} else if (cur->type() == ItemInfo::GOODS) {
		if (GoodsInfoManager_IsGem(cur->id()))
			item->gemDirty = true;
		else
			item->goodsDirty = true;
	}

	if (prev != NULL)
		*prev = *prev_;
	*prev_ = *cur;

	return -2;
}

int64_t Item_AddToPackage(struct Item *item, ItemInfo::Type type, int64_t id, int count, NetProto_GetRes *getRes) {
	if (!Item_IsValid(item)) // || count <= 0)
		return -1;
	//if (count <= 0)
	//	return -1;

	int64_t ret = id;
	if (type == ItemInfo::EQUIPMENT) {
		const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo((int32_t)id);
		if (equip == NULL)
			return -1;

		int32_t empty = Item_EmptyPos(item, ItemPackage::EQUIPMENT, -1);
		if (empty == -1) {
			ret = PlayerEntity_AddMail(item->component->player, ItemInfo::EQUIPMENT, id, 1, Config_Words(0), Config_Words(24));
		} else {
			ItemInfo cur;
			cur.set_type(ItemInfo::EQUIPMENT);
			cur.set_id(Item_AddEquip(item, equip));
			if (cur.id() == -1)
				return -1;
			cur.set_count(1);

			Item_AddToPackage(item, &cur, empty, NULL);

			if (item->component->player != NULL) {
				static NetProto_GetItem getItem;
				getItem.Clear();
				cur.ToPB(getItem.mutable_item());
				getItem.set_pos(empty);
				int32_t player = PlayerEntity_ID(item->component->player);
				GCAgent_SendProtoToClients(&player, 1, &getItem);
			}
			ret = cur.id();
		}
		if (getRes != NULL) {
			PB_ItemInfo *item = getRes->add_items();
			item->set_type(PB_ItemInfo::EQUIPMENT);
			item->set_id(ret);
			item->set_count(1);
		}
	} else if (type == ItemInfo::GOODS) {
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo((int32_t)id);
		if (goods == NULL)
			return -1;

		if (!Item_IsEnough(item, ItemInfo::GOODS, id, count)) {
			PlayerEntity_AddMail(item->component->player, ItemInfo::GOODS, id, count, Config_Words(0), Config_Words(24));
		} else {
			ItemPackage::Begin begin;
			if (GoodsInfoManager_IsGem((int32_t)id))
				begin = ItemPackage::GEM;
			else
				begin = ItemPackage::GOODS;
			int total = count;
			while (total > 0) {
				int32_t empty = Item_EmptyPos(item, begin, id);
				if (empty == -1)
					return -1;

				static ItemInfo cur;
				cur.set_type(ItemInfo::GOODS);
				cur.set_id(id);
				cur.set_count(total);

				int last = Item_AddToPackage(item, &cur, empty, NULL);
				if (last > 0)
					cur.set_count(total - last);

				if (item->component->player != NULL) {
					static NetProto_GetItem getItem;
					getItem.Clear();
					cur.ToPB(getItem.mutable_item());
					getItem.set_pos(empty);
					int32_t player = PlayerEntity_ID(item->component->player);
					GCAgent_SendProtoToClients(&player, 1, &getItem);
				}

				total = last;
			}
			if(begin == ItemPackage::GEM) {
				if (Config_ScribeHost() != NULL) {
					static char buf[CONFIG_FIXEDARRAY];
					memset(buf, 0, sizeof(buf) / sizeof(char));
					char *index = buf;

					SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"gemLevel\":\"%d", goods->arg(3));
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"count\":\"%d", count);
					index += strlen(index);
					SNPRINTF2(buf, index, "\"}");
					int32_t player = PlayerEntity_ID(item->component->player);
					ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "GemLevel", buf));
				}
			}

		}
		if (getRes != NULL) {
			PB_ItemInfo *item = NULL;
			for (int i = 0; i < getRes->items_size(); i++) {
				PB_ItemInfo *unit = getRes->mutable_items(i);
				if ((ItemInfo::Type)unit->type() == type && unit->id() == id) {
					item = unit;
					break;
				}
			}
			if (item == NULL)
				item = getRes->add_items();
			item->set_type(PB_ItemInfo::GOODS);
			item->set_id(ret);
			item->set_count(item->count() + count);
		}
	} else if (type == ItemInfo::WING) {
		const Wing *info = EquipmentInfoManager_Wing((int32_t)id);
		if (info == NULL)
			return -1;

		if (id < 0 || id >= item->package->wings_size())
			return -1;
		int v = item->package->wings(id);
		if (v == -2)
			return -1;

		bool forever = count != 0;
		if (forever) {
			item->package->set_wings(id, -2);
		} else {
			if (v > 0) {
				item->package->set_wings(id, v + Config_WingDays() * 86400);
			} else {
				item->package->set_wings(id, (int)Time_TimeStamp() + Config_WingDays() * 86400);
			}
		}
		if (v <= 0)
			Item_UseWing(item, id, true);

		v = item->package->wings(id);
		if (item->component->player != NULL) {
			static NetProto_BuyWing buyWing;
			buyWing.Clear();
			buyWing.set_id(id);
			buyWing.set_forever(forever);
			int32_t player = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&player, 1, &buyWing);
		}
		if (getRes != NULL) {
			PB_ItemInfo *item = NULL;
			for (int i = 0; i < getRes->items_size(); i++) {
				PB_ItemInfo *unit = getRes->mutable_items(i);
				if ((ItemInfo::Type)unit->type() == type && unit->id() == id) {
					item = unit;
					break;
				}
			}
			if (item == NULL) {
				item = getRes->add_items();
				item->set_type(PB_ItemInfo::WING);
				item->set_id(ret);
				item->set_count(1);
			}
		}
	} else if (type == ItemInfo::FASHION) {
		const FashionInfo *info = FashionInfoManager_FashionInfo((int32_t)id);
		if (info == NULL)
			return -1;
		if (info->professionType() != -1 && (ProfessionInfo::Type)info->professionType() != item->component->baseAtt->professionType())
			return -1;

		if (id < 0 || id >= item->package->fashions_size())
			return -1;
		FashionAsset *asset = item->package->mutable_fashions(id);
		int v = asset->v();
		if (v == -2)
			return -1;

		bool forever = count != 0;
		if (forever) {
			asset->set_v(-2);
		} else {
			if (v > 0) {
				asset->set_v(v + Config_WingDays() * 86400);
			} else {
				asset->set_v((int)Time_TimeStamp() + Config_WingDays() * 86400);
			}
		}
		if (v <= 0)
			Item_UseFashion(item, id, true);

		v = asset->v();
		Equipment_WearFashion(item->component->equipment, id);
		if (item->component->player != NULL) {
			static NetProto_BuyFashion buyFashion;
			buyFashion.Clear();
			buyFashion.set_id(id);
			buyFashion.set_forever(forever);
			int32_t player = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&player, 1, &buyFashion);

			static NetProto_ShiftItem shiftItem;
			shiftItem.set_prevType(NetProto_ShiftItem::PACKAGE);
			shiftItem.set_prevPos(id);
			shiftItem.set_newType(NetProto_ShiftItem::FASHION);
			GCAgent_SendProtoToClients(&player, 1, &shiftItem);
		}
		if (getRes != NULL) {
			PB_ItemInfo *item = NULL;
			for (int i = 0; i < getRes->items_size(); i++) {
				PB_ItemInfo *unit = getRes->mutable_items(i);
				if ((ItemInfo::Type)unit->type() == type && unit->id() == id) {
					item = unit;
					break;
				}
			}
			if (item == NULL) {
				item = getRes->add_items();
				item->set_type(PB_ItemInfo::FASHION);
				item->set_id(ret);
				item->set_count(1);
			}
		}
	} else {
		return -1;
	}

	return ret;
}

static int Item_GenRides(struct Item *item, ItemInfo *info, const GoodsInfo *goods, int32_t pos)
{
	const RidesInfo *ridesInfo = RidesInfoManager_RidesInfo(goods->arg(0), 0);
	if(ridesInfo == NULL)
		return -1;
	if(info->count() < ridesInfo->fragmentCount())
		return -2;

	int remain = info->count();
	int num = 0;
	RidesAsset *asset = NULL;
	static int32_t arr[ItemPackage::RIDES_SIZE];
	for(int i = 0, count = item->package->rides_size(); i < count; i++)
	{
		asset = item->package->mutable_rides(i);
		if(asset->model() < 0)
		{
			remain -= ridesInfo->fragmentCount();
			//asset->set_model(rideInfo->id());
			arr[num] = i;
			asset->set_level(1);
			/*
			asset->set_atk(rideInfo->atk());
			asset->set_def(rideInfo->def());
			asset->set_maxHP(rideInfo->maxHP());
			asset->set_crit(rideInfo->crit());
			asset->set_accuracy(rideInfo->accuracy());
			asset->set_dodge(rideInfo->dodge());
			*/
			asset->set_exp(0);
			asset->set_potential(0);
			for(int j = 0, count = asset->lockAtt_size(); j < count; j++)
			{
				asset->set_att(j, ridesInfo->att(j));
				asset->set_lockAtt(j, false);
			}
			num++;

			if(remain < ridesInfo->fragmentCount())
				break;
		}
	}

	Item_DelFromPackage(item, pos, num * ridesInfo->fragmentCount(), true, true);

	static NetProto_GenRides genRides;
	genRides.Clear();

	for(int i = 0; i < num; i++)
	{
		asset = item->package->mutable_rides(arr[i]);
		asset->set_model(ridesInfo->id());
		genRides.add_indexes(arr[i]);
		genRides.add_ids(ridesInfo->id());
	}

	if(num > 0)
	{
		int32_t player = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&player, 1, &genRides);
	}

	return num;
}

int Item_UseGoods(struct Item *item, int32_t pos, bool all, int32_t *results, size_t size) {
	if (!Item_IsValid(item))
		return -1;
	if (item->cantUseGoods)
		return -1;
	if (!Item_IsValidPos(item, pos))
		return -1;

	ItemInfo *info = item->package->mutable_items(pos);
	if (info->type() != ItemInfo::GOODS)
		return -1;

	static NetProto_GetRes gr;
	gr.Clear();

	int count = all ? info->count() : 1;

	const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(info->id());
	if (item->component->roleAtt->fightAtt().level() < goods->requiredLevel())
		return -1;
	if (!Item_HasRMB(item, goods->useRMB() * count, 1))
		return -1;
	Item_ModifyRMB(item, -goods->useRMB() * count, true, 28, goods->id(), 0); 

	switch(goods->type()) {
		case GoodsInfo::RIDES_FRAGMENT:
			{
				int num = Item_GenRides(item, info, goods, pos);

				//DEBUG_LOGERROR("use rides fragment->%d", num);
				if(num <= 0)
					return -1;

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::RIDES);
				item->set_count(num);
				item->set_id(goods->arg(0));
			}
			break;
		case GoodsInfo::EXP:
			{
				Fight_ModifyExp(item->component->fight, goods->arg(0) * count);
				Item_DelFromPackage(item, pos, count, true, true);

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::EXP);
				item->set_count(goods->arg(0) * count);
			}
			break;
		case GoodsInfo::MONEY:
			{
				Item_ModifyMoney(item, goods->arg(0) * count);
				Item_DelFromPackage(item, pos, count, true, true);

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::MONEY);
				item->set_count(goods->arg(0) * count);
			}
			break;
		case GoodsInfo::BOX:
		case GoodsInfo::LOCK_BOX:
			{
				int key = -1;
				if (goods->type() == GoodsInfo::LOCK_BOX) {
					key = goods->arg(4);
					if (GoodsInfoManager_GoodsInfo(key)->type() != GoodsInfo::KEY)
						return -1;
					if (!Item_HasItem(item, ItemInfo::GOODS, key, 1))
						return -1;
				}

				int total = 0;
				for (int i = 0; i < count; i++) {
					if (key != -1) {
						if (!Item_HasItem(item, ItemInfo::GOODS, key, 1))
							break;
						Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)key, 1);
					}

					int ret = Item_Lottery(item, goods->id(), results + total, (int)size - total, false, &gr);
					Item_DelFromPackage(item, pos, 1, true, true);
					if (ret > 0)
						total += ret;
				}

				Item_NoticeBox(item, goods->id(), results, total, 2);
				int32_t player = PlayerEntity_ID(item->component->player);
				if (gr.items_size() > 0)
					GCAgent_SendProtoToClients(&player, 1, &gr);
				return total;
			}
			break;
		case GoodsInfo::KEY:
			{
				int box = goods->arg(0);
				if (GoodsInfoManager_GoodsInfo(box)->type() != GoodsInfo::LOCK_BOX)
					return -1;
				const ItemInfo *unit = Item_Find(item, ItemInfo::GOODS, box);
				if (unit == NULL)
					return -1;
				int pos = unit - &item->package->items(0);
				return Item_UseGoods(item, pos, all, results, size);
			}
			break;
		case GoodsInfo::RMB:
		case GoodsInfo::SUBRMB:
			{
				Item_ModifyRMB(item, goods->arg(0) * count, false, -3, 0, 0);
				Item_DelFromPackage(item, pos, count, true, true);

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::SUBRMB);
				item->set_count(goods->arg(0) * count);
			}
			break;
		case GoodsInfo::VIP:
			{
				if (item->package->vip() >= goods->arg(0))
					return -1;

				int expire = goods->arg(1);
				if (expire == -1)
					expire = 0;
				else
					expire = (int)Time_TimeStamp() + expire / 1000;
				Item_ModifyVIP(item, goods->arg(0) - item->package->vip(), expire);
				Item_DelFromPackage(item, pos, 1, true, true);

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::VIP);
				item->set_id(goods->id());
				item->set_count(goods->arg(1));
			}
			break;
		case GoodsInfo::UNLOCK_PACKAGE:
			{
				int total = goods->arg(1) * count;
				int type = goods->arg(0);

				ItemPackage::Begin begin;
				int last = 0;
				if (type == 0) { // Equip
					begin = ItemPackage::EQUIPMENT;
					last = ItemPackage::LENGTH - item->package->validNumEquipment();
				} else if (type == 1) { // Goods
					begin = ItemPackage::GOODS;
					last = ItemPackage::LENGTH - item->package->validNumGoods();
				} else { // Gem
					begin = ItemPackage::GEM;
					last = ItemPackage::LENGTH - item->package->validNumGem();
				}

				if (total > last) {
					total = last;
					count = total / goods->arg(1);
					if (total % goods->arg(1) != 0)
						count++;
				}
				if (Item_UnlockPackage(item, begin, total, true) == -1)
					return -1;

				static NetProto_UnlockPackage unlockPackage;
				unlockPackage.set_begin((PB_ItemPackage::Begin)begin);
				unlockPackage.set_count(total);
				int32_t player = PlayerEntity_ID(item->component->player);
				GCAgent_SendProtoToClients(&player, 1, &unlockPackage);

				Item_DelFromPackage(item, pos, count, true, true);
			}
			break;
		case GoodsInfo::SOUL:
			{
				Item_ModifySoul(item, goods->arg(0) * count);
				Item_DelFromPackage(item, pos, count, true, true);

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::SOUL);
				item->set_count(goods->arg(0) * count);
			}
			break;
		case GoodsInfo::SOULJADE:
			{
				Item_ModifySoulJade(item, ExploreInfo::SMALL, goods->arg(0) * count);
				Item_DelFromPackage(item, pos, count, true, true);

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::SOULJADE);
				item->set_count(goods->arg(0) * count);
			}
			break;
		case GoodsInfo::SOULSTONE:
			{
				Item_ModifySoulStone(item, goods->arg(0) * count);
				Item_DelFromPackage(item, pos, count, true, true);

				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::SOULSTONE);
				item->set_count(goods->arg(0) * count);
			}
			break;
		default:
			return -1;
	}

	int32_t player = PlayerEntity_ID(item->component->player);
	if (gr.items_size() > 0)
		GCAgent_SendProtoToClients(&player, 1, &gr);

	if (Config_ScribeHost() != NULL) {
		char arg1[16];
		SNPRINTF1(arg1, "{\"id\":\"%d\"}", (int)goods->id());
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "UseItem", arg1));
	}
	return 0;
}

int Item_UnlockPackage(struct Item *item, ItemPackage::Begin begin, int count, bool free) {
	if (!Item_IsValid(item) || count <= 0)
		return -1;

	int RMBCost = 0;
	int MONEYCost = 0;

	if (count > ItemPackage::LENGTH) 
		return -1;

	if(!free) {
		int start, end;
		if (begin == ItemPackage::EQUIPMENT) {
			start = (int)begin + item->package->validNumEquipment();
		} else if (begin == ItemPackage::GOODS) {
			start = (int)begin + item->package->validNumGoods();
		} else if (begin == ItemPackage::GEM) {
			start = (int)begin + item->package->validNumGem();
		} else {
			return -1;
		}
		end = start + count;

		if (start < 0 || end < 0 || end > begin + ItemPackage::LENGTH)
			return -1;

		for(int ix = start; ix < end; ++ix)
		{
			if(package.numToCost[ix].first == ::BusinessInfo::RMB)
				RMBCost += package.numToCost[ix].second;
			else if(package.numToCost[ix].first == ::BusinessInfo::MONEY)
				MONEYCost += package.numToCost[ix].second;
		}
		if (!(Item_HasRMB(item, RMBCost) && item->package->money() >= MONEYCost))
			return -1;
	}

	if (begin == ItemPackage::EQUIPMENT) {
		if (item->package->validNumEquipment() + count > (int)ItemPackage::LENGTH)
			return -1;
		item->package->set_validNumEquipment(item->package->validNumEquipment() + count);
	} else if (begin == ItemPackage::GOODS) {
		if (item->package->validNumGoods() + count > (int)ItemPackage::LENGTH)
			return -1;
		item->package->set_validNumGoods(item->package->validNumGoods() + count);
	} else if (begin == ItemPackage::GEM) {
		if (item->package->validNumGem() + count > (int)ItemPackage::LENGTH)
			return -1;
		item->package->set_validNumGem(item->package->validNumGem() + count);
	} else {
		return -1;
	}

	if(MONEYCost > 0)
		Item_ModifyMoney(item, -MONEYCost);
	if(RMBCost > 0)
		Item_ModifyRMB(item, -RMBCost, false, 7, count, 0);

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"unlockCount\":\"%d", count);
		index += strlen(index);
		if (begin == ItemPackage::EQUIPMENT) {
			SNPRINTF2(buf, index, "\",\"package\":\"EQUIPMENT");
			index += strlen(index);
		} else if (begin == ItemPackage::GOODS) {
			SNPRINTF2(buf, index, "\",\"package\":\"GOODS");
			index += strlen(index);
		} else if (begin == ItemPackage::GEM) {
			SNPRINTF2(buf, index, "\",\"package\":\"GEM");
			index += strlen(index);
		}

		SNPRINTF2(buf, index, "\"}");
		int32_t player = PlayerEntity_ID(item->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "UnlockCount", buf));
	}

	return 0;
}

bool Item_HasItem(struct Item *item, ItemInfo::Type type, int64_t id, int min) {
	if (!Item_IsValid(item))
		return false;

	int i = 0;
	int end = 0;
	if (type == ItemInfo::EQUIPMENT) {
		i = (int)ItemPackage::EQUIPMENT;
		end = item->package->validNumEquipment() + i;
	} else if (type == ItemInfo::GOODS) {
		if (GoodsInfoManager_IsGem(id)) {
			i = (int)ItemPackage::GEM;
			end = item->package->validNumGem() + i;
		} else {
			i = (int)ItemPackage::GOODS;
			end = item->package->validNumGoods() + i;
		}
	} else {
		return false;
	}

	int total = 0;
	for (; i < end; i++) {
		if (item->package->items(i).type() == type && item->package->items(i).id() == id) {
			total += item->package->items(i).count();
			if (total >= min)
				return true;
		}
	}

	return false;
}

int32_t Item_EmptyPos(struct Item *item, ItemPackage::Begin begin, int64_t id) {
	if (!Item_IsValid(item))
		return -1;

	int i = 0;
	int end = 0;
	if (begin == ItemPackage::EQUIPMENT) {
		i = (int)ItemPackage::EQUIPMENT;
		end = item->package->validNumEquipment() + i;
	} else if (begin == ItemPackage::GOODS) {
		i = (int)ItemPackage::GOODS;
		end = item->package->validNumGoods() + i;
	} else if (begin == ItemPackage::GEM) {
		i = (int)ItemPackage::GEM;
		end = item->package->validNumGem() + i;
	} else {
		return -1;
	}

	if ((begin == ItemPackage::GOODS || begin == ItemPackage::GEM) && id != -1) {
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo((int32_t)id);
		if (goods == NULL)
			return -1;

		for (int n = i; n < end; n++) {
			const ItemInfo *cur = &item->package->items(n);
			if (cur->type() == ItemInfo::GOODS && cur->id() == id && cur->count() < goods->repeat())
				return n;
		}
	}

	for (int n = i; n < end; n++) {
		if (item->package->items(n).type() == ItemInfo::NONE)
			return n;
	}

	return -1;
}

bool Item_IsEnough(struct Item *item, ItemInfo::Type type, int64_t id, int count) {
	if (!Item_IsValid(item))
		return false;

	int i = 0;
	int end = 0;
	int repeat = 1;
	if (type == ItemInfo::EQUIPMENT) {
		i = (int)ItemPackage::EQUIPMENT;
		end = item->package->validNumEquipment() + i;
	} else if (type == ItemInfo::GOODS) {
		if (GoodsInfoManager_IsGem(id)) {
			i = (int)ItemPackage::GEM;
			end = item->package->validNumGem() + i;
		} else {
			i = (int)ItemPackage::GOODS;
			end = item->package->validNumGoods() + i;
		}
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo((int32_t)id);
		assert(goods != NULL);
		repeat = goods->repeat();
	} else {
		return false;
	}

	int total = 0;
	for (; i < end; i++) {
		const ItemInfo *cur = &item->package->items(i);
		if (cur->type() == ItemInfo::NONE) {
			total += repeat;
		} else if (cur->type() == type && cur->id() == id) {
			total += (repeat - cur->count());
		}
		if (total >= count)
			return true;
	}

	return false;
}

const ItemInfo * Item_Find(struct Item *item, ItemInfo::Type type, int64_t id) {
	if (!Item_IsValid(item))
		return NULL;

	int i = 0;
	int end = 0;
	if (type == ItemInfo::EQUIPMENT) {
		i = (int)ItemPackage::EQUIPMENT;
		end = item->package->validNumEquipment() + i;
	} else if (type == ItemInfo::GOODS) {
		if (GoodsInfoManager_IsGem(id)) {
			i = (int)ItemPackage::GEM;
			end = item->package->validNumGem() + i;
		} else {
			i = (int)ItemPackage::GOODS;
			end = item->package->validNumGoods() + i;
		}
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo((int32_t)id);
		assert(goods != NULL);
	} else {
		return NULL;
	}

	for (; i < end; i++) {
		const ItemInfo *cur = &item->package->items(i);
		if (cur->type() == type && cur->id() == id)
			return cur;
	}
	return NULL;
}

int Item_Count(struct Item *item, ItemInfo::Type type, int64_t id) {
	if (!Item_IsValid(item))
		return 0;

	int i = 0;
	int end = 0;
	if (type == ItemInfo::EQUIPMENT) {
		i = (int)ItemPackage::EQUIPMENT;
		end = item->package->validNumEquipment() + i;
	} else if (type == ItemInfo::GOODS) {
		if (GoodsInfoManager_IsGem(id)) {
			i = (int)ItemPackage::GEM;
			end = item->package->validNumGem() + i;
		} else {
			i = (int)ItemPackage::GOODS;
			end = item->package->validNumGoods() + i;
		}
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo((int32_t)id);
		assert(goods != NULL);
	} else {
		return 0;
	}

	int count = 0;
	for (; i < end; i++) {
		const ItemInfo *cur = &item->package->items(i);
		if (cur->type() == type && cur->id() == id)
			count += cur->count();
	}
	return count;
}

void Item_SetCantUseGoods(struct Item *item, bool value) {
	if (!Item_IsValid(item))
		return;

	item->cantUseGoods = value;
}

const ItemInfo * Item_ALTItem(struct Item *item, int32_t id) {
	if (!Item_IsValid(item))
		return NULL;
	if (id < 0 || id >= item->alt->alt_size())
		return NULL;

	return &item->alt->alt(id);
}

bool Item_HasALT(struct Item *item, ItemInfo::Type type, int64_t id) {
	if (!Item_IsValid(item))
		return false;

	for (int i = 0; i < item->alt->alt_size(); i++) {
		const ItemInfo *cur = &item->alt->alt(i);
		if (cur->type() != ItemInfo::NONE && cur->type() == type && cur->id() == id)
			return true;
	}

	return false;
}

void Item_DelAllFromALT(struct Item *item, ItemInfo::Type type, int64_t id) {
	if (!Item_IsValid(item))
		return;

	for (int i = 0; i < item->alt->alt_size(); i++) {
		ItemInfo *cur = item->alt->mutable_alt(i);
		if (cur->type() != ItemInfo::NONE && cur->type() == type && cur->id() == id)
			cur->set_type(ItemInfo::NONE);
	}
}

void Item_DelFromALT(struct Item *item, int32_t id) {
	if (!Item_IsValid(item))
		return;
	if (id < 0 || id >= item->alt->alt_size())
		return;

	item->alt->mutable_alt(id)->set_type(ItemInfo::NONE);
}

void Item_AddToALT(struct Item *item, const ItemInfo *cur, int32_t id) {
	if (!Item_IsValid(item) || cur == NULL || cur->type() == ItemInfo::EQUIPMENT)
		return;
	if (id < 0 || id >= item->alt->alt_size())
		return;

	int i;
	for (i = 0; i < item->alt->alt_size(); i++) {
		ItemInfo *alt = item->alt->mutable_alt(i);
		if (alt->type() == ItemInfo::SKILL && alt->id() == cur->id()) {
			alt->set_type(ItemInfo::NONE);
			alt->set_id(-1);
			break;
		}
	}

	if (i != id)
		*item->alt->mutable_alt(id) = *cur;
}

int32_t Item_EmptyALT(struct Item *item) {
	if (!Item_IsValid(item))
		return -1;

	for (int i = 0; i < item->alt->alt_size(); i++) {
		if (item->alt->alt(i).type() == ItemInfo::NONE)
			return i;
	}
	return -1;
}

int32_t Item_AddEquip(struct Item *item, const EquipmentInfo *info) {
	if (!Item_IsValid(item) || info == NULL)
		return -1;
	for (int i = 0; i < item->package->equips_size(); i++) {
		EquipAsset *asset = item->package->mutable_equips(i);
		if (asset->mode() < 0) {
			asset->set_mode(info->id());
			asset->set_strongLevel(0);
			for (int j = 0; j < asset->gemModel_size(); j++) {
				asset->set_gemModel(j, -2);
				asset->set_gemType(j, Config_RandomGemType());
			}
			for (int j = 0; j < info->enhanceType_size(); j++) {
				asset->set_enhanceDelta(j, 0);
			}
			Config_AdjustGemHole(asset);

			static NetProto_GetEquip proto;
			proto.set_id(i);
			asset->ToPB(proto.mutable_asset());
			int32_t player = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&player, 1, &proto);
			return i;
		}
	}
	return -1;
}

int32_t Item_AddEquip(PB_ItemPackage *package, const EquipmentInfo *info) {
	if (package == NULL || info == NULL)
		return -1;
	for (int i = 0; i < package->equips_size(); i++) {
		PB_EquipAsset *asset = package->mutable_equips(i);
		if (asset->mode() < 0) {
			asset->set_mode(info->id());
			asset->set_strongLevel(0);
			for (int j = 0; j < asset->gemModel_size(); j++) {
				asset->set_gemModel(j, -2);
				asset->set_gemType(j, Config_RandomGemType());
			}
			for (int j = 0; j < info->enhanceType_size(); j++) {
				asset->set_enhanceDelta(j, 0);
			}
			Config_AdjustGemHole(asset);
			return i;
		}
	}
	return -1;
}

void Item_DelEquip(struct Item *item, int32_t id) {
	if (!Item_IsValid(item))
		return;
	EquipAsset *asset = Item_EquipAsset(item, id);
	if (asset == NULL)
		return;
	asset->set_mode(-1);
}

void Item_DelEquip(PB_ItemPackage *package, int32_t id) {
	if (package == NULL)
		return;
	if (id < 0 || id >= package->equips_size())
		return;
	package->mutable_equips(id)->set_mode(-1);
}

EquipAsset * Item_EquipAsset(struct Item *item, int32_t id) {
	if (!Item_IsValid(item))
		return NULL;
	if (id < 0 || id >= item->package->equips_size())
		return NULL;
	EquipAsset *asset = item->package->mutable_equips(id);
	if (asset->mode() < 0)
		return NULL;
	return asset;
}

const EquipmentInfo * Item_EquipInfo(struct Item *item, int32_t id) {
	if (!Item_IsValid(item))
		return NULL;
	EquipAsset *asset = Item_EquipAsset(item, id);
	if (asset == NULL)
		return NULL;
	return EquipmentInfoManager_EquipmentInfo(asset->mode());
}

int32_t Item_Explore(struct Item *item, ExploreInfo::Type type, bool lucky) {
	if (!Item_IsValid(item))
		return -1;

	static NetProto_GetRes gr;
	gr.Clear();
	if (type == ExploreInfo::NORMAL) {
		if (!lucky) {
			if (item->component->playerAtt->dayEvent(0) >= Config_MaxNormalExplore())
				return -1;
			/*
			   if (item->component->playerAtt->dayEvent(0) >= Config_MaxFreeNormalExplore()) {
			   if (item->package->money() < Config_NormalExploreMoney() * Fight_Att(item->component->fight)->level())
			   return -1;
			   }
			   */
		}

		int32_t explore = BloodInfoManager_RandomExploreInfo(type);
		if (explore == -1)
			return -1;

		const ExploreInfo *info = BloodInfoManager_ExploreInfo(explore);
		if (info->soulJadeCount() > 0) {
			Item_ModifySoulJade(item, info->soulJadeType(), info->soulJadeCount());
			PB_ItemInfo *cur = gr.add_items();
			cur->set_type(PB_ItemInfo::SOULJADE);
			cur->set_count(info->soulJadeCount());
		}
		if (info->soul() > 0) {
			Item_ModifySoul(item, info->soul());
			PB_ItemInfo *cur = gr.add_items();
			cur->set_type(PB_ItemInfo::SOUL);
			cur->set_count(info->soul());
		}
		if (info->soulStone() > 0) {
			Item_ModifySoulStone(item, info->soulStone());
			PB_ItemInfo *cur = gr.add_items();
			cur->set_type(PB_ItemInfo::SOULSTONE);
			cur->set_count(info->soulStone());
		}
		int32_t player = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&player, 1, &gr);

		if (!lucky) {
			/*
			   if (item->component->playerAtt->dayEvent(0) >= Config_MaxFreeNormalExplore())
			   Item_ModifyMoney(item, -Config_NormalExploreMoney() * Fight_Att(item->component->fight)->level());
			   */
			PlayerEntity_SetDayEvent(item->component->player, 0, item->component->playerAtt->dayEvent(0) + 1);
		}

		return explore;
	} else if (type == ExploreInfo::HIGH) {
		int price = 0;
		if (!lucky) {
			/*
			   price = max(item->component->playerAtt->dayEvent(1) + 1, Config_MaxHighExplore()) - Config_MaxHighExplore() + Config_HighExploreGem();
			   if (price > Config_MaxHighExploreGem())
			   */
			price = Config_MaxHighExploreGem();
			//if (item->package->rmb() < price)
			if (!Item_HasRMB(item, price))
				return -1;
		}

		int32_t explore = BloodInfoManager_RandomExploreInfo(type);
		if (explore == -1)
			return -1;

		const ExploreInfo *info = BloodInfoManager_ExploreInfo(explore);
		if (info->soulJadeCount() > 0) {
			Item_ModifySoulJade(item, info->soulJadeType(), info->soulJadeCount());
			PB_ItemInfo *cur = gr.add_items();
			cur->set_type(PB_ItemInfo::SOULJADE);
			cur->set_count(info->soulJadeCount());
		}
		if (info->soul() > 0) {
			Item_ModifySoul(item, info->soul());
			PB_ItemInfo *cur = gr.add_items();
			cur->set_type(PB_ItemInfo::SOUL);
			cur->set_count(info->soul());
		}
		if (info->soulStone() > 0) {
			Item_ModifySoulStone(item, info->soulStone());
			PB_ItemInfo *cur = gr.add_items();
			cur->set_type(PB_ItemInfo::SOULSTONE);
			cur->set_count(info->soulStone());
		}
		int32_t player = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&player, 1, &gr);

		if (!lucky) {
			Item_ModifyRMB(item, -price, false, 8, 0, 0);
			PlayerEntity_SetDayEvent(item->component->player, 1, item->component->playerAtt->dayEvent(1) + 1);
		}

		return explore;
	}

	return -1;
}

static void GetBoxRes(struct Item *item, BoxInfo::Type type, int id, int count, NetProto_GetRes *gr, int boxID) {
	switch(type) {
		case BoxInfo::EXP:
			{
				Fight_ModifyExp(item->component->fight, id);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::EXP);
				item->set_count(id);
			}
			break;
		case BoxInfo::MONEY:
			{
				Item_ModifyMoney(item, id);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::MONEY);
				item->set_count(id);
			}
			break;
		case BoxInfo::RMB:
		case BoxInfo::SUBRMB:
			{
				Item_ModifyRMB(item, id, false, -4, 0, 0);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::SUBRMB);
				item->set_count(id);
			}
			break;
		case BoxInfo::SOUL:
			{
				Item_ModifySoul(item, id);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::SOUL);
				item->set_count(id);
			}
			break;
		case BoxInfo::SOULJADE:
			{
				Item_ModifySoulJade(item, ExploreInfo::SMALL, id);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::SOULJADE);
				item->set_count(id);
			}
			break;
		case BoxInfo::SOULSTONE:
			{
				Item_ModifySoulStone(item, id);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::SOULSTONE);
				item->set_count(id);
			}
			break;
		case BoxInfo::DESIGNATION:
			{
				PlayerEntity_AddDesignation(item->component->player, id);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::DESIGNATION);
				item->set_id(id);
				item->set_count(1);
			}
			break;
		case BoxInfo::GOODS:
			{
				if (GoodsInfoManager_GoodsInfo(id) == NULL || count <= 0)
					DEBUG_LOGERROR("box goods is null or count<=0, id: %d, count: %d", id, count);
				assert(GoodsInfoManager_GoodsInfo(id) != NULL && count > 0);
				Item_AddToPackage(item, ItemInfo::GOODS, id, count);

				int64_t roleID = PlayerEntity_RoleID(item->component->player);
				char ch[1024];
				SNPRINTF1(ch, "7-%d-%d", id, count);
				DCProto_SaveSingleRecord saveRecord;
				saveRecord.set_mapID(-23);
				saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
				saveRecord.mutable_record()->mutable_role()->set_name(ch);
				saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
				GCAgent_SendProtoToDCAgent(&saveRecord);

				PB_ItemInfo *unit = gr->add_items();
				unit->set_type(PB_ItemInfo::GOODS);
				unit->set_id(id);
				unit->set_count(count);

				{
					static DCProto_SaveSingleRecord saveRecord;
					static char ch[1024];
					SNPRINTF1(ch, "2-%d-%d-%d", id, count, boxID);
					saveRecord.set_mapID(-20);
					saveRecord.mutable_record()->mutable_role()->set_roleID(item->component->baseAtt->roleID());
					saveRecord.mutable_record()->mutable_role()->set_name(ch);
					saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
					GCAgent_SendProtoToDCAgent(&saveRecord);
				}
			}
			break;
		case BoxInfo::EQUIPMENT:
			{
				assert(EquipmentInfoManager_EquipmentInfo(id) != NULL);
				int64_t equipID = Item_AddToPackage(item, ItemInfo::EQUIPMENT, id, 1);

				int64_t roleID = PlayerEntity_RoleID(item->component->player);
				char ch[1024];
				SNPRINTF1(ch, "8-%d-%d", id, 1);
				DCProto_SaveSingleRecord saveRecord;
				saveRecord.set_mapID(-23);
				saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
				saveRecord.mutable_record()->mutable_role()->set_name(ch);
				saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
				GCAgent_SendProtoToDCAgent(&saveRecord);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::EQUIPMENT);
				item->set_id(equipID);
				item->set_count(1);
			}
			break;
		case BoxInfo::FASHION:
			{
				assert(FashionInfoManager_FashionInfo(id) != NULL);
				Item_AddToPackage(item, ItemInfo::FASHION, id, count);

				int64_t roleID = PlayerEntity_RoleID(item->component->player);
				char ch[1024];
				SNPRINTF1(ch, "9-%d-%d", id, count);
				DCProto_SaveSingleRecord saveRecord;
				saveRecord.set_mapID(-23);
				saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
				saveRecord.mutable_record()->mutable_role()->set_name(ch);
				saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
				GCAgent_SendProtoToDCAgent(&saveRecord);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::FASHION);
				item->set_id(id);
				item->set_count(1);
			}
			break;
		case BoxInfo::WING:
			{
				assert(EquipmentInfoManager_Wing(id) != NULL);
				Item_AddToPackage(item, ItemInfo::WING, id, count);

				int64_t roleID = PlayerEntity_RoleID(item->component->player);
				char ch[1024];
				SNPRINTF1(ch, "10-%d-%d", id, count);
				DCProto_SaveSingleRecord saveRecord;
				saveRecord.set_mapID(-23);
				saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
				saveRecord.mutable_record()->mutable_role()->set_name(ch);
				saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
				GCAgent_SendProtoToDCAgent(&saveRecord);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::WING);
				item->set_id(id);
				item->set_count(1);
			}
			break;
		case BoxInfo::GODSCORE:
			{
				Item_ModifyGodScore(item, id);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::GODSCORE);
				item->set_count(id);
			}
			break;
		case BoxInfo::TRANSFORM:
			{
				Item_TransformLevelUp(item, id, true);

				PB_ItemInfo *item = gr->add_items();
				item->set_type(PB_ItemInfo::TRANSFORM);
				item->set_id(id);
				item->set_count(1);
			}
			break;
		case BoxInfo::GODSHIP:
			{
				DEBUG_LOG("boxid:");
				Item_AddToGodPackage(item, id);
				DEBUG_LOG("boxid: ");
		//		PB_ItemInfo *item = gr->add_items();
		//		item->set_type(PB_ItemInfo::GODSHIP);
		//		item->set_id(id);
		//		item->set_count(1);
			}
			break;
		default:
			DEBUG_LOGERROR("Failed to get box res, type: %d", (int)type);
			break;
	}
}

int Item_Lottery(struct Item *item, int32_t box, int32_t *results, size_t size, bool notice, NetProto_GetRes *getRes) {
	if (!Item_IsValid(item))
		return -1;

	DEBUG_LOG("boxafasf: %d", box);
	const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(box);
	DEBUG_LOG("boxid: %d", goods->arg(0));
	if (goods == NULL)
		return -1;
	if (goods->type() != GoodsInfo::BOX && goods->type() != GoodsInfo::LOCK_BOX)
		return -1;
	DEBUG_LOG("boxid: %d", goods->arg(0));

	const BoxInfo *boxInfo = BoxInfoManager_BoxInfo(goods->arg(0));
	if (boxInfo == NULL)
		return -1;
	DEBUG_LOG("boxid: %d", goods->arg(0));

	if (item->package->honor() < goods->arg(1))
		return -1;
	Item_ModifyHonor(item, -goods->arg(1));
	DEBUG_LOG("safklasdjfalfsafsakfda");

	int table[CONFIG_FIXEDARRAY];
	if (boxInfo->prof()) {
		static BoxInfo tempBox;
		tempBox.set_id(boxInfo->id());
		tempBox.set_num(boxInfo->num());
		tempBox.set_prof(boxInfo->prof());
		ProfessionInfo::Type prof = item->component->baseAtt->professionType();
		for (int i = 0, j = 0; i < boxInfo->type_size(); i++) {
			if (boxInfo->type(i) == BoxInfo::EQUIPMENT) {
				const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(boxInfo->arg2(i));
				if (equip == NULL)
					DEBUG_LOGERROR("Wrong box equip, boxid: %d", boxInfo->id());
				assert(equip != NULL);
				if (equip->professionType() != -1 && (ProfessionInfo::Type)equip->professionType() != prof)
					continue;
			} else if (boxInfo->type(i) == BoxInfo::FASHION) {
				const FashionInfo *fashion = FashionInfoManager_FashionInfo(boxInfo->arg2(i));
				assert(fashion != NULL);
				if (fashion->professionType() != -1 && (ProfessionInfo::Type)fashion->professionType() != prof)
					continue;
			}
			if (j >= tempBox.type_size()) {
				tempBox.add_type(boxInfo->type(i));
				tempBox.add_arg1(boxInfo->arg1(i));
				tempBox.add_arg2(boxInfo->arg2(i));
				tempBox.add_arg3(boxInfo->arg3(i));
			} else {
				tempBox.set_type(j, boxInfo->type(i));
				tempBox.set_arg1(j, boxInfo->arg1(i));
				tempBox.set_arg2(j, boxInfo->arg2(i));
				tempBox.set_arg3(j, boxInfo->arg3(i));
			}
			table[j++] = i;
			if (boxInfo->type(i) == BoxInfo::NONE)
				break;
		}
		boxInfo = &tempBox;
	}

	int total = boxInfo->num();
	bool all = false;
	if (total == 0) {
		for (total = 0; total < boxInfo->type_size(); total++) {
			if (boxInfo->type(total) == BoxInfo::NONE)
				break;
		}
		all = true;
	} else {
		int i;
		for (i = 0; i < boxInfo->type_size(); i++) {
			if (boxInfo->type(i) == BoxInfo::NONE)
				break;
		}
		if (total > i)
			total = 0;
	}

	static NetProto_GetRes gr;
	gr.Clear();

	int num;
	for (num = 0; num < total; num++) {
		if ((int)size <= num)
			return num;

	DEBUG_LOG("safklasdjfalfsafsakfda");
		int i = 0;
		if (!all) {
			int rate[CONFIG_FIXEDARRAY];
			int count;
			for (count = 0; count < boxInfo->type_size(); count++) {
				if (boxInfo->type(count) == BoxInfo::NONE)
					break;

				bool done = false;
				for (int j = 0; j < num; j++) {
					if (results[j] == (boxInfo->prof() ? table[count] : count)) {
						done = true;
						break;
					}
				}
				rate[count] = (done ? 0 : boxInfo->arg1(count));
			}
			if (count <= 0)
				return -1;

	DEBUG_LOG("safklasdjfalfsafsakfda");
			i = Time_RandomSelect(rate, count);
		} else {
			i = num;
		}

		int arg2 = boxInfo->arg2(i);
		int arg3 = boxInfo->arg3(i);
		GetBoxRes(item, boxInfo->type(i), arg2, arg3, &gr, boxInfo->id());

		results[num] = boxInfo->prof() ? table[i] : i;
	}

	if (notice && gr.items_size() > 0) {
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &gr);
	}
	if (getRes != NULL) {
		for (int i = 0; i < gr.items_size(); i++)
			*getRes->add_items() = gr.items(i);
	}

	return num;
}

int Item_ExchangeGoods(struct Item *item, int index, bool all) {
	if (!Item_IsValid(item))
		return -1;

	const ExchangeTable *table = GoodsInfoManager_ExchangeTable(index);
	if (table == NULL)
		return -1;

	bool ok = false;
	static NetProto_GetRes gr;
	gr.Clear();
	do {
		if (!Item_HasItem(item, ItemInfo::GOODS, table->sourceGoods(), table->sourceCount()))
			break;
		Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)table->sourceGoods(), table->sourceCount());
		GetBoxRes(item, table->targetType(), table->arg1(), table->arg2(), &gr, -1);
		ok = true;
	} while (all);

	if (!ok)
		return -1;

	if (gr.items_size() > 0) {
		int32_t player = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&player, 1, &gr);
	}

	return 0;
}

void Item_NoticeBox(struct Item *item, int32_t box, int32_t *results, int size, int from) {
	if (!Item_IsValid(item) || results == NULL || size <= 0)
		return;

	const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(box);
	if (goods == NULL || goods->type() != GoodsInfo::BOX)
		return;

	const BoxInfo *boxInfo = BoxInfoManager_BoxInfo(goods->arg(0));
	if (boxInfo == NULL)
		return;

	for (int i = 0; i < size; i++) {
		if (size >= boxInfo->type_size())
			break;
		BoxInfo::Type type = boxInfo->type(results[i]);
		int32_t arg2 = boxInfo->arg2(results[i]);
		const char *name = NULL;
		const char *color = NULL;
		if (type == BoxInfo::EQUIPMENT) {
			if (PlayerEntity_EquipNeedNotice(arg2)) {
				const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(arg2);
				assert(equip != NULL);
				name = equip->name().c_str();
				color = Config_EquipColorStr(equip->colorType());
			}
		} else if (type == BoxInfo::GOODS) {
			if (PlayerEntity_GoodsNeedNotice(arg2)) {
				const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(arg2);
				assert(goods != NULL);
				name = goods->name().c_str();
				color = Config_EquipColorStr((EquipmentInfo::ColorType)goods->colorType());
			}
		}
		if (name == NULL)
			continue;
		static NetProto_Message message;
		message.Clear();
		char buf[CONFIG_FIXEDARRAY];
		if (from == 0) {
			int32_t map = Movement_Att(item->component->movement)->mapID();
			const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
			const char *mapName = mapInfo == NULL ? "" : mapInfo->name().c_str();
			SNPRINTF1(buf, Config_Words(12), item->component->baseAtt->name(), mapName, color, name);
		} else if (from == 1) {
			SNPRINTF1(buf, Config_Words(13), item->component->baseAtt->name(), color, name);
		} else if (from == 2) {
			SNPRINTF1(buf, Config_Words(14), item->component->baseAtt->name(), Config_EquipColorStr((EquipmentInfo::ColorType)goods->colorType()), goods->name().c_str(), color, name);
		} else {
			continue;
		}
		message.set_content(buf);
		message.set_count(1);
		GCAgent_SendProtoToAllClients(&message);
	}
}

int Item_GetGift(struct Item *item, AwardInfo::Type type, int index, int arg) {
	if (!Item_IsValid(item))
		return -1;

	const AwardInfo *awardInfo = NULL;
	if (type != AwardInfo::ROOM_CHAPTER && type != AwardInfo::INVITECODE) {
		awardInfo = AwardInfoManager_AwardInfo(type, index);
		if (awardInfo == NULL)
			return -1;
	}

	if (type == AwardInfo::ONLINE) {
		if (item->component->playerAtt->fixedEvent((int)type) & (1 << index))
			return -1;
		/*
		   if (item->component->playerAtt->dayEvent(3) + ((int64_t)Time_TimeStamp() - item->component->playerAtt->prevLogin()) < awardInfo->arg() / 1000)
		   return -1;
		   */
	} else if (type == AwardInfo::COME) {
		int v = item->component->playerAtt->fixedEvent(12);
		if (v & (1 << (index + 16)))
			return -1;
		if ((item->component->playerAtt->fixedEvent(12) & 0xffff) < awardInfo->arg())
			return -1;
	} else if (type == AwardInfo::TOTAL_COME) {
		int v = item->component->playerAtt->fixedEvent(44);
		if (v & (1 << index))
			return -1;
		if ((item->component->playerAtt->fixedEvent(31) & 0xffff) < awardInfo->arg())
			return -1;
	} else if (type == AwardInfo::LEVEL) {
		if (item->component->playerAtt->fixedEvent((int)type) & (1 << index))
			return -1;
		if (Fight_Att(item->component->fight)->level() < awardInfo->arg())
			return -1;
	} else if (type == AwardInfo::POWER) {
		if (item->component->playerAtt->fixedEvent((int)type) & (1 << index))
			return -1;
		if (Fight_Power(item->component->fight) < awardInfo->arg())
			return -1;
	} else if (type == AwardInfo::RMB) {
		if (item->component->playerAtt->fixedEvent(9) & (1 << index))
			return -1;
		int64_t totalRMB = 0;
		if (Config_InActiveTime(10)) {
			totalRMB = item->component->playerAtt->fixedEvent(8);
		} else {
			// totalRMB = item->package->totalRMB();
			return -1;
		}
		// DEBUG_LOGRECORD("totalrmb: %d, arg: %d, index: %d", (int)totalRMB, awardInfo->arg(), index);
		if (totalRMB < awardInfo->arg())
			return -1;
	} else if (type == AwardInfo::NEWYEAR_GIFT) {
		if (item->component->playerAtt->fixedEvent(97) & (1 << index))
			return -1;
		int64_t totalMoney = 0;
		if (Config_InActiveTime(19)) {
			totalMoney = item->component->playerAtt->fixedEvent(96);
		} else {
			// totalMoney = item->package->totalRMB();
			return -1;
		}
		// DEBUG_LOGRECORD("totalMoney: %d, arg: %d, index: %d", (int)totalMoney, awardInfo->arg(), index);
		if (totalMoney < awardInfo->arg())
			return -1;
	} else if (type == AwardInfo::VIP) {
		if (item->component->playerAtt->fixedEvent(10) & (1 << index))
			return -1;
		if (item->package->vip() < awardInfo->arg())
			return -1;
		const GoodsInfo * goodsInfo = GoodsInfoManager_GoodsInfo(awardInfo->award());
		if (goodsInfo == NULL)
			return -1;

		if (!Item_HasRMB(item, goodsInfo->rmb()))
			return -1;
	} else if (type == AwardInfo::UNIT_RECHARGE) {
		if (item->component->playerAtt->dayEvent(5) & (1 << 5))
			return -1;
		if ((item->component->playerAtt->dayEvent(5) & (1 << 4)) == 0)
			return -1;
	} else if (type == AwardInfo::TOTAL_COST) {
		if (item->component->playerAtt->fixedEvent(46) & (1 << index))
			return -1;
		if (item->package->totalCost() < awardInfo->arg())
			return -1;
	} else if (type == AwardInfo::RANDOM_RETURN_RMB) {
		if (item->component->playerAtt->fixedEvent(66) & (1 << index))
			return -1;
		if (!Item_HasRMB(item, awardInfo->arg(), 1))
			return -1;
	} else if (type == AwardInfo::FIRST_RECHARGE) {
		if ((item->component->playerAtt->fixedEvent(15) & (1 << 1)) == 0)
			return -1;
		if (item->component->playerAtt->fixedEvent(15) & (1 << 2))
			return -1;
	} else if (type == AwardInfo::MISSION_ACTIVITY) {
		if (item->component->playerAtt->dayEvent(69) < awardInfo->arg())
			return -1;
		if (item->component->playerAtt->dayEvent(70) & (1 << index))
			return -1;
	} else if (type == AwardInfo::ROOM_CHAPTER) {
		awardInfo = AwardInfoManager_AwardInfo(type, arg, index);
		if (awardInfo == NULL)
			return -1;

		int bit = arg * 3 + index;
		int num = bit / 30;
		bit = bit % 30;
		if (item->component->playerAtt->fixedEvent(89 + num) & (1 << bit))
			return -1;

		const vector<int32_t> *chapterMap = MapInfoManager_ChapterMap(arg);
		if (chapterMap == NULL)
			return -1;
		int count = 0;
		for (size_t i = 0; i < chapterMap->size(); i++) {
			int32_t map = (*chapterMap)[i];
			if (map >= Config_SingleBegin() && map <= Config_SingleEnd()) {
				int events[] = {72, 73, 74, 75, 76, 77, 78, 79};
				int index = (map - Config_SingleBegin()) / 15;
				if (index < (int)(sizeof(events) / sizeof(events[0]))) {
					index = events[index];
					int bit = (map - Config_SingleBegin()) % 15 * 2;
					int event = item->component->playerAtt->fixedEvent(index);
					if ((event & (3 << bit)) >= (3 << bit))
						count++;
				}
			} else if (map >= Config_SingleEnhanceBegin() && map <= Config_SingleEnhanceEnd()) {
				int events[] = {80, 81, 82, 83};
				int index = (map - Config_SingleEnhanceBegin()) / 15;
				if (index < (int)(sizeof(events) / sizeof(events[0]))) {
					index = events[index];
					int bit = (map - Config_SingleEnhanceBegin()) % 15 * 2;
					int event = item->component->playerAtt->fixedEvent(index);
					if ((event & (3 << bit)) >= (3 << bit))
						count++;
				}
			}
		}
		if (index == 0 && count < 2)
			return -1;
		else if (index == 1 && count < 4)
			return -1;
		else if (index == 2 && count < 6)
			return -1;
	} else if (type == AwardInfo::INVITECODE) {
		int index_code = 0;
		awardInfo = AwardInfoManager_AwardInfo(type, arg, &index_code);
		if (awardInfo == NULL)
			return -1;
		int value = item->component->playerAtt->fixedEvent(95);
		if (value & (1 << (index_code + 16))) {
			return -1;	
		}
		if ((value & 0x0FFFF) < arg) {
			return -1;
		}
	} else {
		return -1;
	}

	const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(awardInfo->award());
	if (goods == NULL || goods->type() != GoodsInfo::BOX)
		return -1;
	int32_t res[CONFIG_FIXEDARRAY];
	if (Item_Lottery(item, goods->id(), res, CONFIG_FIXEDARRAY) == -1)
		return -1;

	if (type == AwardInfo::ONLINE) {
		PlayerEntity_SetFixedEvent(item->component->player, (int)type, item->component->playerAtt->fixedEvent((int)type) | (1 << index));
	} else if (type == AwardInfo::COME) {
		PlayerEntity_SetFixedEvent(item->component->player, 12, item->component->playerAtt->fixedEvent(12) | (1 << (16 + index)));
	} else if (type == AwardInfo::TOTAL_COME) {
		PlayerEntity_SetFixedEvent(item->component->player, 44, item->component->playerAtt->fixedEvent(44) | (1 << index));
	} else if (type == AwardInfo::LEVEL) {
		PlayerEntity_SetFixedEvent(item->component->player, (int)type, item->component->playerAtt->fixedEvent((int)type) | (1 << index));
	} else if (type == AwardInfo::POWER) {
		PlayerEntity_SetFixedEvent(item->component->player, (int)type, item->component->playerAtt->fixedEvent((int)type) | (1 << index));
	} else if (type == AwardInfo::RMB) {
		PlayerEntity_SetFixedEvent(item->component->player, 9, item->component->playerAtt->fixedEvent(9) | (1 << index));
	} else if (type == AwardInfo::NEWYEAR_GIFT) {
		PlayerEntity_SetFixedEvent(item->component->player, 97, item->component->playerAtt->fixedEvent(97) | (1 << index));
	} else if (type == AwardInfo::VIP) {
		PlayerEntity_SetFixedEvent(item->component->player, 10, item->component->playerAtt->fixedEvent(10) | (1 << index));
		const GoodsInfo * goodsInfo = GoodsInfoManager_GoodsInfo(awardInfo->award());
		Item_ModifyRMB(item, -goodsInfo->rmb(), true, 25, 0, 0);
	} else if (type == AwardInfo::UNIT_RECHARGE) {
		PlayerEntity_SetDayEvent(item->component->player, 5, item->component->playerAtt->dayEvent(5) & ~(1 << 4));
		PlayerEntity_SetDayEvent(item->component->player, 5, item->component->playerAtt->dayEvent(5) | (1 << 5));
	} else if (type == AwardInfo::TOTAL_COST) {
		PlayerEntity_SetFixedEvent(item->component->player, 46, item->component->playerAtt->fixedEvent(46) | (1 << index));
	} else if (type == AwardInfo::RANDOM_RETURN_RMB) {
		PlayerEntity_SetFixedEvent(item->component->player, 66, item->component->playerAtt->fixedEvent(66) | (1 << index));
		Item_ModifyRMB(item, -awardInfo->arg(), true, 26, 0, 0);
	} else if (type == AwardInfo::FIRST_RECHARGE) {
		PlayerEntity_SetFixedEvent(item->component->player, 15, item->component->playerAtt->fixedEvent(15) | (1 << 2));
	} else if (type == AwardInfo::MISSION_ACTIVITY) {
		PlayerEntity_SetDayEvent(item->component->player, 70, item->component->playerAtt->dayEvent(70) | (1 << index));
	} else if (type == AwardInfo::ROOM_CHAPTER) {
		int bit = arg * 3 + index;
		int num = bit / 30;
		bit = bit % 30;
		PlayerEntity_SetFixedEvent(item->component->player, 89 + num, item->component->playerAtt->fixedEvent(89 + num) | (1 << bit));
	} else if (type == AwardInfo::INVITECODE) {
		int index_code = 0;
		awardInfo = AwardInfoManager_AwardInfo(type, arg, &index_code);

		int value = (item->component->playerAtt->fixedEvent(95) & 0x7fff0000) | (1 << (index_code + 16) );
		PlayerEntity_SetFixedEvent(item->component->player, 95, (item->component->playerAtt->fixedEvent(95) & 0xFFFF) | value);
	} else {
		return -1;
	}
	return 0;
}

static void CombineGoods(struct Item *item, ItemPackage::Begin begin) {
	int end = 0;
	if (begin == ItemPackage::GOODS)
		end = (int)ItemPackage::GOODS + item->package->validNumGoods();
	else
		end = (int)ItemPackage::GEM + item->package->validNumGem();
	for (int i = (int)begin; i < end; i++) {
		ItemInfo *cur = item->package->mutable_items(i);
		if (cur->type() != ItemInfo::GOODS)
			continue;

		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(cur->id());
		assert(goods != NULL);
		if (goods->repeat() <= 1 || cur->count() >= goods->repeat())
			continue;

		for (int j = i + 1; j < end; j++) {
			ItemInfo *next = item->package->mutable_items(j);
			if (next->type() != ItemInfo::GOODS || next->id() != cur->id())
				continue;

			cur->set_count(cur->count() + next->count());
			if (cur->count() > goods->repeat()) {
				next->set_count(cur->count() - goods->repeat());
				cur->set_count(goods->repeat());
				break;
			} else {
				next->set_type(ItemInfo::NONE);
			}
		}
	}
}

int Item_Arrange(struct Item *item, ItemPackage::Begin begin) {
	if (!Item_IsValid(item))
		return -1;

	if (begin == ItemPackage::EQUIPMENT) {
		if (item->equipDirty) {
			bool has = false;
			for (int i = 0; i < item->package->validNumEquipment(); i++) {
				const ItemInfo *cur = &item->package->items((int)begin + i);
				if (cur->type() == ItemInfo::EQUIPMENT) {
					has = true;
					break;
				}
			}
			if (has) {
				EquipComp::item = item;
				qsort(item->package->mutable_items((int)begin), item->package->validNumEquipment(), sizeof(ItemInfo), EquipComp::Compare);
			}
			item->equipDirty = false;
		} else {
			return 1;
		}
	} else if (begin == ItemPackage::GOODS) {
		if (item->goodsDirty) {
			bool has = false;
			for (int i = 0; i < item->package->validNumGoods(); i++) {
				const ItemInfo *cur = &item->package->items((int)begin + i);
				if (cur->type() == ItemInfo::GOODS) {
					has = true;
					break;
				}
			}
			if (has) {
				CombineGoods(item, ItemPackage::GOODS);
				qsort(item->package->mutable_items((int)begin), item->package->validNumGoods(), sizeof(ItemInfo), GoodsComp::Compare);
			}
			item->goodsDirty = false;
		} else {
			return 1;
		}
	} else if (begin == ItemPackage::GEM) {
		if (item->gemDirty) {
			bool has = false;
			for (int i = 0; i < item->package->validNumGem(); i++) {
				const ItemInfo *cur = &item->package->items((int)begin + i);
				if (cur->type() == ItemInfo::GOODS) {
					has = true;
					break;
				}
			}
			if (has) {
				CombineGoods(item, ItemPackage::GEM);
				qsort(item->package->mutable_items((int)begin), item->package->validNumGem(), sizeof(ItemInfo), GoodsComp::Compare);
			}
			item->gemDirty = false;
		} else {
			return 1;
		}
	} else {
		return -1;
	}

	return 0;
}

void Item_GenEquip(struct Item *item, int32_t id) {
	if (!Item_IsValid(item))
		return;

	if (!Item_HasItem(item, ItemInfo::GOODS, id, 1))
		return;

	const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(id);
	assert(goods != NULL);
	if (goods->type() != GoodsInfo::RECIPE_EQU)
		return;

	const EquipRecipe *recipe = EquipmentInfoManager_EquipRecipe(goods->arg(0));
	assert(recipe != NULL);

	for (int i = 0; i < recipe->materials_size(); i++) {
		int material = recipe->materials(i);
		if (material == -1)
			continue;
		if (!Item_HasItem(item, ItemInfo::GOODS, material, recipe->nums(i)))
			return;
	}

	const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(recipe->equipments((int)item->component->baseAtt->professionType() - 1));
	int cost = (equip->colorType() + 1) * (equip->colorType() + 1) * equip->equipmentLevel() * 1000;
	if(cost > item->package->money())
		return;

	Item_ModifyMoney(item, -cost);
	Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)id, 1);

	static char buf[CONFIG_FIXEDARRAY];
	char *index = buf;
	if (Config_ScribeHost() != NULL) {
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"itemID1\":\"%d", id);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"itemCount1\":\"1");
		index += strlen(index);
	}

	for (int i = 0; i < recipe->materials_size(); i++) {
		int material = recipe->materials(i);
		if (material == -1)
			continue;
		Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)material, recipe->nums(i));
		if (Config_ScribeHost() != NULL) {
			SNPRINTF2(buf, index, "\",\"itemID%d\":\"%d", i + 2, material);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"itemCount%d\":\"%d", i + 2, recipe->nums(i));
			index += strlen(index);
		}
	}

	static NetProto_GetRes gr;
	gr.Clear();
	int64_t equipID = Item_AddToPackage(item, ItemInfo::EQUIPMENT, recipe->equipments((int)item->component->baseAtt->professionType() - 1), 1, &gr);

	int64_t roleID = PlayerEntity_RoleID(item->component->player);
	char ch[1024];
	SNPRINTF1(ch, "11-%d-%d", recipe->equipments((int)item->component->baseAtt->professionType() - 1), 1);
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ch);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);

	int32_t player = PlayerEntity_ID(item->component->player);
	GCAgent_SendProtoToClients(&player, 1, &gr);

	if (Config_ScribeHost() != NULL) {
		SNPRINTF2(buf, index, "\",\"newEquipID\":\"%lld", (long long int)equipID);
		index += strlen(index);
		const EquipmentInfo *equip = Item_EquipInfo(item, equipID);
		SNPRINTF2(buf, index, "\",\"newEquipName\":\"%s\"}", equip->name().c_str());
	}
	if (Config_ScribeHost() != NULL) {
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "GenEquip", buf));
	}
}

static int GenGem(struct Item *item, int level, int type, map<int, int> *table) {
	int32_t cur = GoodsInfoManager_Gem(level, type);
	if (cur == -1)
		return -1;
	int32_t prev = GoodsInfoManager_Gem(level - 1, type);
	if (prev == -1)
		return -1;

	int has = Item_Count(item, ItemInfo::GOODS, prev);
	if (has < 0)
		has = 0;
	int count = has;
	map<int, int>::iterator it = table->find(prev);
	if (it != table->end())
		count += it->second;
	if (count < Config_GenGemCount())
		return -1;

	if (!Config_CanGenGem(level)) {
		Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)prev, 2);
		return -2;
	}

	if (has > 0) {
		Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)prev, Config_GenGemCount());
	}
	if (has < Config_GenGemCount()) {
		if (it != table->end()) {
			it->second -= (Config_GenGemCount() - has);
			if (it->second <= 0)
				table->erase(it);
		}
	}

	it = table->find(cur);
	if (it != table->end())
		it->second++;
	else
		table->insert(make_pair(cur, 1));

	static char buf[CONFIG_FIXEDARRAY];
	char *index = buf;
	if (Config_ScribeHost() != NULL) {
		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"itemID1\":\"%d", (int)prev);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"itemCount1\":\"%d", Config_GenGemCount());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"NewItemID\":\"%d", (int)cur);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"NewItemCount\":\"1\"}");
		int32_t player = PlayerEntity_ID(item->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "GenGem", buf));
	}
	return cur;
}

static void AddResToGenGem(int cur, int count, NetProto_GenGem *proto) {
	for (int i = 0; i < proto->results_size(); i++) {
		if (proto->results(i).id() == cur) {
			proto->mutable_results(i)->set_count(proto->results(i).count() + count);
			return;
		}
	}
	PB_ItemInfo *res = proto->add_results();
	res->set_type(PB_ItemInfo::GOODS);
	res->set_id(cur);
	res->set_count(count);
}

void Item_GenGem(struct Item *item, int level, int type, NetProto_GenGem::Way way) {
	if (!Item_IsValid(item))
		return;

	static NetProto_GenGem proto;
	proto.Clear();
	proto.set_way(way);
	proto.set_level(level);
	proto.set_type(type);

	static map<int, int> table;
	table.clear();

	int cost = 0;
	int success = 0;
	int fail = 0;

	switch(way) {
		case NetProto_GenGem::NEXT_LEVEL:
			{
				table.clear();
				while (true) {
					cost += Config_GenGemCost(level);
					if (cost > item->package->money()) {
						cost -= Config_GenGemCost(level);
						break;
					}
					int cur = GenGem(item, level, type, &table);
					if (cur == -1) {
						break;
					} else if (cur == -2) {
						fail++;
					} else {
						success++;
					}
				}
			}
			break;
		default:
			return;
	}

	Item_ModifyMoney(item, -cost);

	proto.set_success(success);
	proto.set_fail(fail);

	for (map<int, int>::iterator it = table.begin(); it != table.end(); it++) {
		Item_AddToPackage(item, ItemInfo::GOODS, it->first, it->second);

		int64_t roleID = PlayerEntity_RoleID(item->component->player);
		char ch[1024];
		SNPRINTF1(ch, "12-%d-%d", it->first, it->second);
		DCProto_SaveSingleRecord saveRecord;
		saveRecord.set_mapID(-23);
		saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
		saveRecord.mutable_record()->mutable_role()->set_name(ch);
		saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
		GCAgent_SendProtoToDCAgent(&saveRecord);

		AddResToGenGem(it->first, it->second, &proto);
	}
	int32_t player = PlayerEntity_ID(item->component->player);
	GCAgent_SendProtoToClients(&player, 1, &proto);
}

void Item_UseBaseWing(struct Item *item, bool use) {
	if (!Item_IsValid(item))
		return;

	int level = item->component->roleAtt->fightAtt().baseWingLevel();
	int degree = item->component->roleAtt->fightAtt().baseWingDegree();
	const BaseWing *info = EquipmentInfoManager_BaseWing(level, degree);
	assert(info != NULL);
	for (int i = 0; i < info->att_size(); i++)
		Fight_ModifyPropertyDelta(item->component->fight, (FightAtt::PropertyType)i, use ? info->att(i) : -info->att(i), 0);
}

void Item_UseWing(struct Item *item, int32_t id, bool use) {
	if (!Item_IsValid(item))
		return;

	const Wing *info = EquipmentInfoManager_Wing(id);
	if (info == NULL)
		return;

	if (id < 0 || id >= item->package->wings_size())
		return;
	int v = item->package->wings(id);
	if (v == -1 || v == 0)
		return;

	for (int i = 0; i < info->att_size(); i++)
		Fight_ModifyPropertyDelta(item->component->fight, (FightAtt::PropertyType)i, use ? info->att(i) : -info->att(i), 0);
}

int Item_BuyWing(struct Item *item, int32_t id, bool forever, float discount, BusinessInfo::CurrencyType type) {
	if (!Item_IsValid(item))
		return -1;

	const Wing *info = EquipmentInfoManager_Wing(id);
	if (info == NULL)
		return -1;
	// if (info->rmb() <= 0)
	// 	return -1;

	if (id < 0 || id >= item->package->wings_size())
		return -1;

	int v = item->package->wings(id);
	if (v == -2)
		return -1;

	int price = 0;
	if (type == BusinessInfo::RMB)
		price = info->rmb();
	else if (type == BusinessInfo::SUB_RMB)
		price = info->rmb();
	//	else if (type == BusinessInfo::MONEY)
	//		price = info->price();
	else if (type == BusinessInfo::LOVE_POINT)
		price = info->lovePoint();
	else if (type == BusinessInfo::PVP_SCORE)
		price = info->pvpScore();
	else if (type == BusinessInfo::GOD_SCORE)
		price = info->godScore();
	else if (type == BusinessInfo::FACTION_CONTRIBUTE)
		price = info->factionContribute();
	int cost = forever ? Config_ForeverWingPrice(price) : price;
	price = (int)(cost * discount);

	const ItemPackage *package = Item_Package(item);
	int contribute = FactionPool_GetRoleContribute(item->component->playerAtt);
	if (type == BusinessInfo::RMB || type == BusinessInfo::RMB) {
		//if (price >  package->rmb()) 
		if (!Item_HasRMB(item, price, 3))
			return -1;
		Item_ModifyRMB(item, -price, false, 14, id, forever ? 1 : 0);
	}
	//	else if (type == BusinessInfo::SUB_RMB) {
	//if (price > (package->rmb() + package->subRMB()))
	//		if (!Item_HasRMB(item, price))
	//			return -2;

	//		Item_ModifyRMB(item, -price, false, 14, id, forever ? 1 : 0);
	//	}
	//	else if (type == BusinessInfo::MONEY) {
	//		if (price > package->money())
	//			return -3;
	//		Item_ModifyMoney(item, -price);
	//	}
	else if (type == BusinessInfo::LOVE_POINT) {
		if (price > package->lovePoint())
			return -4;
		Item_ModifyLovePoint(item, -price);
	}
	else if (type == BusinessInfo::PVP_SCORE) {
		if (price > package->pkScore())
			return -5;
		Item_ModifyPKScore(item, -price);
	}
	else if (type == BusinessInfo::GOD_SCORE) {
		if (price > package->godScore())
			return -6;
		Item_ModifyGodScore(item, -price);
	}
	else if (type == BusinessInfo::FACTION_CONTRIBUTE) {
		if (price > contribute) {
			return -7;
		}
		FactionPool_UpateRoleContribute(item->component->playerAtt, -price);
	}else {
		return -8;
	}

	Item_AddToPackage(item, ItemInfo::WING, id, forever ? 1 : 0);

	int64_t roleID = PlayerEntity_RoleID(item->component->player);
	char ch[1024];
	SNPRINTF1(ch, "13-%d-%d", id, forever ? 1 : 0);
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ch);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);

	static char buf[CONFIG_FIXEDARRAY];
	char *index = buf;
	if (Config_ScribeHost() != NULL) {
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"wingID\":\"%d", info->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"wingName\":\"%s", info->name().c_str());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"startTime\":\"%d", (int)Time_TimeStamp());
		index += strlen(index);

		if (forever) {
			SNPRINTF2(buf, index, "\",\"forever\":\"true");
			index += strlen(index);
		} else {
			SNPRINTF2(buf, index, "\",\"forever\":\"false");
			index += strlen(index);
		}
		v = item->package->wings(id);
		if (v == -2) {
			SNPRINTF2(buf, index, "\",\"endTime\":\"0");
			index += strlen(index);
		} else {
			SNPRINTF2(buf, index, "\",\"endTime\":\"%d", v);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"existTime\":\"%d", (int)(v - Time_TimeStamp()));
			index += strlen(index);
		}
		SNPRINTF2(buf, index, "\"}");
	}
	if (Config_ScribeHost() != NULL) {
		int32_t player = PlayerEntity_ID(item->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "BuyWing", buf));
	}
	return 0;
}

void Item_UseFashion(struct Item *item, int32_t id, bool use) {
	if (!Item_IsValid(item))
		return;

	const FashionInfo *info = FashionInfoManager_FashionInfo(id);
	if (info == NULL)
		return;
	if (info->professionType() != -1 && (ProfessionInfo::Type)info->professionType() != item->component->baseAtt->professionType())
		return;

	if (id < 0 || id >= item->package->fashions_size())
		return;
	const FashionAsset *asset = &item->package->fashions(id);
	int v = asset->v();
	if (v == -1 || v == 0)
		return;

	for (int i = 0; i < info->att_size(); i++)
		Fight_ModifyPropertyDelta(item->component->fight, (FightAtt::PropertyType)i, use ? info->att(i) : -info->att(i), 0);
}

int Item_BuyFashion(struct Item *item, int32_t id, bool forever, float discount, BusinessInfo::CurrencyType type) {
	if (!Item_IsValid(item))
		return -1;

	const FashionInfo *info = FashionInfoManager_FashionInfo(id);
	if (info == NULL)
		return -1;
	if (info->professionType() != -1 && (ProfessionInfo::Type)info->professionType() != item->component->baseAtt->professionType())
		return -1;
	// if (info->rmb() <= 0)
	// 	return -1;

	if (id < 0 || id >= item->package->fashions_size())
		return -1;
	const FashionAsset *asset = &item->package->fashions(id);
	int v = asset->v();
	if (v == -2)
		return -1;

	int price = 0;
	if (type == BusinessInfo::RMB)
		price = info->rmb();
	else if (type == BusinessInfo::SUB_RMB)
		price = info->rmb();
	//	else if (type == BusinessInfo::MONEY)
	//		price = info->price();
	else if (type == BusinessInfo::LOVE_POINT)
		price = info->lovePoint();
	else if (type == BusinessInfo::PVP_SCORE)
		price = info->pvpScore();
	else if (type == BusinessInfo::GOD_SCORE)
		price = info->godScore();
	else if (type == BusinessInfo::FACTION_CONTRIBUTE)
		price = info->factionContribute();
	int cost = forever ? Config_ForeverWingPrice(price) : price;
	price = (int)(cost * discount);

	const ItemPackage *package = Item_Package(item);
	int contribute = FactionPool_GetRoleContribute(item->component->playerAtt);
	if (type == BusinessInfo::RMB || type == BusinessInfo::SUB_RMB) {
		//		if (price >  package->rmb()) 
		if (!Item_HasRMB(item, price, 3))
			return -1;
		Item_ModifyRMB(item, -price, false, 15, id, forever ? 1 : 0);
	}
	//else if (type == BusinessInfo::SUB_RMB) {
	//if (price > (package->rmb() + package->subRMB()))
	//	if (!Item_HasRMB(item, price))
	//		return -2;

	//	Item_ModifyRMB(item, -price, false, 15, id, forever ? 1 : 0);
	//}
	//	else if (type == BusinessInfo::MONEY) {
	//		if (price > package->money())
	//			return -3;
	//		Item_ModifyMoney(item, -price);
	//	}
	else if (type == BusinessInfo::LOVE_POINT) {
		if (price > package->lovePoint())
			return -4;
		Item_ModifyLovePoint(item, -price);
	}
	else if (type == BusinessInfo::PVP_SCORE) {
		if (price > package->pkScore())
			return -5;
		Item_ModifyPKScore(item, -price);
	}
	else if (type == BusinessInfo::GOD_SCORE) {
		if (price > package->godScore())
			return -6;
		Item_ModifyGodScore(item, -price);
	}
	else if (type == BusinessInfo::FACTION_CONTRIBUTE) {
		if (price > contribute) {
			return -7;
		}
		FactionPool_UpateRoleContribute(item->component->playerAtt, -price);
	}else {
		return -8;
	}

	Item_AddToPackage(item, ItemInfo::FASHION, id, forever ? 1 : 0);

	int64_t roleID = PlayerEntity_RoleID(item->component->player);
	char ch[1024];
	SNPRINTF1(ch, "14-%d-%d", id, forever ? 1 : 0);
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ch);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"fashionID\":\"%d", info->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"fashionName\":\"%s", info->name().c_str());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"startTime\":\"%d", (int)Time_TimeStamp());
		index += strlen(index);

		if (forever) {
			SNPRINTF2(buf, index, "\",\"forever\":\"true");
			index += strlen(index);
		} else {
			SNPRINTF2(buf, index, "\",\"forever\":\"false");
			index += strlen(index);
		}
		asset = &item->package->fashions(id);
		if (asset->v() == -2) {
			SNPRINTF2(buf, index, "\",\"endTime\":\"0");
			index += strlen(index);
		} else {
			SNPRINTF2(buf, index, "\",\"endTime\":\"%d", v);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"existTime\":\"%d", (int)(v - Time_TimeStamp()));
			index += strlen(index);
		}
		SNPRINTF2(buf, index, "\"}");
		int32_t player = PlayerEntity_ID(item->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "BuyFashion", buf));
	}

	return 0;
}

bool Item_MakeFashionHole(struct Item *item, int32_t id, int32_t index) {
	if (!Item_IsValid(item)) {
		return false;
	}

	const FashionInfo *info = FashionInfoManager_FashionInfo(id);
	if (info == NULL) {
		return false;	
	}

	if (id < 0 || id >= item->package->fashions_size()) {
		return false;
	}

	FashionAsset *fashion = item->package->mutable_fashions(id);
	if (fashion == NULL) {
		return false;
	}
	if (!(fashion->v() == -2 || fashion->v() > 0)) {
		return false;
	}

	if (info->professionType() != -1 && (ProfessionInfo::Type)info->professionType() != item->component->baseAtt->professionType()) {
		return false;
	}

	int size = fashion->runes_size();
	if (index < 0 || index >= size) {
		return false;
	}

	if (fashion->runes(index) != -2) {
		return false;
	}

	int64_t itemID = Config_MakeHoleNeedItemID(index);
	int itemNum = Config_MakeHoleNeedItemNum(index);
	if (itemID == -1 || itemNum == -1) {
		return false;
	}
	if (!Item_HasItem(item, ItemInfo::GOODS, itemID, itemNum)) {
		return false;
	}	
	Item_DelFromPackage(item, ItemInfo::GOODS, itemID, itemNum);

	fashion->set_runes(index, -1);

	return true;

}

bool Item_FashionInlay(struct Item *item, int32_t id, int32_t index, int32_t runeid, bool flag) {
	if (!Item_IsValid(item)) {
		return false;
	}

	const FashionInfo *info = FashionInfoManager_FashionInfo(id);
	if (info == NULL) {
		return false;	
	}

	if (id < 0 || id >= item->package->fashions_size()) {
		return false;
	}

	FashionAsset *fashion = item->package->mutable_fashions(id);
	if (!(fashion->v() == -2 ||fashion->v() > 0)) {
		return false;
	}

	if (info->professionType() != -1 && (ProfessionInfo::Type)info->professionType() != item->component->baseAtt->professionType()) {
		return false;
	}

	int size = fashion->runes_size();
	if (index < 0 || index >= size) {
		return false;
	}

	if (!Item_HasItem(item, ItemInfo::GOODS, runeid, 1)) {
		return false;
	}	

	if (GoodsInfo::RUNE != GoodsInfoManager_GoodsInfo(runeid)->type()) {
		return false;
	}

	if (fashion->runes(index) == -2) {
		return false;
	}

	if (flag) {
		if (!Item_FashionUnInlay(item, id, index)) {
			return false;
		}

		if (Equipment_EquipIsInBody(item->component->equipment, id, ItemInfo::FASHION)) {
			Equipment_DelStatusToBody(item->component->equipment, id);
		}

		fashion->set_runes(index, runeid);

		if (Equipment_EquipIsInBody(item->component->equipment, id, ItemInfo::FASHION)) {
			Equipment_AddStatusToBody(item->component->equipment, id, StatusInfo::ROOM_EXP);
		}

	} else {
		if (Equipment_EquipIsInBody(item->component->equipment, id, ItemInfo::FASHION)) {
			Equipment_DelStatusToBody(item->component->equipment, id);
		}

		fashion->set_runes(index, runeid);

		if (Equipment_EquipIsInBody(item->component->equipment, id, ItemInfo::FASHION)) {
			Equipment_AddStatusToBody(item->component->equipment, id, StatusInfo::ROOM_EXP);
		}
	}

	Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)runeid, 1);

	return true;
}

bool Item_FashionUnInlay(struct Item *item, int32_t id, int32_t index) {
	if (!Item_IsValid(item)) {
		return false;
	}

	const FashionInfo *info = FashionInfoManager_FashionInfo(id);
	if (info == NULL) {
		return false;	
	}

	if (id < 0 || id >= item->package->fashions_size()) {
		return false;
	}

	FashionAsset *fashion = item->package->mutable_fashions(id);
	if (fashion == NULL) {
		return false;
	}

	if (!(fashion->v() == -2 || fashion->v() > 0)) {
		return false;
	}

	if (info->professionType() != -1 && (ProfessionInfo::Type)info->professionType() != item->component->baseAtt->professionType()) {
		return false;
	}

	int size = fashion->runes_size();
	if (index < 0 || index >= size) {
		return false;
	}

	int runeID = fashion->runes(index);
	if (runeID < 0) {
		return false;
	}
	const GoodsInfo *goodInfo = GoodsInfoManager_GoodsInfo(runeID);
	if (goodInfo == NULL) {
		return false;
	}
	int lvl = goodInfo->arg(1);
	int num = Config_UnInlayNeedItemNum(lvl);
	if (num == -1) {
		return false;
	}
	if (!Item_HasItem(item, ItemInfo::GOODS, Config_UnInlayNeedItemID(), num)) {
		return false;
	}	
	Item_DelFromPackage(item, ItemInfo::GOODS, Config_UnInlayNeedItemID(), num);
	Item_AddToPackage(item, ItemInfo::GOODS, runeID, 1);

	int64_t roleID = PlayerEntity_RoleID(item->component->player);
	char ch[1024];
	SNPRINTF1(ch, "15-%d-%d", runeID, 1);
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ch);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);

	if (Equipment_EquipIsInBody(item->component->equipment, id, ItemInfo::FASHION)) {
		Equipment_DelStatusToBody(item->component->equipment, id);
	}

	fashion->set_runes(index, -1);

	if (Equipment_EquipIsInBody(item->component->equipment, id, ItemInfo::FASHION)) {
		Equipment_AddStatusToBody(item->component->equipment, id, StatusInfo::ROOM_EXP);
	}

	return true;
}


void Item_ModifyLovePoint(struct Item *item, int32_t delta) {
	if (!Item_IsValid(item)) {
		return;
	}

	item->package->set_lovePoint(item->package->lovePoint() + delta);
	if (item->package->lovePoint() < 0)
		item->package->set_lovePoint(0);

	if (delta != 0) {
		static NetProto_ModifyLovePoint modifyLovePoint;
		modifyLovePoint.Clear();
		modifyLovePoint.set_lovePoint(item->package->lovePoint());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifyLovePoint);
	}
}

void Item_ModifyBlessScore(struct Item *item, int32_t delta) {
	if (!Item_IsValid(item)) {
		return;
	}

	item->package->set_blessActive(item->package->blessActive() + delta);
	if (item->package->blessActive() < 0)
		item->package->set_blessActive(0);

	if (delta != 0) {
		static NetProto_ModifyBlessScore modify;
		modify.Clear();
		modify.set_bless(item->package->blessActive());
		int32_t id = PlayerEntity_ID(item->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modify);
	}
}

bool Item_BuyDurability(struct Item *item, int32_t  index) {
	if (!Item_IsValid(item)) {
		return false;
	}

	int rmb, v;
	if (!Config_BuyDurability(index, &rmb, &v))
		return false;

	int event = item->component->playerAtt->dayEvent(28) & 0x0FF;

	if (Time_WithinHour(Time_TimeStamp(), Config_GetDurabilityBegin1(), Config_GetDurabilityBegin1() + Config_GetDurabilityTime() / (1000 * 3600))) { 
		if (event & (1 << index)) {
			return false;
		}
		DEBUG_LOG("DDDDDDDDDDDDDDD");
	} else if (Time_WithinHour(Time_TimeStamp(), Config_GetDurabilityBegin2(), Config_GetDurabilityBegin2() + Config_GetDurabilityTime() / (1000 * 3600))) {
		if (event & (1 << (index + 4))) {
			return false;
		}
		index += 4;
	} else {
		return false;
	}

	//if (item->package->rmb() < rmb) {
	if (!Item_HasRMB(item, rmb)) {
		return false;
	}

	DEBUG_LOG("DDDDDDDDDDDDDDD%d", index);

	Item_ModifyDurability(item, v);
	Item_ModifyRMB(item, -rmb, false, 18, index, 0);
	PlayerEntity_SetDayEvent(item->component->player, 28, item->component->playerAtt->dayEvent(28) | (1 << index), false);

	static NetProto_GetRes gr;
	gr.Clear();
	PB_ItemInfo *unit = gr.add_items();
	unit->set_type(PB_ItemInfo::DURABILITY);
	unit->set_count(v);
	int32_t player = PlayerEntity_ID(item->component->player);
	GCAgent_SendProtoToClients(&player, 1, &gr);
	return true;
}

int Item_FlyPlan(struct Item *item, NetProto_FlyPlan* info) {
	if (!Item_IsValid(item)) {
		return -1;
	}

	if (info == NULL) {
		return -2;
	}

	int goodID = -1;
	int cost = Config_FlyPlay(info->index(), goodID);
	if (cost == -1) {
		return -3;
	}

	if (item->package->pkScoreActive() < cost) {
		return -4;
	}

	int32_t id = PlayerEntity_ID(item->component->player);

	item->package->set_pkScoreActive(item->package->pkScoreActive() - cost);
	if (cost != 0) {
		static NetProto_ModifyPKScore modifyPKScoreActive;
		modifyPKScoreActive.Clear();
		modifyPKScoreActive.set_value(item->package->pkScoreActive());
		GCAgent_SendProtoToClients(&id, 1, &modifyPKScoreActive);
	}

	static NetProto_GetRes gr;
	gr.Clear();
	Item_AddToPackage(item, ItemInfo::GOODS, (int64_t)goodID, 1, &gr);

	int64_t roleID = PlayerEntity_RoleID(item->component->player);
	char ch[1024];
	SNPRINTF1(ch, "16-%d-%d", goodID, 1);
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ch);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);
	/*
	   PB_ItemInfo *itemInfo = gr.add_items();
	   itemInfo->set_type(PB_ItemInfo::GOODS);
	   itemInfo->set_count(1);
	   itemInfo->set_id(goodID);
	   */	GCAgent_SendProtoToClients(&id, 1, &gr);

	return 0;
}

int Item_TransformLevelUp(struct Item* item, int id, bool activaction) {
	if (!Item_IsValid(item)) {
		return -1;
	}

	int quality = 0;
	int level = 0;
	bool flag = false;
	for (int i = 0; i < item->package->transforms_size(); ++i) {
		if (item->package->transforms(i).id() == id) {
			quality = item->package->transforms(i).quality();
			level = item->package->transforms(i).level();
			flag = true;
			break;
		}
	}

	if (activaction) {
		if (!flag) {
			for (int i = 0; i < item->package->transforms_size(); ++i) {
				if (item->package->transforms(i).id() == -1) {
					item->package->mutable_transforms(i)->set_id(id);
					item->package->mutable_transforms(i)->set_quality(0);
					item->package->mutable_transforms(i)->set_level(0);

					static NetProto_TransformActive proto;
					proto.Clear();
					proto.set_id(id);
					proto.set_index(i);
					int32_t player = PlayerEntity_ID(item->component->player);
					GCAgent_SendProtoToClients(&player, 1, &proto);
					break;
				}
			}
		}
		return 0;
	} else {
		if (!flag)
			return -1;
	}

	int qualityNext = 0;
	int levelNext = 0;
	const TransformInfo* itemInfo =  TransformInfoManager_TransformInfo(id, quality, level + 1);
	if (itemInfo == NULL) {
		itemInfo =  TransformInfoManager_TransformInfo(id, quality + 1, 0);
		if (itemInfo == NULL) {
			return -3;
		}else {
			qualityNext = quality + 1;
			levelNext = 0;
		}
	}else {
		qualityNext = quality;
		levelNext = level + 1;
	}

	itemInfo = TransformInfoManager_TransformInfo(id, quality, level);
	if (itemInfo == NULL) {
		return -4;
	}

	for (int i = 0; i < itemInfo->materials_size(); ++i) {
		int count = Item_Count(item, ItemInfo::GOODS, itemInfo->materials(i));
		if (count < itemInfo->count(i)) {
			return -5;
		}
	}

	for (int i = 0; i < itemInfo->materials_size(); ++i) {
		Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)itemInfo->materials(i), itemInfo->count(i), true);
	}

	for (int i = 0; i < item->package->transforms_size(); ++i) {
		if (item->package->transforms(i).id() == id) {
			item->package->mutable_transforms(i)->set_quality(qualityNext);
			item->package->mutable_transforms(i)->set_level(levelNext);
			break;
		}
	}

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"quality\":\"%d", qualityNext);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"level\":\"%d", levelNext);
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		int32_t player = PlayerEntity_ID(item->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "Transform", buf));
	}

	return  0;
}

int Item_PetHaloAttributeIncrease(struct Item* item, bool flag) {
	if (!Item_IsValid(item))
		return -1;

	const std::map<int, map<int, PB_PetHaloInfo> >* info = NPCInfoManager_PetHaloInfo();
	if (info == NULL)
		return -2;

	const PlayerAtt* att = PlayerEntity_Att(item->component->player);
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

		value[i] = itUnit->second.propertyValue() * (1 + att->haloValue(i) / 10000.0f);

	}

	for (int i = 0; i < (int)(sizeof(value) / sizeof(int)); i++) {
		if (flag) {
			Fight_ModifyProperty(item->component->fight, (FightAtt::PropertyType)i, value[i]);
		}else {
			Fight_ModifyProperty(item->component->fight, (FightAtt::PropertyType)i, -value[i]);
		}
	}

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *str = buf;

		SNPRINTF2(buf, str, "{\"roleID\":\"%lld", (long long int)item->component->roleAtt->baseAtt().roleID());
		str += strlen(str);
		SNPRINTF2(buf, str, "\",\"roleName\":\"%s", item->component->roleAtt->baseAtt().name());
		str += strlen(str);
		for (int index = 0; index < att->pets_size(); index++) {
			if (att->pets(index).id() == -1) 
				continue;
			PB_PlayerAtt playerAtt;
			att->ToPB(&playerAtt);
			int power = Fight_PetPower(&playerAtt, index);
			if(power < 0)
				continue;

			if (Config_ScribeHost() != NULL) {
				SNPRINTF2(buf, str, "\",\"petLevel\":\"%d", att->pets(index).pet_level());
				str += strlen(str);
				SNPRINTF2(buf, str, "\",\"petID\":\"%d", att->pets(index).id());
				str += strlen(str);
				SNPRINTF2(buf, str, "\",\"petPower\":\"%d", power);
				str += strlen(str);
			}
		}
		SNPRINTF2(buf, str, "\"}");
		int32_t player = PlayerEntity_ID(item->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(player), player, "Pet", buf));
	}

	return 0;
}

RidesAsset * Item_RidesAsset(struct Item *item, int32_t id)
{
	if(!Item_IsValid(item))
		return NULL;

	if(id < 0 || id >= item->package->rides_size())
		return NULL;

	RidesAsset *asset = item->package->mutable_rides(id);
	if(asset->model() < 0)
		return NULL;

	return asset;
}

int32_t Item_GetEmptyRidesPos(struct Item *item)
{
	if(!Item_IsValid(item))
		return -1;

	RidesAsset *asset = NULL;
	for(int i = 0, count = item->package->rides_size(); i < count; i++)
	{
		asset = item->package->mutable_rides(i);
		if(asset->model() < 0)
			return i;
	}

	return -2;
}
void Item_AddToGodPackage(struct Item *item, int64_t id) {
	DEBUG_LOG("sfasfsafsafaf");
	const GodShip *info = GodShipInfoManager_GodShipInfo((int32_t)id);
	if (info == NULL)
		return;

	DEBUG_LOG("godshipbegin");
	for (int i = 0; i < item->package->godShips_size(); i++) {
		if (item->package->godShips(i).id() == -1) {
			for (int j = 0; j < item->package->godShipsPackage_size(); j++) {
				if (item->package->godShipsPackage(j) == -1) {
					item->package->set_godShipsPackage(j, i);

					GodShipAsset *asset = item->package->mutable_godShips(i);
					asset->set_id(info->id());
					asset->set_exp(info->EXP());
					asset->set_quality(info->Quality());
					asset->set_level(info->Level());

					if (item->component->player != NULL) {
						static NetProto_GodPackage godPackage;
						godPackage.Clear();
						godPackage.set_pool(i);
						godPackage.set_package(j);
						godPackage.set_id(info->id());
						godPackage.set_exp(info->EXP());
						godPackage.set_level(info->Level());
						godPackage.set_quality(info->Quality());
						int32_t player = PlayerEntity_ID(item->component->player);
						GCAgent_SendProtoToClients(&player, 1, &godPackage);
					}
					break;
				}
			}
			break;
		}
	}
	DEBUG_LOG("godshipend");
}
int32_t Item_UseGodShip(struct Item *item, int32_t index) {
	if (!Item_IsValid(item))
		return -2;

	GodShipAsset *asset = item->package->mutable_godShips(index);
	if (asset == NULL)
		return -3;
	if (asset->id() == -1)
		return -4;
	const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
	if (info == NULL)
		return -5;

	int playerLevel = item->component->roleAtt->fightAtt().level();
	int i = Equipment_WearGodShip(item->component->equipment, playerLevel, info->Type(), index);
	if (i >= 0 && i < 6) {
		for (int j = 0; j < item->package->godShipsPackage_size(); j++) {
			if (item->package->godShipsPackage(j) == index) {
				item->package->set_godShipsPackage(j, -1);
			}
		}

		Item_GodShipAtt(item, index, true);
	}

	return i;

}

int32_t Item_UnUseGodShip(struct Item *item, int32_t index) {
	if (!Item_IsValid(item))
		return -1;

	for (int i = 0; i < item->package->godShipsPackage_size(); i++) {
		if (item->package->godShipsPackage(i) == -1) {
			int j = Equipment_UnWearGodShip(item->component->equipment, index);
			if (j > 0) {
				Item_GodShipAtt(item, index, false);
				item->package->set_godShipsPackage(i, index);
				return i;
			}
		}
	}
	return -1;
}

void Item_Swallow(struct Item *item, int32_t index, int32_t select) {
	GodShipAsset *assetadd = item->package->mutable_godShips(select);
	if (assetadd == NULL)
		return;
	if (assetadd->id() == -1)
		return;
	int cur = assetadd->exp();
	int quality = assetadd->quality();

	const GodShip *info = GodShipInfoManager_GodShipInfo(assetadd->id());
	if (info == NULL)
		return;
	int exp = info->EXP();

	if (index == 1) {
		int package = -1;
		for (package = 0; package < item->package->godShipsPackage_size(); package++) {
			if (item->package->godShipsPackage(package) == select)
				break;
		}
		if (package < 0 || package >= item->package->godShipsPackage_size())
			return;

		for (int i = package + 1; i < item->package->godShipsPackage_size(); i++) {
			if (item->package->godShipsPackage(i) != -1) {
				GodShipAsset *asset = item->package->mutable_godShips(item->package->godShipsPackage(i));
				if (asset == NULL)
					return;
				if (asset->id() == -1)
					return;
//				const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
//				if (info == NULL)
//					return;
//				cur += (asset->exp() + info->EXP()) / 2;
				cur += asset->exp() / 2;

				asset->set_id(-1);
				item->package->set_godShipsPackage(i, -1);
			}
		}

		int levelup = Config_LevelUp(assetadd->level(), exp, cur);
		assetadd->set_level(levelup);
		assetadd->set_exp(cur);

		if (item->component->player != NULL) {
			int i = -1;
			for (int j = 0; j < item->package->godShipsPackage_size(); j++) {
				if (item->package->godShipsPackage(j) == select) {
					i = j;
					break;
				}
			}
			static NetProto_GodPackage godPackage;
			godPackage.Clear();
			godPackage.set_package(i);
			godPackage.set_pool(select);
			godPackage.set_id(assetadd->id());
			godPackage.set_exp(assetadd->exp());
			godPackage.set_level(assetadd->level());
			godPackage.set_quality(assetadd->quality());
			int32_t player = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&player, 1, &godPackage);
		}

	} else if (index == 2) {
		int package = -1;
		for (package = 0; package < item->package->godShipsPackage_size(); package++) {
			if (item->package->godShipsPackage(package) == select)
				break;
		}

		for (int i = package + 1; i < item->package->godShipsPackage_size(); i++) {
			if (item->package->godShipsPackage(i) != -1 && item->package->godShips(item->package->godShipsPackage(i)).quality() < quality) {
				GodShipAsset *asset = item->package->mutable_godShips(item->package->godShipsPackage(i));
				if (asset == NULL)
					return;
				if (asset->id() == -1)
					return;
//				const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
//				if (info == NULL)
//					return;
//				cur += (asset->exp() + info->EXP()) / 2;
				cur += asset->exp() / 2;

				asset->set_id(-1);
				item->package->set_godShipsPackage(i, -1);
			}
		}

		int levelup = Config_LevelUp(assetadd->level(), exp, cur);
		assetadd->set_level(levelup);
		assetadd->set_exp(cur);

		if (item->component->player != NULL) {
			int i = -1;
			for (int j = 0; j < item->package->godShipsPackage_size(); j++) {
				if (item->package->godShipsPackage(j) == select) {
					i = j;
					break;
				}
			}
			static NetProto_GodPackage godPackage;
			godPackage.Clear();
			godPackage.set_package(i);
			godPackage.set_pool(select);
			godPackage.set_id(assetadd->id());
			godPackage.set_exp(assetadd->exp());
			godPackage.set_level(assetadd->level());
			godPackage.set_quality(assetadd->quality());
			int32_t player = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&player, 1, &godPackage);
		}
	} else {
		return;
	}
}

void Item_SingleGodShip(struct Item *item, int32_t index, int32_t *eat, int32_t size) {
	DEBUG_LOG("size:%d", size);

	for (int i = 0; i < size; i++) {
		DEBUG_LOG("eat:%d", eat[i]);
	}
	if (item == NULL)
		return;
	GodShipAsset *assetadd = item->package->mutable_godShips(index);
	if (assetadd == NULL)
		return;
	if (assetadd->id() == -1)
		return;
	int cur = assetadd->exp();

	const GodShip *info = GodShipInfoManager_GodShipInfo(assetadd->id());
	if (info == NULL)
		return;
	int exp = info->EXP();

	int cost = 0;

	for (int i = 0; i < size; i++) {
		GodShipAsset *asset = item->package->mutable_godShips(eat[i]);
		if (asset == NULL)
			return;
		const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
		if (info == NULL)
			return;
		cost += info->Quality() * info->Quality() * info->Level() * 100;
	}

	if (item->package->money() >= cost) {
		Item_ModifyMoney(item, -cost);

		for (int i = 0; i < size; i++) {
			GodShipAsset *asset = item->package->mutable_godShips(eat[i]);
			if (asset == NULL)
				return;
			if (asset->id() == -1)
				return;
			//			const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
//			if (info == NULL)
//				return;
//			cur += (asset->exp() + info->EXP()) / 2;
			cur += asset->exp() / 2;

			asset->set_id(-1);
			
			for (int j = 0; j < item->package->godShipsPackage_size(); j++) {
				if (item->package->godShipsPackage(j) == eat[i])
					item->package->set_godShipsPackage(j, -1);
			}
		}

		int levelup = Config_LevelUp(assetadd->level(), exp, cur);
		assetadd->set_level(levelup);
		assetadd->set_exp(cur);

		if (item->component->player != NULL) {
			int i = -1;
			for (int j = 0; j < item->package->godShipsPackage_size(); j++) {
				if (item->package->godShipsPackage(j) == index) {
					i = j;
					break;
				}
			}
			static NetProto_GodPackage godPackage;
			godPackage.Clear();
			godPackage.set_package(i);
			godPackage.set_pool(index);
			godPackage.set_id(assetadd->id());
			godPackage.set_exp(assetadd->exp());
			godPackage.set_level(assetadd->level());
			godPackage.set_quality(assetadd->quality());
			int32_t player = PlayerEntity_ID(item->component->player);
			GCAgent_SendProtoToClients(&player, 1, &godPackage);
		}
	}
	return;
}

void Item_ArrangeGodShip(struct Item *item) {
	if (!Item_IsValid(item))
		return;
	int a, b;
	for (a = 0; a < item->package->godShipsPackage_size() - 1; a++) {
		if (item->package->godShipsPackage(a) == -1) {
			for (b = a + 1; b < item->package->godShipsPackage_size(); b++) {
				if (item->package->godShipsPackage(b) != -1) {
					item->package->set_godShipsPackage(a, item->package->godShipsPackage(b));
					item->package->set_godShipsPackage(b, -1);
					break;
				}
			}
		}
	}
	for (a = 0; a < item->package->godShipsPackage_size(); a++) {
		DEBUG_LOG("index(%d) : %d", a, item->package->godShipsPackage(a));
	}
	int i, j, m;
	for (i = 0; i < item->package->godShipsPackage_size() - 1; i++) {
		DEBUG_LOG("indexi(%d)start", i);
		int pooli = item->package->godShipsPackage(i);
		if (pooli == -1)
			return;
		GodShipAsset *asseti = item->package->mutable_godShips(pooli);
		if (asseti == NULL || asseti->id() == -1)
			return;
		for (j = i+1;j < item->package->godShipsPackage_size(); j++) {
			DEBUG_LOG("indexj(%d)start", j);
			int poolj = item->package->godShipsPackage(j);
			if (poolj == -1)
				break;
			GodShipAsset *assetj = item->package->mutable_godShips(poolj);
			if (assetj == NULL || assetj->id() == -1)
				break;
			if (asseti->quality() != assetj->quality()) {
				if (asseti->quality() < assetj->quality()) {
					m = item->package->godShipsPackage(i);
					DEBUG_LOG("index(%d) : %d", i, m);
					item->package->set_godShipsPackage(i, item->package->godShipsPackage(j));
					DEBUG_LOG("index(%d) : %d", i, item->package->godShipsPackage(i));
					item->package->set_godShipsPackage(j, m);
					DEBUG_LOG("index(%d) : %d", j, item->package->godShipsPackage(j));
				} 
			} else if (asseti->level() != assetj->level()) {
				if (asseti->level() < assetj->level()) {
					m = item->package->godShipsPackage(i);
					DEBUG_LOG("index(%d) : %d", i, m);
					item->package->set_godShipsPackage(i, item->package->godShipsPackage(j));
					DEBUG_LOG("index(%d) : %d", i, item->package->godShipsPackage(i));
					item->package->set_godShipsPackage(j, m);
					DEBUG_LOG("index(%d) : %d", j, item->package->godShipsPackage(j));
				} 
			} else if (asseti->id() != assetj->id()) {
				if (asseti->id() > assetj->id()) {
					m = item->package->godShipsPackage(i);
					DEBUG_LOG("index(%d) : %d", i, m);
					item->package->set_godShipsPackage(i, item->package->godShipsPackage(j));
					DEBUG_LOG("index(%d) : %d", i, item->package->godShipsPackage(i));
					item->package->set_godShipsPackage(j, m);
					DEBUG_LOG("index(%d) : %d", j, item->package->godShipsPackage(j));
				} 
			}
		}

	}

	return;	
}

void Item_GodShipAtt(struct Item *item, int32_t index, bool add) {
	GodShipAsset *asset = item->package->mutable_godShips(index);
	if (asset == NULL)
		return;

	const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
	if (info == NULL)
		return;

	float level = (float)asset->level();

	float atk = (float)info->ATK();
	float def = (float)info->DEF();
	float maxhp = (float)info->MAXHP();
	float crit = (float)info->CRIT();
	float accuracy = (float)info->ACCURACY();
	float dodge = (float)info->DODGE();
	if (!add) {
		if (atk != 0) {
			atk = atk + atk * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::ATK, -atk, 0);
		} 
		if (def != 0) {
			def = def + def * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::DEF, -def, 0);
		} 
		if (maxhp != 0) {
			maxhp = maxhp + maxhp * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::MAXHP, -maxhp, 0);
		} 
		if (crit != 0) {
			crit = crit + crit * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::CRIT, -crit, 0);
		} 
		if (accuracy != 0) {
			accuracy = accuracy + accuracy * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::ACCURACY, -accuracy, 0);
		} 
		if (dodge != 0) {
			dodge = dodge + dodge * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::DODGE, -dodge, 0);
		}
	} else {
		if (atk != 0) {
			atk = atk + atk * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::ATK, atk, 0);
		} 
		if (def != 0) {
			def = def + def * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::DEF, def, 0);
		} 
		if (maxhp != 0) {
			maxhp = maxhp + maxhp * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::MAXHP, maxhp, 0);
		} 
		if (crit != 0) {
			crit = crit + crit * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::CRIT, crit, 0);
		} 
		if (accuracy != 0) {
			accuracy = accuracy + accuracy * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::ACCURACY, accuracy, 0);
		} 
		if (dodge != 0) {
			dodge = dodge + dodge * ( (level - 1) / 10 + (level - 1) / 20) * (1 + level / 15);
			Fight_ModifyPropertyDelta(item->component->fight, FightAtt::DODGE, dodge, 0);
		}
	}
}

int Item_RidesTrain(struct Item *item, int32_t index, bool high)
{
	if(!Item_IsValid(item))
		return -1;

	item->ridesTrain = -1;

	RidesAsset *ridesAsset = Item_RidesAsset(item, index);
	if(ridesAsset == NULL)
		return -2;

	if(ridesAsset->model() < 0)
		return -200;
	const RidesInfo *ridesInfo = RidesInfoManager_RidesInfo(ridesAsset->model(), ridesAsset->star());
	if(ridesInfo == NULL)
		return -201;

	int cost = Config_RidesBaseFood();
	int rmb = 0;
	if(high)
	{
		//if(!Item_HasRMB(item, Config_RidesHighRMB()))
		//	return -3;
		rmb += Config_RidesHighRMB();
		cost = Config_RidesHighFood();
	}

	int lockNum = 0;
	int canDown = 0;
	int32_t downIndex[6];
	int v = 0;
	for(int i = 0; i < 6; i++)
	{
		if(ridesAsset->lockAtt(i))
			lockNum++;
		else
		{
			v = (int)(ridesInfo->att(i) * 0.3f);
			if(v >= ridesAsset->att(i) - 5) //avoid to cannot down att
				continue;
			downIndex[canDown] = i;
			canDown++;
		}
	}

	if(lockNum >= 6)
	{
		DEBUG_LOGERROR("rides locked 6 att->%d", index);
		return -100;
	}

	if(canDown <= 0)
	{
		DEBUG_LOGERROR("rides canDown 0 att->%d", index);
		return -101;
	}

	int lockExRMB[] = {10, 25, 45, 70, 100};
	rmb += lockExRMB[lockNum];

	if(!Item_HasRMB(item, rmb))
		return -3;

	if(item->package->ridesFood() < cost)
		return -4;


	/*
	int up  = Time_Random(0, 6);
	int down = -1;
	int num = 0;
	while(true)
	{
		down = Time_Random(0, 6);
		num++;
		if(down == up)
			continue;
		if(num> 50)
			return -5;
	}
	*/
	int down = Time_Random(0, canDown);
	down = downIndex[down];
	int up = -1;
	int num = 0;
	while(true)
	{
		down = Time_Random(0, 6);
		num++;
		if(down == up)
			continue;
		if(num > 50)
			return -5;
	}

	if(up < 0 || up > 5)
		return -6;
	if(down < 0 || down > 5)
		return -7;

	int pv = Time_Random(1, 6);

	int costPotential = Config_RidesPotentialExpend();
	if(ridesAsset->potential() < costPotential)
		costPotential = ridesAsset->potential();

	static NetProto_RidesTrain ridesTrain;
	ridesTrain.Clear();
	ridesTrain.set_index(index);
	ridesTrain.set_high(high);
	ridesTrain.set_upProperty(up);
	ridesTrain.set_downProperty(down);

	item->package->set_ridesFood(item->package->ridesFood() - cost);
	ridesAsset->set_potential(ridesAsset->potential() - costPotential);
	//if(high)
	if(rmb > 0)
		Item_ModifyRMB(item, rmb, false, 35);

	for(int i = 0; i < 6; i++)
		item->ridesTrainDelta[i] = 0;

	FightAtt::PropertyType p = (FightAtt::PropertyType)up;

	v = pv + costPotential;
	if(p == FightAtt::MAXHP)
		v *= 10;

	ridesTrain.set_upv(v);
	item->ridesTrainDelta[up] = v;
	//ridesAsset->set_att(up, ridesAsset->att(up) + v);

	p = (FightAtt::PropertyType)down;
	v = pv;
	if(p == FightAtt::MAXHP)
		v *= 10;
	//Fight_ModifyProperty(fight, p, v, false);
	ridesTrain.set_downv(v);
	item->ridesTrainDelta[down] = -v;

	int32_t player = PlayerEntity_ID(item->component->player);
	GCAgent_SendProtoToClients(&player, 1, &ridesTrain);

	return 0;
}

static void Item_RidesUnLockAtt(struct Item *item, int index)
{
	RidesAsset *asset = Item_RidesAsset(item, index);
	if(asset == NULL)
		return;

	for(int i = 0; i < 6; i++)
		asset->set_lockAtt(i, false);

	static NetProto_RidesUnLockAtt proto;
	proto.Clear();
	proto.set_index(index);
	int32_t player = PlayerEntity_ID(item->component->player);
	GCAgent_SendProtoToClients(&player, 1, &proto);
}

int Item_RidesConfirmTrain(struct Item *item, NetProto_RidesConfirmTrain proto)
{
	if(!Item_IsValid(item))
		return -1;

	if(item->ridesTrain < 0)
		return -2;

	int index = item->ridesTrain;
	RidesAsset *asset = Item_RidesAsset(item, item->ridesTrain);
	if(asset == NULL)
		return -3;

	if(asset->model() < 0)
		return -4;

	const RidesInfo *info = RidesInfoManager_RidesInfo(asset->model(), asset->star());
	if(info == NULL)
		return -5;

	if(item->component->equipment == NULL)
		return -6;

	Fight *fight = item->component->fight;
	if(fight == NULL)
		return -7;

	const EquipmentAtt *equipAtt = Equipment_Att(item->component->equipment);
	if(equipAtt == NULL)
		return -8;

	if(equipAtt->rides() == index)
	{
		for(int i = 0; i < 6; i++)
			Fight_ModifyPropertyDelta(fight, (FightAtt::PropertyType)i, -asset->att(i), 0);
			//Fight_ModifyProperty(fight, (FightAtt::PropertyType)i, asset->att(i), false);
	}

	item->ridesTrain = -1;

	bool unlock = false;
	int v = 0;
	for(int i = 0; i < 6; i++)
	{
		asset->set_att(i, asset->att(i) + item->ridesTrainDelta[i]);
		if(!unlock)
		{
			v = (int)(info->att(i) * 0.3f);
			if(v >= asset->att(i) - 5)
			{
				unlock = true;
				Item_RidesUnLockAtt(item, index);
			}
		}
	}

	if(equipAtt->rides() == index)
	{
		for(int i = 0; i < 6; i++)
			Fight_ModifyPropertyDelta(fight, (FightAtt::PropertyType)i, asset->att(i), 0);
			//Fight_ModifyProperty(fight, (FightAtt::PropertyType)i, asset->att(i), true);
	}

	return 0;
}

int Item_RidesInherit(struct Item *item, NetProto_RidesInherit proto)
{
	if(!Item_IsValid(item))
		return -1;

	item->ridesTrain = -1;

	if(proto.ridesExpend_size() > 10)
		return -2;

	RidesAsset *ridesAsset = NULL;

	int64_t exp = 0;
	const RidesInfo *info = NULL;
	int i_index = -1;
	for(int i = 0, count = proto.ridesExpend_size(); i < count; i++)
	{
		i_index = proto.ridesExpend(i);
		if(i_index == proto.target())
			return -3;
		//ridesAsset = Item_RidesAsset(item, proto.ridesExpend(i));
		ridesAsset = Item_RidesAsset(item, i_index);
		if(ridesAsset == NULL)
			return -4;
		if(ridesAsset->model() < 0)
			return -5;

		info = RidesInfoManager_RidesInfo(ridesAsset->model(), ridesAsset->star());
		if(info == NULL)
			return -6;

		exp += ((info->baseEXP() + ridesAsset->exp()) / 4);
		ridesAsset->set_model(-1);
	}

	//proto.clear_ridesExpend();

	ridesAsset = Item_RidesAsset(item, proto.target());
	if(ridesAsset == NULL)
		return -7;

	ridesAsset->set_exp(ridesAsset->exp() + exp);

	exp = ridesAsset->exp();
	int num = 0;
	int need = 0;
	int level = ridesAsset->level();
	int potential = 0;
	while(num < 10000)
	{
		num++;
		level += 1;
		need = (int)(((float)5 / 3 * level * level * level + 45 * level * level - (float)140 / 3 * level) * (info->quality() + 1));
		DEBUG_LOGERROR("need->%d", need);
		if(need <= exp)
		{
			potential += 200 * (1 + (float)(2 * (level - 1)) / 50);
			continue;
		}
		else
		{
			level--;
			break;
		}
	}

	ridesAsset->set_level(level);
	ridesAsset->set_potential(ridesAsset->potential() + potential);

	proto.set_curExp(ridesAsset->exp());
	proto.set_level(ridesAsset->level());
	proto.set_potential(ridesAsset->potential());

	int32_t player = PlayerEntity_ID(item->component->player);
	GCAgent_SendProtoToClients(&player, 1, &proto);

	return 0;
}

int Item_RidesUP(struct Item *item, NetProto_RidesUP proto)
{
	if(!Item_IsValid(item))
		return -1;
	item->ridesTrain = -1;

	RidesAsset *asset = Item_RidesAsset(item, proto.index());
	if(asset == NULL)
		return -2;

	if(asset->model() < 0)
		return -3;

	const RidesInfo *info = RidesInfoManager_RidesInfo(asset->model(), asset->star() + 1);
	if(info == NULL)
		return -4;

	if(!Item_IsEnough(item, ItemInfo::GOODS, info->tpGoods(), info->tpCount()))
		return -5;

	Item_DelFromPackage(item, ItemInfo::GOODS, (int64_t)info->tpGoods(), info->tpCount());

	if(!Time_CanPass(info->tpSuccessRate()))
		return -6;

	asset->set_star(info->tpStar());

	proto.set_star(asset->star());

	int32_t player = PlayerEntity_ID(item->component->player);
	GCAgent_SendProtoToClients(&player, 1, &proto);

	return 0;
}
