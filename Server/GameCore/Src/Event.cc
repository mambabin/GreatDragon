#include "Event.hpp"
#include "GCAgent.hpp"
#include "PlayerPool.hpp"
#include "GMPool.hpp"
#include "Time.hpp"
#include "AwardInfoManager.hpp"
#include "DataManager.hpp"
#include "FactionPool.hpp"
#include <list>
#include <set>
#include <vector>
#include <ctime>
#include <sys/types.h>

using namespace std;

#define MIN_INTERVAL 5

struct forbidData {
	int64_t roleID;
	int32_t endTime;
};

struct ForbidComp {
	bool operator()(const forbidData& value1, const forbidData& value2) {
		return value1.roleID < value2.roleID;
	}	
};

list<stNotice> listNotice;
list<stForbid> listForbid;
set<forbidData, ForbidComp> playerNoTalking;
set<forbidData, ForbidComp> playerFreeze;
static int interval = 0;

list<DCProto_GMInfo> listGMData;
set<string> forbidMessage;

bool Event_NoticeOperator(NetProto_GMNoticeMgr* info, int32_t id) {
	if (info == NULL)
		return false;
	
	if (info->flag()) {
		for (list<stNotice>::iterator itor = listNotice.begin(); itor != listNotice.end(); ++itor) {
			NetProto_GMNotice* gmNotice = info->add_notice();
			gmNotice->set_id(itor->id);
			gmNotice->set_startTime(itor->startTime);
			gmNotice->set_endTime(itor->endTime);
			gmNotice->set_hz(itor->hz);
			gmNotice->set_content(itor->content);
			gmNotice->set_state2(itor->flag);

			if (itor->startTime <= Time_TimeStamp() && itor->endTime >= Time_TimeStamp() && itor->flag) {
				gmNotice->set_state1(true);
			}else {
				gmNotice->set_state1(false);
			}
		}
		return true;
	}

	for (int i = 0; i < info->notice_size(); ++i) {
		if (info->notice(i).op() == NetProto_GMNotice::ADD) {
			list<stNotice>::iterator itor = listNotice.begin();
			for (; itor != listNotice.end(); ++itor) {
				if (itor->content.compare(info->notice(i).content().c_str()) == 0) {
					break;
				}
			}
			if (itor == listNotice.end()) {
				stNotice notice;
				notice.id = Event_CreateGMID();
				notice.startTime = info->notice(i).startTime();	
				notice.endTime = info->notice(i).endTime();
				notice.hz = info->notice(i).hz();
				notice.content = info->notice(i).content();
				notice.GM = GMPool_IDByAccountID(id);
				notice.flag = info->notice(i).state2();
				if (notice.startTime <= Time_TimeStamp() && notice.endTime >= Time_TimeStamp() && notice.flag) {
					info->mutable_notice(i)->set_state1(true);
				}else {
					info->mutable_notice(i)->set_state1(false);
				}
				info->mutable_notice(i)->set_id(notice.id);

				if (notice.startTime > Time_TimeStamp()) {
					notice.recentTime = notice.startTime;
				} else {
					notice.recentTime = (((Time_TimeStamp() - notice.startTime) / notice.hz) + 1) * notice.hz + notice.startTime;
				}
				listNotice.push_back(notice);
				{
					static DCProto_GMSaveData gmSaveData;
					gmSaveData.Clear();
					gmSaveData.set_addOrDel(true);
					DCProto_GMData *gmData = gmSaveData.mutable_gmData();
					gmData->set_id(notice.id);
					gmData->set_roleID(-1);
					gmData->set_name(notice.content);
					gmData->set_level(notice.hz);
					gmData->set_startTime(notice.startTime);
					gmData->set_endTime(notice.endTime);
					gmData->set_GM(notice.GM);
					gmData->set_flag(notice.flag);
		
					GCAgent_SendProtoToDCAgent(&gmSaveData);
				}
			}
		}else if (info->notice(i).op() == NetProto_GMNotice::DEL) {
			for (list<stNotice>::iterator itor = listNotice.begin(); itor != listNotice.end();) {
				if (itor->id == info->notice(i).id()) {
					listNotice.erase(itor++);
					{
						static DCProto_GMSaveData gmSaveData;
						gmSaveData.Clear();
						gmSaveData.set_addOrDel(false);
						DCProto_GMData *gmData = gmSaveData.mutable_gmData();
						gmData->set_id(info->notice(i).id());
			
						GCAgent_SendProtoToDCAgent(&gmSaveData);
					}
				}else {
					itor++;
				}
			}
		}else if (info->notice(i).op() == NetProto_GMNotice::ALTER) {
			for (list<stNotice>::iterator itor = listNotice.begin(); itor != listNotice.end();) {
				if (itor->id == info->notice(i).id()) {
					itor->startTime = info->notice(i).startTime();	
					itor->endTime = info->notice(i).endTime();
					itor->hz = info->notice(i).hz();
					itor->content = info->notice(i).content();
					itor->GM = GMPool_IDByAccountID(id);
					itor->flag = info->notice(i).state2();
					if (itor->startTime <= Time_TimeStamp() && itor->endTime >= Time_TimeStamp() && itor->flag) {
						info->mutable_notice(i)->set_state1(true);
					}else {
						info->mutable_notice(i)->set_state1(false);
					}
					if (itor->startTime > Time_TimeStamp()) {
						itor->recentTime = itor->startTime;
					} else {
						itor->recentTime = (((Time_TimeStamp() - itor->startTime) / itor->hz) + 1) * itor->hz + itor->startTime;
					}
					{
						static DCProto_GMSaveData gmSaveData;
						gmSaveData.Clear();
						gmSaveData.set_addOrDel(false);
						DCProto_GMData *gmData = gmSaveData.mutable_gmData();
						gmData->set_id(info->notice(i).id());
			
						GCAgent_SendProtoToDCAgent(&gmSaveData);
					}
					{
						static DCProto_GMSaveData gmSaveData;
						gmSaveData.Clear();
						gmSaveData.set_addOrDel(true);
						DCProto_GMData *gmData = gmSaveData.mutable_gmData();
						gmData->set_id(info->notice(i).id());
						gmData->set_roleID(-1);
						gmData->set_name(info->notice(i).content().c_str());
						gmData->set_level(info->notice(i).hz());
						gmData->set_startTime(info->notice(i).startTime());
						gmData->set_endTime(info->notice(i).endTime());
						gmData->set_GM(GMPool_IDByAccountID(id));
						gmData->set_flag(info->notice(i).state2());
			
						GCAgent_SendProtoToDCAgent(&gmSaveData);
					}
					break;
				}else {
					itor++;
				}
			}
		}
	}

	interval = listNotice.begin()->recentTime;
	for (list<stNotice>::iterator iter = listNotice.begin(); iter != listNotice.end(); iter++) {
		if (interval > iter->recentTime) {
			interval = iter->recentTime;
		}
	}
	return true;
}


bool Event_AddGMInfo(stForbid& st, bool flag) {
	if (st.roleID == -1) {
		stNotice notice;
		notice.id = st.id;
		notice.startTime = st.startTime;
		notice.endTime = st.endTime;
		notice.hz = st.level;
		notice.content = st.name;
		notice.GM = st.GM;
		notice.flag = st.flag;
		if (notice.startTime > Time_TimeStamp()) {
			notice.recentTime = notice.startTime;
		} else {
			notice.recentTime = (((Time_TimeStamp() - notice.startTime) / notice.hz) + 1) * notice.hz + notice.startTime;
		}
		listNotice.push_back(notice);
	
		interval = listNotice.begin()->recentTime;
		for (list<stNotice>::iterator iter = listNotice.begin(); iter != listNotice.end(); iter++) {
			if (interval > iter->recentTime) {
				interval = iter->recentTime;
			}
		}
	}else {
		listForbid.push_back(st);
		forbidData data;
		data.roleID = st.roleID;
		data.endTime = st.endTime;
		if (st.flag) {
			playerNoTalking.insert(data);
		} else {
			playerFreeze.insert(data);
			PlayerEntity* entity = PlayerEntity_PlayerByRoleID(st.roleID);
			if (entity != NULL) {
				PlayerEntity_Logout(entity, true);
			}
		}
	}

	if (flag) {
		static DCProto_GMSaveData gmSaveData;
		gmSaveData.Clear();
		gmSaveData.set_addOrDel(true);
		DCProto_GMData *gmData = gmSaveData.mutable_gmData();
		gmData->set_id(st.id);
		gmData->set_roleID(st.roleID);
		gmData->set_name(st.name);
		gmData->set_level(st.level);
		gmData->set_profession(st.profession);
		gmData->set_startTime(st.startTime);
		gmData->set_endTime(st.endTime);
		gmData->set_GM(st.GM);
		gmData->set_flag(st.flag);
		
		GCAgent_SendProtoToDCAgent(&gmSaveData);
	} else {
		if (Time_TimeStamp() >= st.endTime && st.endTime != 0) {
			if (st.roleID != -1) {
				Event_DelForbid(st.id);
			}
		}
	}
	return true;
}

bool Event_DelForbid(int id) {
	for (list<stForbid>::iterator iter = listForbid.begin(); iter != listForbid.end(); ++iter) {
		if (iter->id == id) {

			forbidData data;
			data.roleID = iter->roleID;
			data.endTime = iter->endTime;
			if (iter->flag) {
				playerNoTalking.erase(data);
			} else {
				playerFreeze.erase(data);
			}
			listForbid.erase(iter);

			static DCProto_GMSaveData gmSaveData;
			gmSaveData.Clear();
			gmSaveData.set_addOrDel(false);
			DCProto_GMData *gmData = gmSaveData.mutable_gmData();
			gmData->set_id(id);
		
			GCAgent_SendProtoToDCAgent(&gmSaveData);
			return true;
		}
	}
	return false;
}

bool Event_DelForbid(int roleID, bool flag) {
	for (list<stForbid>::iterator iter = listForbid.begin(); iter != listForbid.end(); ++iter) {
		if (iter->roleID == roleID && iter->flag == flag) {
			static DCProto_GMSaveData gmSaveData;
			gmSaveData.Clear();
			gmSaveData.set_addOrDel(false);
			DCProto_GMData *gmData = gmSaveData.mutable_gmData();
			gmData->set_id(iter->id);
		
			GCAgent_SendProtoToDCAgent(&gmSaveData);
			
			forbidData data;
			data.roleID = iter->roleID;
			data.endTime = iter->endTime;
			if (iter->flag) {
				playerNoTalking.erase(data);
			} else {
				playerFreeze.erase(data);
			}
			listForbid.erase(iter);

			return true;
		}
	}
	return false;

}

int Event_CreateGMID() {
	int id = -1;
	for (list<stForbid>::iterator iter = listForbid.begin(); iter != listForbid.end(); ++iter) {
		if (id < iter->id) {
			id = iter->id;
		}
	}
	for (list<stNotice>::iterator iter = listNotice.begin(); iter != listNotice.end(); ++iter) {
		if (id < iter->id) {
			id = iter->id;
		}
	}
	return ++id;
}

bool Event_GMRequest(NetProto_GMRequest& gmRequest) {
	if (gmRequest.select() == NetProto_GMRequest::NOTALKING) {
		for (list<stForbid>::iterator iter = listForbid.begin(); iter != listForbid.end(); ++iter) {
			if (iter->flag) {
				NetProto_GMForbid *forbid = gmRequest.add_forbid();
				forbid->set_id(iter->id);
				forbid->set_roleID(iter->roleID);
				forbid->set_name(iter->name);
				forbid->set_level(iter->level);
				forbid->set_professionType(PB_ProfessionInfo::Type(iter->profession));
				forbid->set_GM(iter->GM);
				forbid->set_startTime(iter->startTime);
				forbid->set_endTime(iter->endTime);
				forbid->set_select(NetProto_GMForbid::NOTALKING);
			}
		}
		return true;
	}

	if (gmRequest.select() == NetProto_GMRequest::FREEZE) {
		for (list<stForbid>::iterator iter = listForbid.begin(); iter != listForbid.end(); ++iter) {
			if (!iter->flag) {
				NetProto_GMForbid *forbid = gmRequest.add_forbid();
				forbid->set_id(iter->id);
				forbid->set_roleID(iter->roleID);
				forbid->set_name(iter->name);
				forbid->set_level(iter->level);
				forbid->set_professionType(PB_ProfessionInfo::Type(iter->profession));
				forbid->set_GM(iter->GM);
				forbid->set_startTime(iter->startTime);
				forbid->set_endTime(iter->endTime);
				forbid->set_select(NetProto_GMForbid::FREEZE);
			}
		}
		return true;
	}
	
	if (gmRequest.select() == NetProto_GMRequest::ALL) {
		for (list<stForbid>::iterator iter = listForbid.begin(); iter != listForbid.end(); ++iter) {
			NetProto_GMForbid *forbid = gmRequest.add_forbid();
			forbid->set_id(iter->id);
			forbid->set_roleID(iter->roleID);
			forbid->set_name(iter->name);
			forbid->set_level(iter->level);
			forbid->set_professionType(PB_ProfessionInfo::Type(iter->profession));
			forbid->set_GM(iter->GM);
			forbid->set_startTime(iter->startTime);
			forbid->set_endTime(iter->endTime);
			forbid->set_select(iter->flag ? NetProto_GMForbid::NOTALKING : NetProto_GMForbid::FREEZE);
		}
		return true;
	}
	return false;
}

bool Event_IsNoTalking(int64_t roleID) {
	forbidData data;
	data.roleID = roleID;
	set<forbidData, ForbidComp>::iterator iter = playerNoTalking.find(data);
	if (iter == playerNoTalking.end()) {
		return false;
	}
	if (Time_TimeStamp() >= iter->endTime && iter->endTime != 0) {
		Event_DelForbid(roleID, true);
		return false;
	}
	return true;
}

bool Event_IsFreeze(int64_t roleID) {
	forbidData data;
	data.roleID = roleID;
	set<forbidData, ForbidComp>::iterator iter = playerFreeze.find(data);
	if (iter == playerFreeze.end()) {
		return false;
	}
	if (Time_TimeStamp() >= iter->endTime && iter->endTime != 0) {
		Event_DelForbid(roleID, false);
		return false;
	}
	return true;
}

void Event_Notice(void *arg) {
	time_t cur = Time_TimeStamp();
	if (interval - cur > MIN_INTERVAL) {
		return;
	}
	
	for (list<stNotice>::iterator iter = listNotice.begin(); iter != listNotice.end(); iter++) {
		if (iter->endTime < cur && iter->endTime != 0) {
			continue;
		}
		
		if (abs(iter->recentTime - cur) <= MIN_INTERVAL && iter->flag) {
			//send
			static NetProto_Message message;
			message.Clear();
			message.set_content(iter->content);
			message.set_count(1);
			GCAgent_SendProtoToAllClients(&message);
			iter->recentTime += iter->hz;
		}
	}
	
	interval = listNotice.begin()->recentTime;
	for (list<stNotice>::iterator iter = listNotice.begin(); iter != listNotice.end(); ++iter) {
		if (interval > iter->recentTime) {
			interval = iter->recentTime;
		}
	}
}

/*
static bool WithinDays(int y1, int m1, int d1, int y2, int m2, int d2) {
	time_t cur = Time_TimeStamp();
	tm curTM = *Time_LocalTime(&cur);
	int curY = curTM.tm_year + 1900;
	int curM = curTM.tm_mon + 1;
	int curD = curTM.tm_mday;
	if (curY < y1 || curY > y2) 
		return false;
	if ((curY == y1 && curM < m1) || (curY == y2 && curM > m2))
		return false;
	if ((curY == y1 && curM == m1 && curD < d1) || (curY == y2 && curM == m2 && curD > d2))
		return false;
	return true;
}
*/

int Event_DailyMissionAward() {
	/*
	int y1 = 2014, m1 = 4, d1 = 16;
	int y2 = 2014, m2 = 4, d2 = 27;
	if (WithinDays(y1, m1, d1, y2, m2, d2)) {
		int value = 2;
		return value;
	} else {
		return 1;
	}
	*/
	return 1;
}

int Event_Goods(int32_t *goods, size_t size) {
	/*
	int y1 = 2014, m1 = 5, d1 = 6;
	int y2 = 2014, m2 = 5, d2 = 20;
	if (WithinDays(y1, m1, d1, y2, m2, d2)) {
		// UP TO 4
		int32_t goods_[] = {397, 398, 399};
		size_t n = sizeof(goods_) / sizeof(goods_[0]);
		if (n > size)
			return -1;
		for (size_t i = 0; i < n; i++)
			goods[i] = goods_[i];
		return (int)n;
	} else {
		return -1;
	}
	*/
	return -1;
}

int Event_ExtraWorldBoss(time_t cur, int begin[]) {
	if(Config_InActiveTime(14)) {
		begin[0] = 22;
		return 1;
	} else {
		return 0;
	}
}

void Event_CheckFactionWarWinnerDesignation(struct PlayerEntity *entity, bool notice) {
	if (!PlayerEntity_IsValid(entity))
		return;

	struct Component *component = PlayerEntity_Component(entity);
	int designation = 44;
	if (PlayerEntity_HasDesignation(entity, designation)) {
		string name;
		int64_t hurt;
		if (PlayerPool_GetWinFactionFight(MapPool_FactionWarNum(), &name, &hurt)) {
			if (name != component->playerAtt->faction()
					|| FactionPool_GetMemOffic(name.c_str(), component->baseAtt->roleID()) != 1) {
				PlayerEntity_DelDesignation(entity, designation, notice);
			}
		}
	}
}

void Event_ResetDoubleRecharge(struct PlayerEntity *entity) {
	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL)
		return;
	if(Config_InActiveTime(18)) {
		if ((component->playerAtt->fixedEvent(15) & (1 << 10)) == 0) {
			for(int index = 3; index < 10; ++index) {
				if (component->playerAtt->fixedEvent(15) & (1 << index))
					PlayerEntity_SetFixedEvent(entity, 15, component->playerAtt->fixedEvent(15) & (~(1 << index)), true);
			}   
			PlayerEntity_SetFixedEvent(entity, 15, component->playerAtt->fixedEvent(15) | (1 << 10));
		}   
	}
}   
void Event_RecieveRoses(struct PlayerEntity *entity) {
	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL)
		return;
	if(Config_InActiveTime(15)) {
		if ((component->playerAtt->dayEvent(5) & (1 << 7)) == 0) {
			const AwardInfo * award = AwardInfoManager_AwardInfo(AwardInfo::REDROSE, 0);
			if (award != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
				PlayerEntity_SetDayEvent(entity, 5, component->playerAtt->dayEvent(5) | (1 << 7));
			}
		}   
	}   
} 

void Event_RealGod(struct PlayerEntity *entity, int event) {
	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL)
		return;
	if(Config_InActiveTime(17) && (event >= 10)) {
		if ((component->playerAtt->dayEvent(5) & (1 << 8)) == 0) {
			int64_t roleID = PlayerEntity_RoleID(entity);
			const AwardInfo *award = PlayerPool_GodAwardInfoByRoleID(roleID);
			if (award != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
				PlayerEntity_SetDayEvent(entity, 5, component->playerAtt->dayEvent(5) | (1 << 8));
			}
		}   
	}   
}

void Event_AwardFromSky_Cost(struct Item *item, int64_t delta) {
	if (Config_InActiveTime(9)) {
		if (delta <= 0)
			return;
		struct Component *component = Item_Component(item);
		if(component == NULL)
			return;

		int64_t total = delta + ((component->playerAtt->fixedEvent(94) & 0xff0000) >> 16);
		int count = total / 200;
		if (count > 0) {
			PlayerEntity_AddMail(component->player, ItemInfo::GOODS, Config_StrongStone(), count, Config_Words(0), Config_Words(92));
			PlayerEntity_SetFixedEvent(component->player, 94, component->playerAtt->fixedEvent(94) + count, true);
		}
		total = total % 200;
		PlayerEntity_SetFixedEvent(component->player, 94, (component->playerAtt->fixedEvent(94) & (0xff00ffff)) | (total << 16));
	}
}

static void AwardFromSky(struct PlayerEntity *entity) {
	struct Component *component = PlayerEntity_Component(entity);
	int prevCount = (component->playerAtt->fixedEvent(35) & 0xffff0000) >> 16;
	int curCount = prevCount + 1;
	PlayerEntity_SetFixedEvent(component->player, 35, component->playerAtt->fixedEvent(35) + (1 << 16), true);
	int delta = 0;
	if (prevCount < 5 && curCount >= 5)
		delta = 5;
	PlayerEntity_SetFixedEvent(component->player, 94, component->playerAtt->fixedEvent(94) - 1 + delta, true);
	const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::AWARD_FROM_SKY_TOP, 0);
	if (award != NULL) {
		if (prevCount < award->arg() && curCount >= award->arg()) {
			PlayerEntity_AddMail(component->player, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
		}
	}
}

bool Event_AwardFromSky(struct PlayerEntity *entity) {
	if (Config_InActiveTime(9)) {
		if (!PlayerEntity_IsValid(entity))
			return false;
		struct Component *component = PlayerEntity_Component(entity);

		int count = component->playerAtt->fixedEvent(94) & 0xffff;
		if (count <= 0)
			return false;

		int32_t results[CONFIG_FIXEDARRAY];
		int res = Item_Lottery(component->item, 482, results, CONFIG_FIXEDARRAY);
		if (res == -1)
			return false;

		AwardFromSky(entity);
		return true;
	} else {
		return false;
	}
}

/*void Event_NewYearGift(struct Item *item, int64_t delta) {
	if (!Item_IsValid(item))
		return;
	if(Config_InActiveTime(19)) {
		struct Component *component = Item_Component(item);
		if(component == NULL)
			return;
		int rmb = component->playerAtt->dayEvent(68);
		const vector<const AwardInfo *> * award = AwardInfoManager_AllAwardInfo(AwardInfo::NEWYEAR_GIFT, rmb, delta);
		if (!(award == NULL || award->size() == 0)) {
			for (int i = 0; i < (int)award->size(); ++i) {
				PlayerEntity_AddMail(component->player, ItemInfo::GOODS, (*award)[i]->award(), 1, Config_Words(0), Config_Words(86));
			}
		}
		PlayerEntity_SetDayEvent(component->player, 68, rmb + delta);
	}
}*/

void Event_AwardFromRecharge(struct Item *item, int64_t delta) {
	if (!Item_IsValid(item))
		return;
	if(Config_InActiveTime(24)) {
		struct Component *component = Item_Component(item);
		if(component == NULL)
			return;
		int rmb = component->playerAtt->dayEvent(71);
		const vector<const AwardInfo *> * award = AwardInfoManager_AllAwardInfo(AwardInfo::AWARD_FROM_RECHARGE, rmb, delta);
		if (!(award == NULL || award->size() == 0)) {
			for (int i = 0; i < (int)award->size(); ++i) {
				PlayerEntity_AddMail(component->player, ItemInfo::GOODS, (*award)[i]->award(), 1, (*award)[i]->name().c_str(), (*award)[i]->content().c_str());
			}
		}
		PlayerEntity_SetDayEvent(component->player, 71, rmb + delta);
	}
}

void Event_AwardFromSpend(struct Item *item, int64_t delta) {
	if (!Item_IsValid(item))
		return;
	if(Config_InActiveTime(25)) {
		struct Component *component = Item_Component(item);
		if(component == NULL)
			return;
		int rmb = component->playerAtt->dayEvent(72);
		const vector<const AwardInfo *> * award = AwardInfoManager_AllAwardInfo(AwardInfo::AWARD_FROM_SPEND, rmb, delta);
		if (!(award == NULL || award->size() == 0)) {
			for (int i = 0; i < (int)award->size(); ++i) {
				PlayerEntity_AddMail(component->player, ItemInfo::GOODS, (*award)[i]->award(), 1, (*award)[i]->name().c_str(), (*award)[i]->content().c_str());
			}
		}
		PlayerEntity_SetDayEvent(component->player, 72, rmb + delta);
	}
}

void Event_AwardStrongWeapon(struct PlayerEntity* entity) {
	if (entity == NULL)
		return;

	if(Config_InActiveTime(22)) {
		const AwardInfo * award = AwardInfoManager_AwardInfo(AwardInfo::TOP_WEAPON, 0);
		if (award != NULL ) {
			PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
		}
	}
}

void Event_AwardConsume(struct Item* item, int delta) {
	if (!Item_IsValid(item))
		return;

	struct Component *component = Item_Component(item);
	if(component == NULL)
		return;

	int64_t roleID = PlayerEntity_RoleID(component->player);
	if (PlayerPool_IsInRekooRole(roleID)) {
		return;
	}
	
	if(Config_InActiveTime(23)) {
		const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
		if (fixedMap == NULL) {
			return;
		}

		multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-23);
		if (begin != fixedMap->end()) {
			int endTime = component->playerAtt->fixedEvent(93);
			int rmb = component->playerAtt->fixedEvent(92);
			int activeTime = begin->second & 0xFFFFFFFF;
			if (endTime == activeTime) {
				PlayerEntity_SetFixedEvent(component->player, 92, rmb + delta);
			}else {
				PlayerEntity_SetFixedEvent(component->player, 92, delta);
				PlayerEntity_SetFixedEvent(component->player, 93, activeTime);
			}

			rmb = component->playerAtt->fixedEvent(92);
			if (rmb >= 1000) {
				PlayerPool_RankConsumeAdjust(roleID, rmb);
			}
		}
	}else {
		PlayerEntity_SetFixedEvent(component->player, 92, 0);
		PlayerEntity_SetFixedEvent(component->player, 93, 0);
	}
}

bool Event_ForbidMessage(int group, int unit) {
	static char str[64];
	SNPRINTF1(str, "%d,%d", group, unit);

	set<string>::iterator iter = forbidMessage.find(str);
	if (iter != forbidMessage.end()) {
		return true;
	}
	return false;
}

//op = -1 load Data
int Event_AddGMData(DCProto_LoadAllDataFromGMDataTable * info, int op) {
	if (info == NULL) {
		return -1;
	}

	char ch[64];
	for (int i = 0; i < info->info_size(); ++i) {
		if (op == -1 || op == 1) {
			if (info->info(i).key() == -3 || info->info(i).key() == -4 || info->info(i).key() == -5) {
				continue;
			}

			listGMData.push_back(info->info(i));
			if (info->info(i).key() == -1) {
				memset(ch, 0, sizeof(ch) / sizeof(char));
				SNPRINTF1(ch, "%d,%d", info->info(i).arg1(), info->info(i).arg2());
				forbidMessage.insert(ch);
			}
		}else if (op == 2) {
			for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end();) {
				if (
						itor->key() == info->info(i).key() && itor->arg1() == info->info(i).arg1() &&
						itor->arg2() == info->info(i).arg2() && itor->arg3() == info->info(i).arg3() &&
						itor->str1() == info->info(i).str1() && itor->str2() == info->info(i).str2() &&
						itor->str3() == info->info(i).str3() && itor->roleID() == info->info(i).roleID()
				   ) {
					if (info->info(i).key() == -1) {
						memset(ch, 0, sizeof(ch) / sizeof(char));
						SNPRINTF1(ch, "%d,%d", info->info(i).arg1(), info->info(i).arg2());
						forbidMessage.erase(ch);
					}
					listGMData.erase(itor++);
					break;
				}
				++itor;
			}
		}else if ( op == 3 ) {
			if (info->info(i).key() == -3 || info->info(i).key() == -4 || info->info(i).key() == -5) {
				continue;
			}

			listGMData.push_back(info->info(i));
			return 0;
		}
	}

	if (op == 1 || op == 2) {
		DCProto_SaveGMDataTable save;
		for (int i = 0; i < info->info_size(); ++i) {
			*save.add_info() = info->info(i);
		}
		save.set_op(op);
		GCAgent_SendProtoToDCAgent(&save);
	}

	return 0;
}

bool Event_IsGM(NetProto_GMLogin * info) {
	if (info == NULL)
		return false;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(info->account()) == 0 && itor->str2().compare(info->passwd()) == 0) {
			info->set_permission(::NetProto_GMLogin_OP(itor->arg1()));
			return true;
		}
	}
	return false;
}

int Event_GMRegister(NetProto_GMRegister * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(info->account()) == 0) {
			return -2;
		}

		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() != 1) {
				return -3;
			}
		}
	}

	DCProto_LoadAllDataFromGMDataTable message;
	DCProto_GMInfo *gmInfo = message.add_info();
	gmInfo->set_key(-2);
	gmInfo->set_arg1((int)info->permission());
	gmInfo->set_str1(info->account());
	gmInfo->set_str2(info->password());

	Event_AddGMData(&message, 1);
	return 0;

}

int Event_GMShutDownMessage(NetProto_GMShutDownMessage * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() == (int)NetProto_GMLogin::CP || itor->arg1() == NetProto_GMLogin::YUNYING) {
				break;
			}else {
				return -2;
			}
		}
	}

	DCProto_LoadAllDataFromGMDataTable message;
	DCProto_GMInfo *gmInfo = message.add_info();
	gmInfo->set_key(-1);
	gmInfo->set_arg1(info->groupID());
	gmInfo->set_arg2(info->unitID());
	gmInfo->set_str1(str);

	Event_AddGMData(&message, 1);
	return 0;
}

int Event_GMOpenMessage(NetProto_GMOpenMessage * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() == (int)NetProto_GMLogin::CP || itor->arg1() == NetProto_GMLogin::YUNYING) {
				break;
			}else {
				return -2;
			}
		}
	}

	DCProto_LoadAllDataFromGMDataTable message;
	DCProto_GMInfo *gmInfo = message.add_info();
	gmInfo->set_key(-1);
	gmInfo->set_arg1(info->groupID());
	gmInfo->set_arg2(info->unitID());
	gmInfo->set_str1(str);

	Event_AddGMData(&message, 2);
	return 0;
}

int Event_GMModifyVIP(NetProto_GMModifyVIP * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() == (int)NetProto_GMLogin::CP || itor->arg1() == NetProto_GMLogin::YUNYING) {
				break;
			}else {
				return -2;
			}
		}
	}

	if (info->delta() < 0) {
		info->set_delta(0);
	}

	while (VIPInfoManager_VIPInfo(info->delta()) == NULL) {
		info->set_delta(info->delta() - 1);
	}

	bool flag = true;
	PlayerEntity* entity = PlayerEntity_PlayerByRoleID(info->roleID());
	if (entity != NULL) {
		struct Component * component = PlayerEntity_Component(entity);
		if (component != NULL) {
			Item_SetVIP(component->item, info->delta());
			flag = false;
		}
	}

	if (flag) {
		DCProto_SetVIP setVIP;
		*setVIP.mutable_info() = *info;
		GCAgent_SendProtoToDCAgent(&setVIP);
	}


	DCProto_LoadAllDataFromGMDataTable message;
	DCProto_GMInfo *gmInfo = message.add_info();
	gmInfo->set_key(-3);
	gmInfo->set_arg1(info->delta());
	gmInfo->set_str1(str);
	gmInfo->set_roleID(info->roleID());

	Event_AddGMData(&message, 1);

	return 0;
}

int Event_GMAddExchange(NetProto_GMAddExchange * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() == (int)NetProto_GMLogin::CP || itor->arg1() == NetProto_GMLogin::YUNYING) {
				break;
			}else {
				return -2;
			}
		}
	}

	DCProto_GMAddExchange dc;
	*dc.mutable_info() = *info;
	GCAgent_SendProtoToDCAgent(&dc);

	DCProto_LoadAllDataFromGMDataTable message;
	DCProto_GMInfo *gmInfo = message.add_info();
	gmInfo->set_key(-4);
	gmInfo->set_arg1(info->group());
	gmInfo->set_arg2(info->giftID());
	gmInfo->set_arg3(info->endTime());
	gmInfo->set_str1(str);

	Event_AddGMData(&message, 1);
	return 0;
}

bool Event_GMPermission(const char * str) {
	if (str == NULL)
		return false;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() == (int)NetProto_GMLogin::CP || itor->arg1() == NetProto_GMLogin::YUNYING) {
				return true;
			}else {
				return false;
			}
		}
	}

	return false;
}

int Event_GMQueryFaction(NetProto_GMQueryFaction * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	FactionPool_GMQueryFaction(info);
	return 0;
}

int Event_GMChangeFactionMem(NetProto_GMChangeFactionMem * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() == (int)NetProto_GMLogin::CP || itor->arg1() == NetProto_GMLogin::YUNYING) {
				break;
			}else {
				return -2;
			}
		}
	}

	FactionPool_GMChangeFactionMem(info);

	DCProto_LoadAllDataFromGMDataTable message;
	DCProto_GMInfo *gmInfo = message.add_info();
	gmInfo->set_key(-5);
	gmInfo->set_arg1(info->flag());
	gmInfo->set_str1(str);
	gmInfo->set_str2(info->factionName());
	gmInfo->set_roleID(info->roleID());

	Event_AddGMData(&message, 1);

	return 0;
}

int Event_GMAddRekooRole(NetProto_GMAddRekooRole * info, const char * str) {
	if (info == NULL || str == NULL)
		return -1;

	for (list<DCProto_GMInfo>::iterator itor = listGMData.begin(); itor != listGMData.end(); ++itor) {
		if (itor->key() == -2 && itor->str1().compare(str) == 0) {
			if (itor->arg1() == (int)NetProto_GMLogin::CP || itor->arg1() == NetProto_GMLogin::YUNYING) {
				break;
			}else {
				return -2;
			}
		}
	}

	static DCProto_GMRekooRole dc;
	dc.Clear();
	*dc.mutable_info() = *info;
	dc.set_str(str);
	GCAgent_SendProtoToDCAgent(&dc);

	PlayerPool_AddRekooRole(info->roleID());

	return 0;
}
