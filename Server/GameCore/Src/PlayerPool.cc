#include "PlayerPool.hpp"
#include "Config.hpp"
#include "PlayerEntity.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "HashArray.hpp"
#include "GCAgent.hpp"
#include "DataManager.hpp"
#include "EquipmentInfoManager.hpp"
#include "Item.hpp"
#include "GlobalMissionManager.hpp"
#include "PlayOffManager.hpp"
#include "DCProto.pb.h"
#include "FactionInfo.pb.h"
#include "Time.hpp"
#include "Timer.hpp"
#include "Event.hpp"
#include "ConfigUtil.hpp"
#include "MathUtil.hpp"
#include "AwardInfoManager.hpp"
#include "Reservation.pb.h"
#include "FactionPool.hpp"
#include "AccountPool.hpp"
#include "ScribeClient.hpp"
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <list>
#include <ctime>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>

using namespace std;

#define SAVE_STEP 40
#define UPDATE_INTERVAL 5
#define UPDATE_INTERVAL_RANK 1
#define LOGIN_LATER_STEP 30

struct Reservation {
	int64_t roleID1;
	int64_t roleID2;
	string name1;
	string name2;
	int rmb;
	PB_ProfessionInfo::Type type1;
	PB_ProfessionInfo::Type type2;
	int powerType;
	set<int64_t> setRole1;
	set<int64_t> setRole2;
	Reservation() : roleID1(-100), roleID2(-100), name1(""), name2(""), rmb(0), type1(PB_ProfessionInfo::NPC), type2(PB_ProfessionInfo::NPC), powerType(0) {setRole1.clear(); setRole2.clear();}
};


static struct{
	int maxMaps;
	vector<vector<set<struct PlayerEntity *> > > ids; // map.line.players

	int64_t cur;
	time_t prevTime;
	int64_t prevRank;

	time_t loginLaterTime;

	vector<PB_PlayerAtt> free;

	map<int64_t, int64_t> restriction;
	
	vector<RecordInfo> rankPower;
	map<int64_t, int64_t> rankPowerInfo;
	
	vector<RecordInfo> rankTower;
	map<int64_t, int64_t> rankTowerInfo;
	
	vector<RecordInfo> rankLevel;
	map<int64_t, int64_t> rankLevelInfo;

	vector<RecordInfo> rankWorldBoss;
	map<int64_t, int64_t> rankWorldBossInfo;

	RecordInfo rankWorldBossFinal;

	vector<RecordInfo> rankWBCur;
	
	vector<RecordInfo> rankGod;
	map<int64_t, int64_t> rankGodInfo;

	map<int64_t, int64_t> rankGodCur;
	int32_t rankGodRecordsTime;

	vector<RecordInfo> rankBlessingCome;
	map<int64_t, int64_t> rankBlessingComeInfo;

	map<string, int64_t> factionFight;
	int curSave;
	string winFactionName;
	int64_t winFactionHurt;
	int winSave;
	
	vector<RecordInfo> rankFactionInfo;
	bool isRankFaction;
	
	vector<RecordInfo> rankPetInfo;

	vector<RecordInfo> rankDevil;
	map<int64_t, RecordInfo> rankDevilInfo;
	vector<RecordInfo> vecDevilRank;
	bool isRankDevil;

	vector<RecordInfo> rankLuck;
	map<int64_t, int64_t> rankLuckInfo;
	
	vector<RecordInfo> rankPurchase;
	map<int64_t, int64_t> rankPurchaseInfo;
	
	vector<RecordInfo> rankConsume;

	set<int64_t> rekooRole;

	map<int64_t, pair<string, string> > mapInviteCode;
	map<string, int64_t> mapCodeInfo;

	int32_t maxRoleCount;
	string maxRoleCountInfo;
	
	int giftCount;
	int maxGiftInGiftCount;
	set<int64_t> giftRoleID;

	int catGiftCount;
	int catGiftEndTime;

	int groupPurchase;
	int groupPurchaseEndTime;

	Reservation reservation[20];
	int reservationMap;
	int refreshIndex;
	PB_AllReservationTimes all;
}pool;


void PlayerPool_Init() {
	pool.maxMaps = Config_MaxMaps();
	pool.ids.resize(pool.maxMaps);
	for (size_t i = 0; i < pool.ids.size(); i++)
		pool.ids[i].resize(1);

	pool.free.clear();

	pool.prevTime = Time_TimeStamp();
	pool.loginLaterTime = 0;

	pool.prevRank = 0;

	pool.rankGodRecordsTime = 0;

	pool.rankWorldBossFinal.mutable_role()->set_roleID(-1);

	pool.restriction.clear();
	pool.rankWBCur.clear();
	pool.factionFight.clear();
	
	pool.curSave = 0;
	pool.winFactionName = "";
	pool.winFactionHurt = 0;
	pool.winSave = 0;
	
	pool.rankFactionInfo.clear();
	pool.isRankFaction = false;
	pool.rankPetInfo.clear();

	pool.rankDevil.clear();
	pool.rankDevilInfo.clear();
	pool.isRankDevil = false;
	pool.vecDevilRank.clear();
	
	pool.rankLuck.clear();
	pool.rankLuckInfo.clear();
	
	pool.rankConsume.clear();
	pool.rekooRole.clear();
	
	pool.mapInviteCode.clear();
	pool.mapCodeInfo.clear();
	
	pool.maxRoleCount = 0;
	pool.maxRoleCountInfo = "";
	
	pool.giftCount = 0;
	pool.maxGiftInGiftCount = 0;
	pool.giftRoleID.clear();

	pool.catGiftCount = 0;
	pool.catGiftEndTime = 0;
	
	pool.groupPurchase = 0;
	pool.groupPurchaseEndTime = 0;
			
	pool.rankPurchase.clear();
	pool.rankPurchaseInfo.clear();
	
	pool.reservationMap = -1;
	pool.refreshIndex = 0;
	
	string src = Config_DataPath() + string("/Fight.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!pool.all.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
}

static void NoticePlayOff(void *arg) {
	int playoff = *((int *)arg);
	// delete (int*)arg;

	const PlayOff *info = PlayOffManager_PlayOff(playoff);
	if (info == NULL)
		return;

	int day, pass, turn, overTime;
	if (PlayOffManager_CurData(playoff, &day, &pass, &turn, &overTime) == 0) {
		static vector<struct PlayerEntity *> allPlayers;
		allPlayers.clear();
		int allCount = PlayerPool_Players(&allPlayers);
		for (int k = 0; k < allCount; k++) {
			struct Component *component = PlayerEntity_Component(allPlayers[k]);
			if (component == NULL)
				continue;
			int right = PlayerEntity_CanEnterPlayOff(component->player, playoff, day, pass);
			if (right == 0 || right == -2) {
				int32_t id = PlayerEntity_ID(allPlayers[k]);
				static NetProto_PlayOffInfo proto;
				proto.Clear();
				if (right == 0)
					proto.set_res(0);
				else if (right == -2)
					proto.set_res(-3);
				proto.set_id(playoff);
				proto.set_day(day);
				proto.set_pass(pass);
				proto.set_overTime(overTime);
				proto.set_turn(turn);
				proto.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(-1);
				int64_t roleID = component->baseAtt->roleID();
				struct PlayOffUnit *unit = MapPool_SelectPlayOff(roleID);
				if (unit != NULL) {
					if (roleID == unit->lhs)
						proto.set_result((unit->lWin << 16) | unit->rWin);
					else if (roleID == unit->rhs)
						proto.set_result((unit->rWin << 16) | unit->lWin);

					const PB_PlayerAtt *another = NULL;
					if (unit->lhs == roleID)
						another = &unit->rAtt;
					else if (unit->rhs == roleID)
						another = &unit->lAtt;
					if (PlayOffManager_Count(info->over(day)) > 1) {
						if (another != NULL)
							*proto.mutable_att() = *another;
					}
				}
				GCAgent_SendProtoToClients(&id, 1, &proto);

				if (right == 0) {
					static NetProto_Message proto;
					proto.set_content(Config_Words(61));
					proto.set_count(2);
					GCAgent_SendProtoToClients(&id, 1, &proto);
				}
			}
		}
	}
}

static void UpdatePlayer(void *arg) {
	static vector<struct PlayerEntity *> allPlayers;
	allPlayers.clear();
	int allCount = PlayerPool_Players(&allPlayers);
	for (int k = 0; k < allCount; k++) {
		struct Component *component = PlayerEntity_Component(allPlayers[k]);
		Component_Update(component);
	}
}


static int ConfigTime(int id, int index) {
	if (id < 0 || id > 19) 
		return -1;

	if (index < 0 || index > 11) 
		return -2;

	PB_ReservationTime times = pool.all.times(id);
	
	switch (index) {
		case 0:
			return times.appointmentBeginHour();
		case 1:
			return times.appointmentBeginMinute();
		case 2:
			return times.appointmentEndHour();
		case 3:
			return times.appointmentEndMinute();
		case 4:
			return times.challengeEndHour();
		case 5:
			return times.challengeEndMinute();
		case 6:
			return times.enterBeginHour();
		case 7:
			return times.enterBeginMinute();
		case 8:
			return times.fightBeginHour();
		case 9:
			return times.fightBeginMinute();
		case 10:
			return times.fightEndHour();
		case 11:
			return times.fightEndMinute();
		default:
			return -3;
	}
	return -4;
}

static void RefreshReservation(void *arg) {
//TODO:
	int hour = 0;
	int min = 0;
	//int index = -1;
	Config_Reservation(Time_TimeStamp(), NULL, NULL, NULL, &hour, &min, NULL);

	if (hour == 0 && min == 0) {
		PlayerPool_ReservationRefresh(-1, 0);
		pool.refreshIndex = 1;
	}

//	int hour = 0;
//	int min = 0;
	hour = ConfigTime(0, 2);
	min = ConfigTime(0, 3);
	if (hour == -1 || min == -1)
		return;

	int begin = Config_ReservationTime(hour, min);
	
	if (Time_TimeStamp() - begin >= 0 && Time_TimeStamp() - begin < 5) {
		if (!(pool.refreshIndex & 2)) {
			PlayerPool_ReservationRefresh(-2, 0);
			pool.refreshIndex = pool.refreshIndex | 2;
		}
	}

	hour = ConfigTime(8, 2);
	min = ConfigTime(8, 3);
	if (hour == -1 || min == -1)
		return;
	
	begin = Config_ReservationTime(hour, min);

	if (Time_TimeStamp() - begin >= 0 && Time_TimeStamp() - begin < 5) {
		if (!(pool.refreshIndex & 4)) {
			PlayerPool_ReservationRefresh(-3, 0);
			pool.refreshIndex = pool.refreshIndex | 4;
		}
	}
	

	for (int index = 0; index < 20; ++index) {
		hour = ConfigTime(index, 10);
		min = ConfigTime(index, 11);
		if (hour == -1 || min == -1)
			return;

		begin = Config_ReservationTime(hour, min);
		
		if (Time_TimeStamp() - begin >= 0 && Time_TimeStamp() - begin < 5) {
			if (!(pool.refreshIndex & (1 << (index + 3)))) {
				PlayerPool_ReservationRefresh(index, 0);
				pool.refreshIndex = pool.refreshIndex | (1 << (index + 3));
			}
		}
	
	}

	/*	
	if (hour == 12 || hour == 13 || hour == 19 || hour == 20 || hour == 21) {
		if ( (min % 5) == 0 ) {
			if (hour == 12)
				index = 0;
			if (hour == 13)
				index = 4;
			if (hour == 19)
				index = 8;
			if (hour == 20)
				index = 12;
			if (hour == 21)
				index = 16;

			index += min / 15;
			if (min == 10 || min == 25 || min == 40 || min == 55) {
				if (!(pool.refreshIndex & (1 << (index + 3)))) {
					PlayerPool_ReservationRefresh(index, 0);
					pool.refreshIndex = pool.refreshIndex | (1 << (index + 3));
				}
			}
		}
	}
	*/
}

static void UpdateEvent(void *arg) {
	time_t cur = Time_TimeStamp();

	static vector<struct PlayerEntity *> players;
	players.clear();
	PlayerPool_Players(&players);

	int dayDelta = Time_DayDelta(pool.prevTime, cur);
	if (dayDelta > 0) {
		bool isAnotherWeek = Time_WeekDelta(pool.prevTime, cur) > 0;
		DEBUG_LOGRECORD("anotherDay: 1, anotherWeek: %d", (int)isAnotherWeek);

		static NetProto_ResetEvent resetEvent;
		resetEvent.set_monday(isAnotherWeek);

		for (vector<struct PlayerEntity *>::iterator it = players.begin(); it != players.end(); it++) {
			PlayerEntity_ResetDayEvent(*it, 1, isAnotherWeek ? 1 : 0);
			GlobalMissionManager_ApplyDailyMission(PlayerEntity_Component(*it)->movement);

			struct Component *component = PlayerEntity_Component(*it);
			resetEvent.set_resetCome(component->playerAtt->fixedEvent(1) == 0);
			int32_t id = PlayerEntity_ID(*it);
			GCAgent_SendProtoToClients(&id, 1, &resetEvent);
		}
	}

	if (Time_PassHour(pool.prevTime, cur, Config_HellBegin() + Config_HellTime() / (1000 * 3600))) {
		for (vector<struct PlayerEntity *>::iterator it = players.begin(); it != players.end(); it++) {
			PlayerEntity_SetFixedEvent(*it, 18, 0, true);
		}
	}
	const vector<MailGift> *mailGift = Config_MailGift(MailGift::EVERY_DAY);
	if (mailGift != NULL) {
		for (size_t i = 0; i < mailGift->size(); i++) {
			const MailGift *unit = &(*mailGift)[i];
			bool ok = false;
			if (unit->arg1() == 0 && unit->arg2() == 0 && unit->arg3() == 0) {
				ok = Time_PassHour(pool.prevTime, cur, unit->arg4());
			} else {
				tm curTM;
				Time_LocalTime(cur, &curTM);
				if (curTM.tm_year + 1900 == unit->arg1()
						&& curTM.tm_mon + 1 == unit->arg2()
						&& curTM.tm_mday == unit->arg3())
					ok = Time_PassHour(pool.prevTime, cur, unit->arg4());
			}
			if (ok) {
				for (vector<struct PlayerEntity *>::iterator it = players.begin(); it != players.end(); it++) {
					struct Component *component = PlayerEntity_Component(*it);
					if (component == NULL)
						continue;
					static PB_MailInfo mail;
					mail = unit->mail();
					if (mail.item().type() == PB_ItemInfo::EQUIPMENT) {
						const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(mail.item().id());
						assert(equip != NULL);
						int32_t newID = Item_AddEquip(component->item, equip);
						if (newID == -1)
							continue;
						mail.mutable_item()->set_id(newID);
						mail.mutable_item()->set_count(1);
					}
					int32_t pos = PlayerEntity_AddMail(*it, &mail);
					if (pos != -1) {
						static NetProto_SendMail sm;
						sm.set_receiver(PlayerEntity_RoleID(*it));
						*sm.mutable_mail() = mail;
						sm.set_pos(pos);
						int32_t id = PlayerEntity_ID(*it);
						GCAgent_SendProtoToClients(&id, 1, &sm);
					}
				}
			}
		}
	}

	pool.prevTime = cur;
}

static vector<RecordInfo>* CurRank(NetProto_Rank::Type type) {
	vector<RecordInfo>* cur = NULL;

	if (type == NetProto_Rank::POWER)
		cur = &pool.rankPower;
	else if (type == NetProto_Rank::TOWER)
		cur = &pool.rankTower;
	else if (type == NetProto_Rank::LEVEL)
		cur = &pool.rankLevel;
	else if (type == NetProto_Rank::WORLD_BOSS)
		cur = &pool.rankWorldBoss;
	else if (type == NetProto_Rank::GOD)
		cur = &pool.rankGod;
	else if (type == NetProto_Rank::BLESSCOME)
		cur = &pool.rankBlessingCome;
	else if (type == NetProto_Rank::FACTION)
		cur = &pool.rankFactionInfo;
	else if (type == NetProto_Rank::PET)
		cur = &pool.rankPetInfo;
	else if (type == NetProto_Rank::DEVIL)
		cur = &pool.vecDevilRank;
	else if (type == NetProto_Rank::LUCK)
		cur = &pool.rankLuck;
	else if (type == NetProto_Rank::CONSUME)
		cur = &pool.rankConsume;
	else if (type == NetProto_Rank::GROUP_PURCHASE)
		cur = &pool.rankPurchase;

	return cur;
}

static void RankTop3(NetProto_Rank::Type type, int64_t *rank) {
	vector<RecordInfo>* cur = CurRank(type);
	if (cur == NULL)
		return;

	int count = 0;
	for (vector<RecordInfo>::iterator itor = cur->begin(); itor != cur->end() && count < 3; ++itor, ++count) {
		*(rank + count) = itor->role().roleID();
	}
}

static const char * EnterRankStr(NetProto_Rank::Type type) {
	if (type == NetProto_Rank::POWER) {
		return Config_Words(15);
	} else if (type == NetProto_Rank::TOWER) {
		return Config_Words(16);
	} else if (type == NetProto_Rank::LEVEL) {
		return Config_Words(11);
	} else if (type == NetProto_Rank::GOD) {
		return Config_Words(30);
	} else {
		return Config_Words(11);
	}
}

static void NoticeRank(struct PlayerEntity *entity, NetProto_Rank::Type type, int level) {
	// playofftest
	// return;

	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL)
		return;

	static NetProto_Message message;
	message.Clear();
	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, EnterRankStr(type), component->baseAtt->name(), Config_RankStr(type), level);
	message.set_content(buf);
	message.set_count(1);
	GCAgent_SendProtoToAllClients(&message);
}

static void GMRoleOnlineCount(void *arg) {
	PlayerPool_GMRoleOnlineCount();
}


static void FirstDayReLoadRank(void *arg) {
	if (Config_InActiveTime(8)) {
		DCProto_InitRank rankBless;
		rankBless.set_type(NetProto_Rank::BLESSCOME);
		GCAgent_SendProtoToDCAgent(&rankBless, false);
	}

	if (Config_InActiveTime(20)) {
		DCProto_InitRank rankLuck;
		rankLuck.set_type(NetProto_Rank::LUCK);
		GCAgent_SendProtoToDCAgent(&rankLuck, false);
	}

	if (!Time_TheSameDay(Time_TimeStamp(), Config_OpenTime(NULL))) {
		return;
	}

	static int firstDayReLoadRankIndex = 0;
	if (firstDayReLoadRankIndex % 3 == 0) {
		DCProto_InitRank rankPower;
		rankPower.set_type(NetProto_Rank::POWER);
		GCAgent_SendProtoToDCAgent(&rankPower, false);
		//	GCAgent_WaitDCAgent(1);
	}else if (firstDayReLoadRankIndex % 3 == 1) {
		DCProto_InitRank rankTower;
		rankTower.set_type(NetProto_Rank::TOWER);
		GCAgent_SendProtoToDCAgent(&rankTower, false);
	//	GCAgent_WaitDCAgent(1);
	}else if (firstDayReLoadRankIndex % 3 == 2) {
		DCProto_InitRank rankLevel;
		rankLevel.set_type(NetProto_Rank::LEVEL);
		GCAgent_SendProtoToDCAgent(&rankLevel, false);
	//	GCAgent_WaitDCAgent(1);
	}else {
		return;
	}
	
	firstDayReLoadRankIndex++;
}


static void ReLoadRank(void *arg) {
	if (Config_InActiveTime(29)) {
		DCProto_InitRank rankPurchase;
		rankPurchase.set_type(NetProto_Rank::GROUP_PURCHASE);
		GCAgent_SendProtoToDCAgent(&rankPurchase, false);
	}

	static int index = 0;
	static int count = 6; 
	if ( (index % count == 0) ) {
		DCProto_InitRank rankPower;
		rankPower.set_type(NetProto_Rank::POWER);
		GCAgent_SendProtoToDCAgent(&rankPower, false);
	}else if ( (index % count) == 1 ) {
		DCProto_InitRank rankTower;
		rankTower.set_type(NetProto_Rank::TOWER);
		GCAgent_SendProtoToDCAgent(&rankTower, false);
	}else if ( (index % count) == 2) {
		DCProto_InitRank rankLevel;
		rankLevel.set_type(NetProto_Rank::LEVEL);
		GCAgent_SendProtoToDCAgent(&rankLevel, false);
	} else if ( (index % count) == 3) {
		DCProto_InitRank rankFaction;
		rankFaction.set_type(NetProto_Rank::FACTION);
		GCAgent_SendProtoToDCAgent(&rankFaction, false);
	
		DCProto_InitRank rankPet;
		rankPet.set_type(NetProto_Rank::PET);
		GCAgent_SendProtoToDCAgent(&rankPet, false);
		
		if (Config_InActiveTime(23)) {
			DCProto_InitRank rankConsume;
			rankConsume.set_type(NetProto_Rank::CONSUME);
			GCAgent_SendProtoToDCAgent(&rankConsume, false);
		}
	} else if ( (index % count) == 4) {
		if (Config_InActiveTime(13)) {
			DCProto_InitRank rankDevil;
			rankDevil.set_type(NetProto_Rank::DEVIL);
			GCAgent_SendProtoToDCAgent(&rankDevil, false);
		}
	}else if ( (index % count) == 5 ) {
		if (Config_InActiveTime(20)) {
			DCProto_InitRank rankLuck;
			rankLuck.set_type(NetProto_Rank::LUCK);
			GCAgent_SendProtoToDCAgent(&rankLuck, false);
		}
	}
	index++;
}

static void SendWeekGodAwardCP(void *arg) {
	bool v = *(bool *)arg;
	// delete (bool *)arg;
	PlayerPool_SendWeekGodAward(v);
}

static void SendGodAwardCP(void *arg) {
	bool v = *(bool *)arg;
	// delete (bool *)arg;
	PlayerPool_SendGodAward(v);
}

static void UpdateFree(void *arg) {
	DCProto_RandomRoles randomRoles;
	randomRoles.set_count(Config_MaxPlayersOfLineInPeace() * 5);
	GCAgent_SendProtoToDCAgent(&randomRoles, false);
}

static void GenSnapshot(void *arg) {
	DataManager_GenSnapshot();
}

static int WBCurCompare(const void *lhs_, const void *rhs_) {
	const RecordInfo *lhs = (const RecordInfo *)lhs_;
	const RecordInfo *rhs = (const RecordInfo *)rhs_;
	return (int)(lhs->arg1() - rhs->arg1());
}

static void UpdateWBCur(void *arg) {
	int32_t curMap = MapPool_WorldBoss();
	if (curMap >= 0) {
			static vector<struct PlayerEntity *> players;
			players.clear();
			int count = PlayerPool_Players(curMap, &players);
			for (int i = 0; i < count; i++) {
				struct Component *component = PlayerEntity_Component(players[i]);
				if (component == NULL)
					continue;
				if (component->roleAtt->fightAtt().worldBossNum() != MapPool_WorldBossNum())
					continue;
				if (component->roleAtt->fightAtt().worldBossHurt() <= 0)
					continue;

				bool done = false;
				for (size_t j = 0; j < pool.rankWBCur.size(); j++) {
					RecordInfo *record = &pool.rankWBCur[j];
					if (record->role().roleID() != component->baseAtt->roleID())
						continue;
					record->set_arg1(component->roleAtt->fightAtt().worldBossHurt());
					done = true;
					break;
				}
				if (!done) {
					RecordInfo record;
					record.set_arg1(component->roleAtt->fightAtt().worldBossHurt());
					record.mutable_role()->set_roleID(component->baseAtt->roleID());
					record.mutable_role()->set_name(component->baseAtt->name());
					record.mutable_role()->set_professionType((PB_ProfessionInfo::Type)component->baseAtt->professionType());
					pool.rankWBCur.push_back(record);
				}
			}

			if (!pool.rankWBCur.empty()) {
				qsort(&pool.rankWBCur[0], pool.rankWBCur.size(), sizeof(pool.rankWBCur[0]), WBCurCompare);
			}

			static NetProto_WorldBossCurRank proto;
			proto.Clear();
			for (size_t i = 0; i < 5; i++) {
				if (i >= pool.rankWBCur.size())
					break;
				*proto.add_rank() = pool.rankWBCur[pool.rankWBCur.size() - i - 1];
			}

			proto.set_selfRank(-1);
			for (int i = 0; i < count; i++) {
				struct Component *component = PlayerEntity_Component(players[i]);
				if (component == NULL)
					continue;
				if (component->roleAtt->fightAtt().worldBossNum() != MapPool_WorldBossNum()
						|| component->roleAtt->fightAtt().worldBossHurt() <= 0) {
					int32_t id = PlayerEntity_ID(component->player);
					GCAgent_SendProtoToClients(&id, 1, &proto);
				}
			}
			for (size_t i = 0; i < pool.rankWBCur.size(); i++) {
				const RecordInfo *record = &pool.rankWBCur[i];
				struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(record->role().roleID());
				if (entity == NULL)
					continue;
				struct Component *component = PlayerEntity_Component(entity);
				if (component == NULL)
					continue;
				if (component->roleAtt->movementAtt().mapID() != curMap)
					continue;

				int rank = (int)(pool.rankWBCur.size() - i - 1);
				proto.set_selfRank(rank);
				*proto.mutable_self() = *record;

				int32_t id = PlayerEntity_ID(entity);
				GCAgent_SendProtoToClients(&id, 1, &proto);
			}
	} else {
		pool.rankWBCur.clear();
	}
}


static void UpdateFWCur(void *arg) {
	int32_t curMap = MapPool_FactionWar();
	if (curMap < 0)
		return;

	const map<string, int64_t> *table = PlayerPool_GetFactionFightInfo();
	if (table->empty())
		return;

	static multimap<int64_t, string> rank;
	rank.clear();
	for (map<string, int64_t>::const_iterator it = table->begin(); it != table->end(); it++)
		rank.insert(make_pair(it->second, it->first));

	static map<string, int> factionRank;
	factionRank.clear();
	int i = 0;
	for (multimap<int64_t, string>::reverse_iterator it = rank.rbegin(); it != rank.rend(); it++)
		factionRank[it->second] = i++;

	static NetProto_FactionWarCurRank proto;
	proto.Clear();
	i = 0;
	for (multimap<int64_t, string>::reverse_iterator it = rank.rbegin(); it != rank.rend(); it++) {
		if (i++ >= 5)
			break;
		RecordInfo *record = proto.add_rank();
		record->set_arg1(it->first);
		record->mutable_role()->set_name(it->second);
	}

	static vector<struct PlayerEntity *> players;
	players.clear();
	int count = PlayerPool_Players(curMap, &players);
	for (int i = 0; i < count; i++) {
		struct Component *component = PlayerEntity_Component(players[i]);
		if (component == NULL)
			continue;

		map<string, int64_t>::const_iterator it = table->find(component->playerAtt->faction());
		if (it == table->end()) {
			proto.set_selfRank(-1);
			RecordInfo *record = proto.mutable_self();
			record->set_arg1(0);
			record->mutable_role()->set_name(component->playerAtt->faction());
		} else {
			proto.set_selfRank(factionRank[it->first]);
			RecordInfo *record = proto.mutable_self();
			record->set_arg1(it->second);
			record->mutable_role()->set_name(it->first);
		}

		int32_t id = PlayerEntity_ID(component->player);
		GCAgent_SendProtoToClients(&id, 1, &proto);
	}

	PlayerPool_SaveFactionFightInfo();
}

static void AutoRecoverDurability(void *arg) {
	static vector<struct PlayerEntity *> players;
	players.clear();
	int count = PlayerPool_Players(&players);
	for (int i = 0; i < count; i++) {
		struct Component *component = PlayerEntity_Component(players[i]);
		if (component == NULL)
			continue;
		Item_ModifyDurability(component->item, Config_AutoRecoverDurability(), false);
	}
}


static void UpdateLoginLaterTime(void *arg) {
	if (pool.loginLaterTime > 0)
		pool.loginLaterTime--;
}

int PlayerPool_LoginLaterTime() {
	pool.loginLaterTime += LOGIN_LATER_STEP;
	return pool.loginLaterTime;
}

static void SevenDayRankAward(void *arg) {
	NetProto_Rank::Type type = *((NetProto_Rank::Type *)arg);
	vector<RecordInfo> *container;
	AwardInfo::Type awardType;
	if (type == NetProto_Rank::LEVEL) {
		container = &pool.rankLevel;
		awardType = AwardInfo::LEVEL_SEVEN_DAY;
	} else if (type == NetProto_Rank::POWER) {
		container = &pool.rankPower;
		awardType = AwardInfo::POWER_SEVEN_DAY;
	} else {
		return;
	}

	for (int i = 0; i < (int)container->size() && i < 10; i++) {
		const AwardInfo *award = AwardInfoManager_AwardInfo(awardType, i);
		if (award == NULL)
			continue;
		int64_t role = (*container)[i].role().roleID();
		struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(role);
		if (entity != NULL) {
			PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
		} else {
			static DCProto_SendMail dsm;
			dsm.Clear();
			dsm.set_id(-1);
			dsm.mutable_sm()->set_receiver(role);
			dsm.mutable_sm()->mutable_mail()->set_title(award->name());
			dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
			dsm.mutable_sm()->mutable_mail()->set_content(award->content());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
			GCAgent_SendProtoToDCAgent(&dsm);
		}
	}
}

static void SendRMBToRekooRole(void *arg) {
	for (set<int64_t>::iterator it = pool.rekooRole.begin(); it != pool.rekooRole.end(); ++it) {
		struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(*it);
		if (entity != NULL) {
			struct Component *component = PlayerEntity_Component(entity);
			Item_ModifyRMB(component->item, Config_SendRekooRoleRMB(), true, 0, 0, 0);
		}else {
			DCProto_GMAddRekooRMB dc;
			dc.set_roleID(*it);
			dc.set_rmb(Config_SendRekooRoleRMB());
			GCAgent_SendProtoToDCAgent(&dc);

		}
	}
}
/*
static void Temp(void *) {
	Time_ActiveEndSendAward(23);
	Time_ActiveEndSendAward(3);
	Time_ActiveEndSendAward(13);
	Time_ActiveEndSendAward(16);
	Time_ActiveEndSendAward(20);
}
*/
void PlayerPool_Prepare() {
//	Timer_AddEvent(Time_TimeStamp() + 60 * 5, -1, Temp, NULL);
	Timer_AddEvent(0, 60, Config_Reload, NULL, "Config_Reload");
	Timer_AddEvent(0, 1, UpdateLoginLaterTime, NULL);
	Timer_AddEvent(0, 0, UpdatePlayer, NULL, "UpdatePlayer");
	Timer_AddEvent(0, 0, RefreshReservation, NULL, "RefreshReservation");
	Timer_AddEvent(0, UPDATE_INTERVAL, UpdateEvent, NULL, "UpdateEvent");
	Timer_AddEvent(0, UPDATE_INTERVAL, Config_ActiveTick, NULL, "Config_ActiveTick");
	Timer_AddEvent(0, 60 * 60, FirstDayReLoadRank, NULL, "FirstDayReLoadRank");
	Timer_AddEvent(0, 5 * 60, GMRoleOnlineCount, NULL, "GMRoleOnlineCount");
	int h[] = {1, 2, 3, 4, 5, 6};
	Timer_AddEvent(-1, h, 6, ReLoadRank, NULL, "ReloadRank");
	h[0] = 1;
	Timer_AddEvent(-1, h, 1, SendRMBToRekooRole, NULL, "SendRMBToRekooRole");
	Timer_AddEvent(0, 0, Event_Notice, NULL, "Event_Notice");
	Timer_AddEvent(0, 2, UpdateWBCur, NULL, "UpdateWBCur");
	Timer_AddEvent(0, 2, UpdateFWCur, NULL, "UpdateFWCur");
	static int dayEnd[1] = {0};
	Timer_AddEvent(-1, dayEnd, 1, SendGodAwardCP, new bool(true), "DailyGod");
	Timer_AddEvent(1, dayEnd, 1, SendWeekGodAwardCP, new bool(true), "WeeklyGod");
	for (int i = 0; ; i++) {
		const PlayOff *info = PlayOffManager_PlayOff(i);
		if (info == NULL)
			break;
		int playofftime = Config_PlayOffTime() / 1000;
		int playoffinterval = Config_PlayOffInterval() / 1000;
		int playoffwait = Config_PlayOffWait() / 1000;
		for (int j = 0; j < info->time_size(); j++)
			for (int k = j == 0 ? info->limit() : info->over(j - 1), l = 0; k > (info->over(j) == 1 ? 0 : info->over(j)); k /= 2, l++) {
				int count = PlayOffManager_Count(info->over(j));
				for (int n = 0; n < count; n++)
					Timer_AddEvent((time_t)info->time(j) + (l * count + n) * (playoffinterval + playofftime + playoffwait), -1, NoticePlayOff, new int(i));
			}
	}
	Timer_AddEvent((time_t)Config_ServerStartTime() + 60 * 60, 60 * 60, UpdateFree, NULL);

	{
		time_t cur = Time_TimeStamp();
		tm date;
		Time_LocalTime(cur, &date);
		date.tm_hour = 1;
		date.tm_min = 0;
		date.tm_sec = 0;
		time_t next = mktime(&date);
		if (next < cur)
			next += 3600 * 24;
		Timer_AddEvent(next, 3600 * 24, GenSnapshot, NULL, "GenSnapshot");
	}

	{
		time_t cur = Time_TimeStamp();
		tm date;
		Time_LocalTime(cur, &date);
		if (date.tm_min <= 20) {
			date.tm_min = 0;
		} else if (date.tm_min <= 40) {
			date.tm_min = 20;
		} else {
			date.tm_min = 40;
		}
		date.tm_sec = 0;
		time_t next = mktime(&date) + 60 * 20;
		Timer_AddEvent(next, 60 * 20, AutoRecoverDurability, NULL, "AutoRecoverDurability");
	}

	{
		vector<int> t;
		const multimap<int, int64_t> *table = Config_fixedEventMap();
		multimap<int, int64_t>::const_iterator begin = table->lower_bound(11);
		multimap<int, int64_t>::const_iterator end = table->upper_bound(11);
		if (begin != table->end() && end != table->end()) {
			while (begin != end) {
				t.push_back((int)(begin->second & 0xffffffff));
				begin++;
			}
		}
		begin = table->find(-11);
		if (begin != table->end()) {
			t.push_back((int)(begin->second & 0xffffffff));
		}
		for (size_t i = 0; i < t.size(); i++) {
			// static NetProto_Rank::Type typeLevel = NetProto_Rank::LEVEL;
			// Timer_AddEvent(end, -1, SevenDayRankAward, &typeLevel, "SevenDayRank");
			if (Config_InActiveTime(11)) { 
				static NetProto_Rank::Type typePower = NetProto_Rank::POWER;
				Timer_AddEvent(t[i], -1, SevenDayRankAward, &typePower, "SevenDayRank");
			}
		}
	}


	DEBUG_LOGRECORD("begin prepare playerpool");
	DEBUG_LOGRECORD("begin collect role");
	DCProto_CollectRole collect;
	GCAgent_SendProtoToDCAgent(&collect, false);
	GCAgent_WaitDCAgent(1);

	DCProto_GMLoadData gmLoadData;
	GCAgent_SendProtoToDCAgent(&gmLoadData, false);
	GCAgent_WaitDCAgent(1);

	DCProto_FactionLoadData factionData;
	GCAgent_SendProtoToDCAgent(&factionData, false);
	GCAgent_WaitDCAgent(1);


	Time_Flush();
	int64_t begin = Time_ElapsedTime();
	DEBUG_LOGRECORD("power end  %lldms", (long long)begin);

	DCProto_InitRank rankPower;
	rankPower.set_type(NetProto_Rank::POWER);
	GCAgent_SendProtoToDCAgent(&rankPower, false);
	GCAgent_WaitDCAgent(1);


	DCProto_InitRank rankTower;
	rankTower.set_type(NetProto_Rank::TOWER);
	GCAgent_SendProtoToDCAgent(&rankTower, false);
	GCAgent_WaitDCAgent(1);

	DCProto_InitRank rankLevel;
	rankLevel.set_type(NetProto_Rank::LEVEL);
	GCAgent_SendProtoToDCAgent(&rankLevel, false);
	GCAgent_WaitDCAgent(1);

	DCProto_InitRank rankWorldBoss;
	rankWorldBoss.set_type(NetProto_Rank::WORLD_BOSS);
	rankWorldBoss.set_flag(false);
	GCAgent_SendProtoToDCAgent(&rankWorldBoss, false);
	GCAgent_WaitDCAgent(1);

	DCProto_InitRank rankGod;
	rankGod.set_type(NetProto_Rank::GOD);
	GCAgent_SendProtoToDCAgent(&rankGod, false);
	GCAgent_WaitDCAgent(1);

	DCProto_InitRank rankFaction;
	rankFaction.set_type(NetProto_Rank::FACTION);
	GCAgent_SendProtoToDCAgent(&rankFaction, false);
	GCAgent_WaitDCAgent(1);

	DCProto_InitRank rankPet;
	rankPet.set_type(NetProto_Rank::PET);
	GCAgent_SendProtoToDCAgent(&rankPet, false);
	GCAgent_WaitDCAgent(1);

	{
		if (Config_InActiveTime(13)) {
			DCProto_InitRank rankDevil;
			rankDevil.set_type(NetProto_Rank::DEVIL);
			GCAgent_SendProtoToDCAgent(&rankDevil, false);
			GCAgent_WaitDCAgent(1);
		}

		if (Config_InActiveTime(20)) {
			DCProto_InitRank rankLuck;
			rankLuck.set_type(NetProto_Rank::LUCK);
			GCAgent_SendProtoToDCAgent(&rankLuck, false);
			GCAgent_WaitDCAgent(1);
		}

		if (Config_InActiveTime(23)) {
			DCProto_InitRank rankConsume;
			rankConsume.set_type(NetProto_Rank::CONSUME);
			GCAgent_SendProtoToDCAgent(&rankConsume, false);
			GCAgent_WaitDCAgent(1);
		}

		if (Config_InActiveTime(27)) {
			DCProto_InitRank catGift;
			catGift.set_type(NetProto_Rank::CATGIFT);
			GCAgent_SendProtoToDCAgent(&catGift, false);
			GCAgent_WaitDCAgent(1);
		}

		if (Config_InActiveTime(29)) {
			DCProto_InitRank purchase;
			purchase.set_type(NetProto_Rank::GROUP_PURCHASE);
			GCAgent_SendProtoToDCAgent(&purchase, false);
			GCAgent_WaitDCAgent(1);
		}
		{
			DCProto_InitRank purchaseRecord;
			purchaseRecord.set_type(NetProto_Rank::GROUPRECORD);
			GCAgent_SendProtoToDCAgent(&purchaseRecord, false);
			GCAgent_WaitDCAgent(1);
		}

		{
			DCProto_InitRank reservation;
			reservation.set_type(NetProto_Rank::RESERVATION);
			reservation.mutable_finalKiller()->set_arg1(Config_ReservationTime(0, 0));
			GCAgent_SendProtoToDCAgent(&reservation, false);
			GCAgent_WaitDCAgent(1);
		}
	}	

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap != NULL) {
		multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-8);
		if (begin != fixedMap->end()) {
			DCProto_InitRank rankBless;
			rankBless.set_type(NetProto_Rank::BLESSCOME);
			GCAgent_SendProtoToDCAgent(&rankBless, false);
			GCAgent_WaitDCAgent(1);
		}
	}

	DCProto_RandomRoles randomRoles;
	randomRoles.set_count(Config_MaxPlayersOfLineInPeace() * 5);
	GCAgent_SendProtoToDCAgent(&randomRoles, false);
	GCAgent_WaitDCAgent(1);

	DCProto_LoadAllDataFromGMDataTable gmDataTable;
	GCAgent_SendProtoToDCAgent(&gmDataTable, false);
	GCAgent_WaitDCAgent(1);

	DCProto_LoadRekooRole rekooRole;
	GCAgent_SendProtoToDCAgent(&rekooRole, false);
	GCAgent_WaitDCAgent(1);

	DCProto_LoadInviteCode inviteCode;
	GCAgent_SendProtoToDCAgent(&inviteCode, false);
	GCAgent_WaitDCAgent(1);
	
	pool.reservationMap = MapPool_Gen(Config_ReservationMap(), true);
	if (pool.reservationMap == -1)
		exit(1);

	DEBUG_LOGRECORD("end prepare playerpool");
}

void PlayerPool_Finalize() {
	static DCProto_SaveRoleData saveRoleData;
	saveRoleData.Clear();

	vector<struct PlayerEntity *> allPlayers;
	int allCount = PlayerPool_Players(&allPlayers);
	if (allCount > 0) {
		for (int i = 0; i < allCount; i++) {
			if (!PlayerEntity_IsValid(allPlayers[i]))
				continue;
			PlayerEntity_Logout(allPlayers[i], false);
		}
	}

	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-21);
	saveRecord.mutable_record()->mutable_role()->set_roleID(pool.catGiftCount);
	saveRecord.mutable_record()->set_arg1(pool.catGiftEndTime);
	GCAgent_SendProtoToDCAgent(&saveRecord);
	
	saveRecord.Clear();
	saveRecord.set_mapID(-22);
	saveRecord.mutable_record()->mutable_role()->set_roleID(pool.groupPurchase);
	saveRecord.mutable_record()->set_arg1(pool.groupPurchaseEndTime);
	GCAgent_SendProtoToDCAgent(&saveRecord);

	PlayerPool_SaveReservationToDB();

	saveRoleData.Clear();
	saveRoleData.add_data()->mutable_att()->mutable_baseAtt()->set_roleID(CONFIG_EXIT_CODE);
	saveRoleData.add_info();
	GCAgent_SendProtoToDCAgent(&saveRoleData, false);
}

void PlayerPool_SetIdles(int64_t cur) {
	pool.cur = cur;
	DEBUG_LOG("cur role: %lld", cur);
}

int64_t PlayerPool_GenRoleID() {
	return pool.cur == -1 ? -1 : pool.cur++;
}

void PlayerPool_FilterDesignation(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return;

	int64_t roleID = PlayerEntity_RoleID(entity);
	const PlayerAtt *att = PlayerEntity_Att(entity);
	for (int i = (int)NetProto_Rank::Type_MIN; i <= (int)NetProto_Rank::Type_MAX; i++) {
		static int64_t curRank[3] = {-1, -1, -1};
		RankTop3((NetProto_Rank::Type)i, curRank);

		for (size_t j = 0; j < 3; j++) {
			int32_t id = Config_RankDesignation((NetProto_Rank::Type)i, j);
			if (id == -1)
				continue;
			if (curRank[j] != roleID) {
				if (att->designationRecord(id).has()) {
					PlayerEntity_DelDesignation(entity, id);
				}
			} else {
				if (!att->designationRecord(id).has()) {
					PlayerEntity_AddDesignation(entity, id);
					// PlayerEntity_ShowDesignation(entity, id);
				}
			}
		}
	}
}

static inline bool IsValidMapID(int32_t id) {
	return id >= 0 && id < pool.maxMaps;
}

static int FindLine(int32_t map, int limit) {
	vector<set<struct PlayerEntity *> > &container = pool.ids[map];
	for (size_t i = 0; i < container.size(); i++) {
		if ((int)container[i].size() < limit)
			return (int)i;
	}

	container.resize(container.size() + 1);
	return (int)container.size() - 1;
}

static int FindFactionWarLine(int32_t map, int limit, const char *faction) {
	vector<set<struct PlayerEntity *> > &container = pool.ids[map];
	for (size_t i = 0; i < container.size(); i++) {
		if ((int)container[i].size() < limit) {
			int count = 0;
			for (set<struct PlayerEntity *>::iterator it = container[i].begin(); it != container[i].end(); it++) {
				struct Component *component = PlayerEntity_Component(*it);
				if (component == NULL)
					continue;
				if (strcmp(component->playerAtt->faction(), faction) == 0)
					count++;
			}
			if (count < Config_MaxCountInFactionInFactionWar())
				return (int)i;
		}
	}

	container.resize(container.size() + 1);
	return (int)container.size() - 1;
}

int PlayerPool_Add(struct PlayerEntity *playerEntity) {
	if (playerEntity == NULL)
		return -1;

	struct Component *component = PlayerEntity_Component(playerEntity);
	int mapID = Movement_Att(component->movement)->mapID();
	if (!IsValidMapID(mapID))
		return -1;

	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
	int line = 0;
	if (mapInfo != NULL) {
		if (mapInfo->id() == Config_WorldBossMap()) {
			line = FindLine(mapID, Config_MaxPlayersOfLineInWorldBoss());
		} else if (mapInfo->id() == Config_FactionWarMap()) {
			line = FindFactionWarLine(mapID, Config_MaxPlayersOfLineInWorldBoss(), component->playerAtt->faction());
		} else if (mapInfo->id() == Config_FactionBossMap()) {
			line = FindLine(mapID, Config_MaxPlayersOfLineInWorldBoss());
		} else if (mapInfo->mapType() == MapInfo::PEACE) {
			line = FindLine(mapID, Config_MaxPlayersOfLineInPeace());
		}
		/*
		   else if (mapInfo->mapType() == MapInfo::HELL)
		   line = FindLine(mapID, Config_MaxPlayersOfLineInWorldBoss());
		 */
	}

	pool.ids[mapID][line].insert(playerEntity);
	Movement_SetLine(component->movement, line);

	return 0;
}

void PlayerPool_Del(struct PlayerEntity *playerEntity) {
	if (playerEntity == NULL)
		return;

	struct Component *component = PlayerEntity_Component(playerEntity);
	int mapID = Movement_Att(component->movement)->mapID();
	if (!IsValidMapID(mapID))
		return;

	int line = Movement_Line(component->movement);
	assert(line >= 0 && line < (int)pool.ids[mapID].size());

	size_t count = pool.ids[mapID][line].erase(playerEntity);
	assert(count == 1);
}

int PlayerPool_Count(int32_t mapID, int line) {
	if (!IsValidMapID(mapID))
		return -1;

	int32_t real = MapPool_MapInfo(mapID);
	if (real == -1 || real < MAP_START)
		return 0;

	if (MapPool_IsSingle(mapID))
		return 1;

	if (line == -1) {
		int total = 0;
		for (size_t i = 0; i < pool.ids[mapID].size(); i++)
			total += (int)pool.ids[mapID][i].size();
		return total;
	} else {
		if (line < 0 || line >= (int)pool.ids[mapID].size())
			return -1;
		return (int)pool.ids[mapID][line].size();
	}
}

int PlayerPool_TotalCount() {
	int total = 0;
	for (size_t i = 0; i < pool.ids.size(); i++)
		for (size_t j = 0; j < pool.ids[i].size(); j++)
			total += (int)pool.ids[i][j].size();
	return total;
}

int PlayerPool_Players(int32_t mapID, struct PlayerEntity *player, vector<struct PlayerEntity *> *players) {
	if (!IsValidMapID(mapID) || players == NULL)
		return -1;

	if (mapID < MAP_START || MapPool_IsSingle(mapID)) {
		if (!PlayerEntity_IsValid(player))
			return -1;
		players->push_back(player);
		return 1;
	}

	int line = 0;
	if (PlayerEntity_IsValid(player)) {
		struct Component *component = PlayerEntity_Component(player);
		line = Movement_Line(component->movement);
	}

	assert(line >= 0 && line < (int)pool.ids[mapID].size());

	set<struct PlayerEntity *> &container = pool.ids[mapID][line];
	int count = 0;
	for (set<struct PlayerEntity *>::iterator it = container.begin(); it != container.end(); it++) {
		players->push_back(*it);
		count++;
	}

	return count;
}

int PlayerPool_Players(int32_t mapID, vector<struct PlayerEntity *> *players) {
	if (!IsValidMapID(mapID) || players == NULL)
		return -1;

	if (mapID < MAP_START || MapPool_IsSingle(mapID))
		return -1;

	int count = 0;
	for (size_t i = 0; i < pool.ids[mapID].size(); i++) {
		set<struct PlayerEntity *> &container = pool.ids[mapID][i];
		for (set<struct PlayerEntity *>::iterator it = container.begin(); it != container.end(); it++) {
			players->push_back(*it);
			count++;
		}
	}

	return count;
}

int PlayerPool_Players(vector<struct PlayerEntity *> *players) {
	if (players == NULL)
		return -1;

	int count = 0;
	for (size_t i = MAP_CHANGING; i < pool.ids.size(); i++) {
		for (size_t j = 0; j < pool.ids[i].size(); j++) {
			set<struct PlayerEntity *> &container = pool.ids[i][j];
			for (set<struct PlayerEntity *>::iterator it = container.begin(); it != container.end(); it++) {
				players->push_back(*it);
				count++;
			}
		}
	}
	return count;
}

int PlayerPool_FreeFaction(int32_t map, struct PlayerEntity *entity) {
	static vector<struct PlayerEntity *> players;
	players.clear();
	int count = PlayerPool_Players(map, entity, &players);
	int faction = 0;
	for (int i = 0; i < count; i++) {
		struct Component *component = PlayerEntity_Component(players[i]);
		if (component != NULL)
			faction |= component->roleAtt->fightAtt().selfFaction();
	}
	for (int i = 0; i < Config_MaxPlayersOfLineInWorldBoss(); i++) {
		if ((faction & (1 << i)) == 0) {
			return 1 << i;
		}
	}
	return 1;
}

static void * Push(void *arg) {
	char *content = (char *)arg;
	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, "cd %s && ./%s \"%s\" 1>/dev/null 2>&1", Config_PushDir(), Config_Push(), content);
	system(buf);
	DEBUG_LOG("Push: %s", buf);
	return NULL;
}

void PlayerPool_Push(const char *content) {
	if (Config_Push() == NULL || content == NULL)
		return;

	char *copy = new char[strlen(content) + 1];
	strcpy(copy, content);

	pthread_t t;
	if (pthread_create(&t, NULL, &Push, copy) != 0) {
		DEBUG_LOGERROR("Failed to run Push");
	}
	delete[] copy;
}

int PlayerPool_ServerDayNum(ItemInfo::Type type, int32_t id) {
	map<int64_t, int64_t>::iterator iter = pool.restriction.find( TwoInt32ToInt64( (int32_t)type, id ) );
	if (iter != pool.restriction.end()) {
		int timeDayDelta = Time_DayDelta(iter->second >> 32, Time_TimeStamp());
		if (timeDayDelta == 0) {	
			return iter->second & 0xFFFFFFFF;
		} else {
			pool.restriction[ TwoInt32ToInt64( (int32_t)type, id ) ] =  TwoInt32ToInt64(Time_TimeStamp(), 0);
			PlayerPool_AddServerDayNum(type, id, 0);
		}
	}
	return 0;
}

void PlayerPool_AddServerDayNum(ItemInfo::Type type, int32_t id, int delta) {
	int64_t index = TwoInt32ToInt64( (int32_t)type, id );
	map<int64_t, int64_t>::iterator iter = pool.restriction.find( index );
	bool flag = false;
	if (iter != pool.restriction.end()) {
		int timeDayDelta = Time_DayDelta(iter->second >> 32, Time_TimeStamp());
		if (timeDayDelta == 0) {	
			pool.restriction[index] =  TwoInt32ToInt64(Time_TimeStamp(), iter->second & 0xFFFFFFFF) +  delta;
		} else {
			pool.restriction[index] = TwoInt32ToInt64(Time_TimeStamp(), delta);
		}
	} else {
		pool.restriction.insert(make_pair(index, TwoInt32ToInt64(Time_TimeStamp(), delta)));
		flag = true;
	}
	PlayerPool_SaveRestrictionRecord(index, pool.restriction[index], flag);
}

void PlayerPool_SaveRestrictionRecord(int64_t index, int64_t num, bool flag) {
	int32_t h = 0;
	int32_t l = 0;
	Int64ToTwoInt32(index, &h, &l);
	if ((int)pool.restriction.size() >= Config_MaxRestrictionNum()) {
		DEBUG_LOG("restriction is full");
		return;
	}
	char ch[64] = {0};
	SNPRINTF1(ch, "%d:%d-%d",(int)pool.restriction.size(), h, l);

	RecordInfo record;
	record.set_arg1((::google::protobuf::int64)num);
	record.mutable_role()->set_roleID(0);
	record.mutable_role()->set_name(string(ch).c_str());
	//record.mutable_role()->set_professionType(0);
	DCProto_SaveSingleRecord save;
	if (flag) {
		save.set_mapID(-1);
	} else {
		save.set_mapID(-2);
	}
	*save.mutable_record() = record;
	GCAgent_SendProtoToDCAgent(&save);
	//DataManager_SaveRestrictionRecord(string(ch).c_str(), num);
}

void PlayerPool_SetRestrictionRecords(const google::protobuf::RepeatedPtrField<RecordInfo> *Records) {
	if (Records == NULL) {
		return;
	}

	for (int i = 0; i < Records->size(); ++i) {
		string str = Records->Get(i).role().name();
		size_t flag = str.find("-");
		if (flag != string::npos) {
			string strType = str.substr(0,(int)flag);
			string strID = str.substr((int)flag + 1,(int)(str.length() - flag -1));
			int32_t type = atoi(strType.c_str());
			int32_t id = atoi(strID.c_str());
			pool.restriction.insert(make_pair(TwoInt32ToInt64( type, id ), Records->Get(i).arg1()));
		}
	} 
}

static void SysRankInfo(int64_t *rank, NetProto_Rank::Type type) {
	vector<RecordInfo>* cur = CurRank(type);
	assert(cur != NULL);

	int tmp[3];
	for (int i = 0; i < 3 && i < (int)cur->size(); ++i) {
		if (*(rank + i) == (*cur)[i].role().roleID()) {
			tmp[i] = -1;
		}
	}

	bool up[3] = {false, false, false};
	for (size_t i = 0; i < cur->size() && i < 3; ++i) {
		if (i == 0) {
			if ((*cur)[i].role().roleID() != *(rank + i)) {
				up[i] = true;
			}
		} else if (i == 1) {
			if ((*cur)[i].role().roleID() != *(rank + i) && (*cur)[i].role().roleID() != *rank) {
				up[i] = true;
			}
		} else if (i == 2) {
			if ((*cur)[i].role().roleID() != *(rank + i) && (*cur)[i].role().roleID() != *(rank + 1) && (*cur)[i].role().roleID() != *rank) {
				up[i] = true;
			}
		}
	}


	for (int i = 0; i < 3; ++i) {
		int32_t id = Config_RankDesignation(type, i);
		if (-1 != id && tmp[i] != -1) {
			if (*(rank + i) != -1) {
				struct PlayerEntity * oldEntity = PlayerEntity_PlayerByRoleID(*(rank + i));
				if (oldEntity != NULL) {
					PlayerEntity_DelDesignation(oldEntity, id);
				}
			}
		}
	}

	for (size_t i = 0; i < cur->size() && i < 3; ++i) {
		struct PlayerEntity * newEntity = PlayerEntity_PlayerByRoleID((*cur)[i].role().roleID());
		int32_t id = Config_RankDesignation(type, i);
		if (-1 != id && tmp[i] != -1) {
			if (newEntity != NULL) {
				PlayerEntity_AddDesignation(newEntity, id);
				if (up[i]) {
					NoticeRank(newEntity, type, i + 1);
				}
			}
		}
	}
}

static void Init_Reservation(DCProto_InitRank* rank) {
	if (rank == NULL)
		return;
//TODO
	static PB_ReservationToString proto;
	for (int64_t i = 0; i < rank->rank_size(); ++i) {
		proto.Clear();
		Reservation tmp;
		if (proto.ParseFromString(rank->rank(i).role().name())) {
		
			{
				tmp.powerType = proto.powerType();
				tmp.roleID1 = proto.roleID1();
				tmp.roleID2 = proto.roleID2();
				tmp.name1 = proto.name1();
				tmp.name2 = proto.name2();

				{
					for (int i = 0; i < (int)proto.setRole1_size(); ++i) {
						tmp.setRole1.insert(proto.setRole1(i));
					}
					for (int i = 0; i < (int)proto.setRole2_size(); ++i) {
						tmp.setRole2.insert(proto.setRole2(i));
					}
				}

				tmp.rmb = proto.rmb();
				tmp.type1 = proto.type1();
				tmp.type2 = proto.type2();
			}



				
			int index = rank->rank(i).role().roleID();
			if (index > 0 && index < 21) {
				pool.reservation[index - 1] = tmp;
			}
		}
	}
}

void PlayerPool_InitRank(DCProto_InitRank* rank) {
	if (rank == NULL) {
		return;
	}

	DEBUG_LOG("AAAAAAA%d---- %d ,---",rank->type(),rank->rank_size());
	if (rank->type() == NetProto_Rank::CATGIFT) {
		if (rank->rank_size() > 0) {
			pool.catGiftCount = rank->rank(0).role().roleID();
			pool.catGiftEndTime = rank->rank(0).arg1();
		}
		return;
	}

	if (rank->type() == NetProto_Rank::RESERVATION) {
		if (rank->rank_size() > 0) {
			Init_Reservation(rank);
		}
		return;
	}

	if (rank->type() == NetProto_Rank::GROUPRECORD) {
		if (rank->rank_size() > 0) {
			pool.groupPurchase = rank->rank(0).role().roleID();
			pool.groupPurchaseEndTime = rank->rank(0).arg1();
			DEBUG_LOG("AAAAAAAAAAAAA%d, - %d", pool.groupPurchase, pool.groupPurchaseEndTime);
		}
		return;
	}

	vector<RecordInfo>* cur = CurRank(rank->type());
	if (cur == NULL) {
		return;
	}

	int64_t rankTop3[3] = {-1, -1, -1};
	if (rank->type() == NetProto_Rank::POWER || rank->type() == NetProto_Rank::TOWER ||rank->type() == NetProto_Rank::LEVEL) {
		if (cur->size() != 0) {
			int count = 0;
			for (vector<RecordInfo>::iterator itor = cur->begin(); itor != cur->end() && count < 3; ++itor, ++count) {
				rankTop3[count] = itor->role().roleID();
			}
		}
	}

	if (rank->type() == NetProto_Rank::POWER) {
		pool.rankPower.clear();
		pool.rankPowerInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankPower.push_back(rank->rank(i));
			pool.rankPowerInfo[rank->rank(i).role().roleID()] = i;
		}
		Time_Flush();
		int64_t begin = Time_ElapsedTime();
		DEBUG_LOGRECORD("power end  %lldms", (long long)begin);
	} else if (rank->type() == NetProto_Rank::TOWER) {
		pool.rankTower.clear();
		pool.rankTowerInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankTower.push_back(rank->rank(i));
			pool.rankTowerInfo[rank->rank(i).role().roleID()] = i;
		}
	} else if (rank->type() == NetProto_Rank::LEVEL) {
		pool.rankLevel.clear();
		pool.rankLevelInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankLevel.push_back(rank->rank(i));
			pool.rankLevelInfo[rank->rank(i).role().roleID()] = i;
		}
	} else if (rank->type() == NetProto_Rank::WORLD_BOSS) {
		pool.rankWorldBoss.clear();
		pool.rankWorldBossInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankWorldBoss.push_back(rank->rank(i));
			pool.rankWorldBossInfo[rank->rank(i).role().roleID()] = i;
		}
		if (rank->has_finalKiller()) {
			pool.rankWorldBossFinal = rank->finalKiller();
		}
	} else if (rank->type() == NetProto_Rank::GOD) {
		pool.rankGod.clear();
		pool.rankGodInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankGod.push_back(rank->rank(i));
			pool.rankGodInfo[rank->rank(i).role().roleID()] = i;
		}
	} else if (rank->type() == NetProto_Rank::BLESSCOME) {
		pool.rankBlessingCome.clear();
		pool.rankBlessingComeInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankBlessingCome.push_back(rank->rank(i));
			pool.rankBlessingComeInfo[rank->rank(i).role().roleID()] = i;
		}
	} else if (rank->type() == NetProto_Rank::FACTION) {
		pool.isRankFaction = false;
		pool.rankFactionInfo.clear();

		static DCProto_FactionPower proto;
		proto.Clear();
		static vector<int64_t> vec;
		char ch[128];

		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankFactionInfo.push_back(rank->rank(i));

			FactionPower* info = proto.add_info();	
			info->set_str(rank->rank(i).role().name());
			vec.clear();
			memset(ch, 0, sizeof(ch) / sizeof(char));

			for (int j = 0; j < (int)rank->rank(i).role().name().size(); ++j) {
				if (rank->rank(i).role().name().at(j) == ',') {
					ch[j] = '\0';
					break;
				}else {
					ch[j] = rank->rank(i).role().name().at(j);
				}
			}
			FactionPool_GetRolesFromFaction(ch, 1 | 2 | 4 | 8, &vec);

			for (int j = 0; j < (int)vec.size(); ++j) {
				info->add_roleID(vec[j]);
			}
		}
		GCAgent_SendProtoToDCAgent(&proto);
	} else if (rank->type() == NetProto_Rank::PET) {
		pool.rankPetInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankPetInfo.push_back(rank->rank(i));
		}
	} else if (rank->type() == NetProto_Rank::DEVIL) {
		pool.rankDevilInfo.clear();
		pool.rankDevil.clear();
		RecordInfo devilRecordInfo;
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankDevil.push_back(rank->rank(i));

			int64_t arg1 = rank->rank(i).arg1(); 
			devilRecordInfo = rank->rank(i);

			arg1 += 0x0100000000;
			devilRecordInfo.set_arg1(arg1);
			pool.rankDevilInfo[rank->rank(i).role().roleID()] = devilRecordInfo;
		}
		DCProto_QueryRoleFaction info;
		for (map<int64_t, RecordInfo>::iterator it = pool.rankDevilInfo.begin(); it != pool.rankDevilInfo.end(); ++it) {
			info.add_roleID(it->first);
			info.add_faction("");
		}
		GCAgent_SendProtoToDCAgent(&info);
	} else if (rank->type() == NetProto_Rank::LUCK) {
		pool.rankLuck.clear();
		pool.rankLuckInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankLuck.push_back(rank->rank(i));
			pool.rankLuckInfo[rank->rank(i).role().roleID()] = i;
		}
	} else if (rank->type() == NetProto_Rank::CONSUME) {
		pool.rankConsume.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankConsume.push_back(rank->rank(i));
		}
	} else if (rank->type() == NetProto_Rank::GROUP_PURCHASE) {
		pool.rankPurchase.clear();
		pool.rankPurchaseInfo.clear();
		for (int64_t i = 0; i < rank->rank_size(); ++i) {
			pool.rankPurchase.push_back(rank->rank(i));
			pool.rankPurchaseInfo[rank->rank(i).role().roleID()] = i;
		}
	} else {
		return;
	}

	if (rank->type() == NetProto_Rank::POWER || rank->type() == NetProto_Rank::TOWER ||rank->type() == NetProto_Rank::LEVEL) {
		SysRankInfo(rankTop3, rank->type());
	}
}

int64_t PlayerPool_QueryRank(const int64_t roleID, NetProto_Rank* rank) {
	if (rank == NULL) {
		return -1;
	}

	if (rank->type() == NetProto_Rank::POWER) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankPower.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankPower[i];
		}
		map<int64_t, int64_t>::iterator it = pool.rankPowerInfo.find(roleID);
		if (it == pool.rankPowerInfo.end()) {
			rank->mutable_self()->mutable_role()->set_roleID(-1);
		} else {
			*rank->mutable_self() = pool.rankPower[it->second];
			rank->set_range(it->second);
		}
	} else if (rank->type() == NetProto_Rank::TOWER) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankTower.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankTower[i];
		}
		map<int64_t, int64_t>::iterator it = pool.rankTowerInfo.find(roleID);
		if (it == pool.rankTowerInfo.end()) {
			rank->mutable_self()->mutable_role()->set_roleID(-1);
		} else {
			*rank->mutable_self() = pool.rankTower[it->second];
			rank->set_range(it->second);
		}
	} else if (rank->type() == NetProto_Rank::LEVEL) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankLevel.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankLevel[i];
		}
		map<int64_t, int64_t>::iterator it = pool.rankLevelInfo.find(roleID);
		if (it == pool.rankLevelInfo.end()) {
			rank->mutable_self()->mutable_role()->set_roleID(-1);
		} else {
			*rank->mutable_self() = pool.rankLevel[it->second];
			rank->set_range(it->second);
		}
	} else if (rank->type() == NetProto_Rank::WORLD_BOSS) {
		RecordInfo* info = rank->mutable_finalKiller();
		*info = pool.rankWorldBossFinal;
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankWorldBoss.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankWorldBoss[i];
		}
		map<int64_t, int64_t>::iterator it = pool.rankWorldBossInfo.find(roleID);
		if (it == pool.rankWorldBossInfo.end()) {
			rank->mutable_self()->mutable_role()->set_roleID(-1);
		} else {
			*rank->mutable_self() = pool.rankWorldBoss[it->second];
			rank->set_range(it->second);
		}
	} else if (rank->type() == NetProto_Rank::GOD) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankGod.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankGod[i];
		}
	} else if (rank->type() == NetProto_Rank::BLESSCOME) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankBlessingCome.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankBlessingCome[i];
		}
		map<int64_t, int64_t>::iterator it = pool.rankBlessingComeInfo.find(roleID);
		if (it == pool.rankBlessingComeInfo.end()) {
			rank->mutable_self()->mutable_role()->set_roleID(-1);
		} else {
			*rank->mutable_self() = pool.rankBlessingCome[it->second];
			rank->set_range(it->second);
		}
	} else if (rank->type() == NetProto_Rank::FACTION) {
		if (pool.isRankFaction) {
			for (int i = 0; i < Config_RankCount() && i < (int)pool.rankFactionInfo.size(); ++i) {
				RecordInfo* info = rank->add_rank();
				*info = pool.rankFactionInfo[i];
			}
		}
	} else if (rank->type() == NetProto_Rank::PET) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankPetInfo.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankPetInfo[i];
		}
	} else if (rank->type() == NetProto_Rank::DEVIL) {
		if (pool.isRankDevil) {
			for (int i = 0; i < Config_RankCount() && i < (int)pool.vecDevilRank.size(); ++i) {
				RecordInfo* info = rank->add_rank();
				*info = pool.vecDevilRank[i];
			}
		}
	} else if (rank->type() == NetProto_Rank::LUCK) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankLuck.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankLuck[i];
		}
		map<int64_t, int64_t>::iterator it = pool.rankLuckInfo.find(roleID);
		if (it == pool.rankLuckInfo.end()) {
			rank->mutable_self()->mutable_role()->set_roleID(-1);
		} else {
			*rank->mutable_self() = pool.rankLuck[it->second];
			rank->set_range(it->second);
		}
	} else if (rank->type() == NetProto_Rank::CONSUME) {
		for (int i = 0; i < Config_RankCount() && i < (int)pool.rankConsume.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankConsume[i];
		}
	} else if (rank->type() == NetProto_Rank::GROUP_PURCHASE) {
		for (int i = 0; i < 10 && i < (int)pool.rankPurchase.size(); ++i) {
			RecordInfo* info = rank->add_rank();
			*info = pool.rankPurchase[i];
		}
		map<int64_t, int64_t>::iterator it = pool.rankPurchaseInfo.find(roleID);
		if (it == pool.rankPurchaseInfo.end()) {
			rank->mutable_self()->mutable_role()->set_roleID(-1);
		} else {
			*rank->mutable_self() = pool.rankPurchase[it->second];
			rank->set_range(it->second);
		}
	} 

	return 0;
}

void PlayerPool_WorldBossRank(DCProto_InitRank* rank) {
	if (rank == NULL) {
		return;	
	}

	set<RecordInfo, RecordComp_> setPlayer;
	for (int64_t i = 0; i < rank->rank_size(); ++i) {
		setPlayer.insert(rank->rank(i));
	}

	vector<struct PlayerEntity *> allOnLinePlayer;		
	PlayerPool_Players(&allOnLinePlayer);

	set<RecordInfo, RecordComp_> setOnLinePlayer;
	RecordInfo info;
	for (int i = 0; i < (int)allOnLinePlayer.size(); ++i) {
		const PlayerAtt* att = PlayerEntity_Att(allOnLinePlayer[i]);
		if (att != NULL) {
			if (att->att().fightAtt().worldBossNum() == MapPool_WorldBossNum()) {
				info.set_arg1(att->att().fightAtt().worldBossHurt());
				info.mutable_role()->set_roleID(att->att().baseAtt().roleID());
				info.mutable_role()->set_name(att->att().baseAtt().name());
				info.mutable_role()->set_professionType((PB_ProfessionInfo::Type)att->att().baseAtt().professionType());
				setOnLinePlayer.insert(info);
			}
		}
	}

	multiset<RecordInfo, RecordComp> setWorldBossHurt;
	set_union(setOnLinePlayer.begin(), setOnLinePlayer.end(), setPlayer.begin(), setPlayer.end(), inserter(setWorldBossHurt, setWorldBossHurt.begin()), RecordComp_());

	pool.rankWorldBoss.clear();
	pool.rankWorldBossInfo.clear();
	int index = 0;
	for (multiset<RecordInfo, RecordComp>::reverse_iterator itor = setWorldBossHurt.rbegin(); itor != setWorldBossHurt.rend(); ++itor) {
		pool.rankWorldBoss.push_back(*itor);
		pool.rankWorldBossInfo[itor->role().roleID()] = index++;
	}
	pool.rankWorldBossFinal = rank->finalKiller();

	{
		if (Config_InActiveTime(13)) {
			if (rank->finalKiller().role().roleID() != -1) {
				DCProto_SaveSingleRecord saveRecord;
				saveRecord.set_mapID(-14);
				*saveRecord.mutable_record() = rank->finalKiller();
				saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
				GCAgent_SendProtoToDCAgent(&saveRecord);
				pool.isRankDevil = false;

				DCProto_InitRank rankDevil;
				rankDevil.set_type(NetProto_Rank::DEVIL);
				GCAgent_SendProtoToDCAgent(&rankDevil, false);
			}
		}
	}	
}

void PlayerPool_SaveGodRankRecord(int64_t roleID1, int64_t& roleID2, bool flag, int& score) {
	map<int64_t, int64_t>::iterator itor1 = pool.rankGodInfo.find(roleID1);
	if (roleID2 == -1) {
		roleID2 = pool.rankGodCur.find(roleID1)->second;
	}
	map<int64_t, int64_t>::iterator itor2 = pool.rankGodInfo.find(roleID2);

	if (itor1 == pool.rankGodInfo.end() || itor2 == pool.rankGodInfo.end()) {
		return;
	}
	int up = itor1->second;
	if (true == (itor1->second > itor2->second) && true == flag) {
		swap(pool.rankGod[itor1->second], pool.rankGod[itor2->second]);
		swap(itor1->second, itor2->second);

		static DCProto_ModifyGodRank rank;
		rank.Clear();
		rank.set_roleID(roleID1);
		rank.set_rank(itor1->second);	
		GCAgent_SendProtoToDCAgent(&rank);

		rank.Clear();
		rank.set_roleID(roleID2);
		rank.set_rank(itor2->second);	
		GCAgent_SendProtoToDCAgent(&rank);

		if (Config_ScribeHost() != NULL) {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;

			SNPRINTF2(buf, index, "{\"roleID1\":\"%lld", (long long int)roleID1);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"roleID1Rank\":\"%lld", (long long int)itor1->second);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"roleID2\":\"%lld", (long long int)roleID2);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"roleID2Rank\":\"%lld", (long long int)itor2->second);
			index += strlen(index);
			SNPRINTF2(buf, index, "\"}");
			struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(roleID1);
			if (entity != NULL) {
				int32_t id  = PlayerEntity_ID(entity);
				ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "GodRank", buf));
			} else {
				struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(roleID2);
				if (entity != NULL) {
					int32_t id  = PlayerEntity_ID(entity);
					ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "GodRank", buf));
				} 
			}
		}
	}
	int64_t upRank = up - itor1->second;
	PlayerEntity_SaveGodRankRecords(roleID1, roleID2, flag, upRank);

	time_t OpenTime = (time_t)Config_OpenTime(NULL);
	struct tm local;
	Time_LocalTime(OpenTime, &local);
	tm tmOpenTime = local;
	time_t t = Time_TimeStamp();
	Time_LocalTime(t, &local);

	if (tmOpenTime.tm_year == local.tm_year && tmOpenTime.tm_mon == local.tm_mon && tmOpenTime.tm_mday == local.tm_mday) {
		return;
	} else {
		if (flag) {
			map<int64_t, int64_t>::iterator it = pool.rankGodInfo.find(roleID1);
			PlayerEntity_HistoryRecordAward(roleID1, (int)it->second, score);
		}
	}

	roleID2 = abs(upRank);
}

void PlayerPool_AddGodRank(int64_t roleID, const RecordInfo* info) {
	if (info == NULL) 
		return;

	map<int64_t, int64_t>::iterator itor = pool.rankGodInfo.find(roleID);
	if (itor != pool.rankGodInfo.end()) {
		return;
	}

	static DCProto_ModifyGodRank rank;
	rank.Clear();
	rank.set_roleID(roleID);
	rank.set_rank(pool.rankGod.size());	
	GCAgent_SendProtoToDCAgent(&rank);

	pool.rankGodInfo.insert(make_pair(roleID, (int64_t)pool.rankGod.size()));
	pool.rankGod.push_back(*info);
}

int PlayerPool_RandomGod(int64_t roleID, NetProto_GodRandomPlayer* info) {
	if (info == NULL) 
		return -1;

	map<int64_t, int64_t>::iterator itor = pool.rankGodInfo.find(roleID);
	if (itor == pool.rankGodInfo.end()) {
		return -2;
	}

	set<int64_t> v;
	int randNum = rand();
	if (itor->second != 0) {
		if (itor->second / 5 != 0) {
			v.insert( abs((randNum) % (itor->second / 5) + (itor->second - itor->second / 5)) );
			v.insert( abs((randNum * 2) % (itor->second / 5) + (itor->second - itor->second / 5)) );
			v.insert( abs((randNum * 3) % (itor->second / 5) + (itor->second - itor->second / 5)) );
		}
	}

	for (int i = itor->second - 1; v.size() < 3 && i >= 0; --i) {
		v.insert(i);
	}

	int i = 0;
	for (set<int64_t>::reverse_iterator it = v.rbegin(); it != v.rend() && i < 3; ++it) {
		if (pool.rankGod[*it].role().roleID() != roleID) {
			++i;
			NetProto_GodPlayer* player = info->add_player();
			player->set_roleID(pool.rankGod[*it].role().roleID());
			player->set_str(pool.rankGod[*it].role().name());
			player->set_professionType(pool.rankGod[*it].role().professionType());
			player->set_godRank(*it);
			player->set_level(pool.rankGod[*it].arg1() >> 32);
			player->set_power(pool.rankGod[*it].arg1() & 0xFFFFFFFF);
		} 
	}
	return 0;
}

void PlayerPool_SelectGodRole(int64_t roleID1, int64_t roleID2) {
	pool.rankGodCur[roleID1] = roleID2;
}

void PlayerPool_QueryGodRank(int64_t roleID, NetProto_GodPanel* info) {
	if (info == NULL)
		return;

	map<int64_t, int64_t>::iterator itor = pool.rankGodInfo.find(roleID);
	if (itor != pool.rankGodInfo.end()) {
		info->mutable_player()->set_godRank(itor->second);
	}
}

void PlayerPool_SynGodInfoTime(int32_t preTime, bool flag) {
	pool.rankGodRecordsTime = preTime;
	if (flag) {
		DCProto_SaveSingleRecord save;
		save.set_mapID(-10);
		save.mutable_record()->set_arg1(pool.rankGodRecordsTime);
		GCAgent_SendProtoToDCAgent(&save);
	}
}

void PlayerPool_SendGodAward(bool flag) {
	static vector<RecordInfo> rankAward;
	static int64_t index;
	if (flag) {
		if (Time_TheSameDay(pool.rankGodRecordsTime, Time_TimeStamp())) {
			return;	
		}

		PlayerPool_SynGodInfoTime(Time_TimeStamp(), true);
		rankAward.clear();
		rankAward = pool.rankGod;
		index = 0;

		DEBUG_LOGRECORD("PlayerPool_SendGodAward begin: %d", (int)Time_TimeStamp());
	}

	if (index < (int64_t)rankAward.size()) {
		int range = Config_AwardRange(index);
		const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::DAILY_GOD, range);
		assert(award != NULL);

		struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(rankAward[index].role().roleID());
		while (entity != NULL) {
			PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());

			index++;
			if (index >= (int64_t)rankAward.size()) {
				DEBUG_LOGRECORD("PlayerPool_SendGodAward end: %d", (int)Time_TimeStamp());
				return;
			}

			range = Config_AwardRange(index);
			award = AwardInfoManager_AwardInfo(AwardInfo::DAILY_GOD, range);
			assert(award != NULL);
			entity = PlayerEntity_PlayerByRoleID(rankAward[index].role().roleID());
		}

		if (entity == NULL) {
			static DCProto_SendMail dsm;
			dsm.Clear();
			dsm.set_id(-2);
			dsm.mutable_sm()->set_receiver(rankAward[index].role().roleID());
			dsm.mutable_sm()->mutable_mail()->set_title(award->name());
			dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
			dsm.mutable_sm()->mutable_mail()->set_content(award->content());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
			GCAgent_SendProtoToDCAgent(&dsm);
			index++;
			Time_Flush();
			DEBUG_LOGRECORD("TTTTTTTTTTTTT%d,%lld", index, Time_ElapsedTime());
		}
	} else {
		DEBUG_LOGRECORD("PlayerPool_SendGodAward end: %d", (int)Time_TimeStamp());
	}

}

void PlayerPool_SendWeekGodAward(bool flag) {
	static vector<RecordInfo> rankAwardWeek;
	static int64_t indexWeek;
	if (flag) {
		if (Time_TheSameDay(pool.rankGodRecordsTime, Time_TimeStamp())) {
			return;	
		}

		PlayerPool_SynGodInfoTime(Time_TimeStamp(), true);
		rankAwardWeek.clear();
		rankAwardWeek = pool.rankGod;
		indexWeek = 0;
	}

	if (indexWeek < (int64_t)rankAwardWeek.size()) {
		int range = Config_AwardRange(indexWeek);
		const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::WEEKLY_GOD, range);
		assert(award != NULL);

		struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(rankAwardWeek[indexWeek].role().roleID());
		while (entity != NULL) {
			PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());

			indexWeek++;
			if (indexWeek >= (int64_t)rankAwardWeek.size()) {
				return;
			}

			range = Config_AwardRange(indexWeek);
			award = AwardInfoManager_AwardInfo(AwardInfo::WEEKLY_GOD, range);
			assert(award != NULL);
			entity = PlayerEntity_PlayerByRoleID(rankAwardWeek[indexWeek].role().roleID());
		}

		if (entity == NULL) {
			static DCProto_SendMail dsm;
			dsm.Clear();
			dsm.set_id(-3);
			dsm.mutable_sm()->set_receiver(rankAwardWeek[indexWeek].role().roleID());
			dsm.mutable_sm()->mutable_mail()->set_title(award->name());
			dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
			dsm.mutable_sm()->mutable_mail()->set_content(award->content());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
			GCAgent_SendProtoToDCAgent(&dsm);
			indexWeek++;
		}
	}

}

void PlayerPool_SynRoleLevelAndPower(int64_t roleID, int64_t value) {
	map<int64_t, int64_t>::iterator itor = pool.rankGodInfo.find(roleID);
	if (itor != pool.rankGodInfo.end()) {
		pool.rankGod[itor->second].set_arg1(value);

	}
}

void PlayerPool_AddRoleInfoToRank(const PB_PlayerAtt* att) {
	int64_t roleID = att->att().baseAtt().roleID();
	map<int64_t, int64_t>::iterator it = pool.rankLevelInfo.find(roleID);
	RecordInfo info;

	if (it == pool.rankLevelInfo.end()) {
		pool.rankLevelInfo[roleID] = pool.rankLevel.size();

		info.mutable_role()->set_roleID(roleID);
		info.mutable_role()->set_name(att->att().baseAtt().name());
		info.mutable_role()->set_professionType((PB_ProfessionInfo::Type)att->att().baseAtt().professionType());
		info.set_arg1((int64_t)att->att().fightAtt().level());
		pool.rankLevel.push_back(info);
	}

	it = pool.rankPowerInfo.find(roleID);
	if (it == pool.rankPowerInfo.end()) {
		pool.rankPowerInfo[roleID] = pool.rankPower.size();

		info.mutable_role()->set_roleID(roleID);
		info.mutable_role()->set_name(att->att().baseAtt().name());
		info.mutable_role()->set_professionType((PB_ProfessionInfo::Type)att->att().baseAtt().professionType());
		info.set_arg1((int64_t)Fight_Power(&att->att().fightAtt()));
		pool.rankPower.push_back(info);
	}

	it = pool.rankTowerInfo.find(roleID);
	if (it == pool.rankTowerInfo.end()) {
		pool.rankTowerInfo[roleID] = pool.rankTower.size();

		info.mutable_role()->set_roleID(roleID);
		info.mutable_role()->set_name(att->att().baseAtt().name());
		info.mutable_role()->set_professionType((PB_ProfessionInfo::Type)att->att().baseAtt().professionType());
		info.set_arg1((int64_t)att->att().fightAtt().maxTower());
		pool.rankTower.push_back(info);
	}

}

void PlayerPool_UpdateFreeRoles(const google::protobuf::RepeatedPtrField<PB_PlayerAtt> *atts) {
	if (atts == NULL)
		return;

	pool.free.clear();
	for (int i = 0; i < atts->size(); i++) {
		pool.free.push_back(atts->Get(i));
	}
}

static const PB_Vector3f * RandomFreeCoord(const MapUnit *mapUnit, bool reset) {
	static int index = 0;
	if (reset)
		index = Time_Random(0, mapUnit->freeCoords_size());
	if (index >= mapUnit->freeCoords_size())
		index = 0;
	return &mapUnit->freeCoords(index++);
}

void PlayerPool_RandomFreeRoles(int32_t map, int count, std::vector<int64_t> *exists, google::protobuf::RepeatedPtrField<PB_PlayerAtt> *atts) {
	if (atts == NULL)
		return;

	const MapInfo *mapInfo = MapInfoManager_MapInfo(map);
	const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
	if (mapUnit == NULL || mapUnit->freeCoords_size() <= 0)
		return;

	int size = (int)pool.free.size();
	if (size <= 0)
		return;

	int begin = Time_Random(0, size);
	int total = 0;

	Vector3f v3;
	Vector2i v2;

	for (int i = begin; i < size; i++) {
		if (total >= count)
			return;
		if (exists != NULL) {
			bool has = false;
			for (size_t j = 0; j < exists->size(); j++) {
				if (pool.free[i].att().baseAtt().roleID() == (*exists)[j]) {
					has = true;
					break;
				}
			}
			if (has)
				continue;
		}
		PB_PlayerAtt *att = atts->Add();
		PlayerEntity_ToSceneData(&pool.free[i], att);
		PB_MovementAtt *mAtt = att->mutable_att()->mutable_movementAtt();
		mAtt->set_mapID(map);
		*mAtt->mutable_position() = *RandomFreeCoord(mapUnit, total == 0);
		v3.FromPB(&mAtt->position());
		MapInfoManager_LogicCoordByPosition(&v3, &v2);
		v2.ToPB(mAtt->mutable_logicCoord());
		total++;
	}
	for (int i = 0; i < begin; i++) {
		if (total >= count)
			return;
		if (exists != NULL) {
			bool has = false;
			for (size_t j = 0; j < exists->size(); j++) {
				if (pool.free[i].att().baseAtt().roleID() == (*exists)[j]) {
					has = true;
					break;
				}
			}
			if (has)
				continue;
		}
		PB_PlayerAtt *att = atts->Add();
		PlayerEntity_ToSceneData(&pool.free[i], att);
		PB_MovementAtt *mAtt = att->mutable_att()->mutable_movementAtt();
		mAtt->set_mapID(map);
		*mAtt->mutable_position() = *RandomFreeCoord(mapUnit, total == 0);
		v3.FromPB(&mAtt->position());
		MapInfoManager_LogicCoordByPosition(&v3, &v2);
		v2.ToPB(mAtt->mutable_logicCoord());
		total++;
	}
}

void PlayerPool_RandomFreeRoles(int count, std::vector<int64_t> *exists, std::vector<PlayerAtt> *atts) {
	if (atts == NULL)
		return;

	int size = (int)pool.free.size();
	if (size <= 0)
		return;

	int begin = Time_Random(0, size);
	int total = 0;

	for (int i = begin; i < size; i++) {
		if (total >= count)
			return;
		if (exists != NULL) {
			bool has = false;
			for (size_t j = 0; j < exists->size(); j++) {
				if (pool.free[i].att().baseAtt().roleID() == (*exists)[j]) {
					has = true;
					break;
				}
			}
			if (has)
				continue;
		}
		PlayerAtt att;
		att.FromPB(&pool.free[i]);
		atts->push_back(att);
		total++;
	}
	for (int i = 0; i < begin; i++) {
		if (total >= count)
			return;
		if (exists != NULL) {
			bool has = false;
			for (size_t j = 0; j < exists->size(); j++) {
				if (pool.free[i].att().baseAtt().roleID() == (*exists)[j]) {
					has = true;
					break;
				}
			}
			if (has)
				continue;
		}
		PlayerAtt att;
		att.FromPB(&pool.free[i]);
		atts->push_back(att);
		total++;
	}
}

int PlayerPool_RandomHire(int64_t roleID, int64_t* ids) {
	map<int64_t, int64_t>::iterator it = pool.rankPowerInfo.find(roleID);
	if (it == pool.rankPowerInfo.end()) {
		return -1;
	}

	static vector<int64_t> vRoleID;
	vRoleID.clear();
	for (int i = 1; i < 10; ++i) {
		if (it->second + i < (int64_t)pool.rankPower.size()) {
			vRoleID.push_back(pool.rankPower[it->second + i].role().roleID());
		}

		if (it->second - i >= 0) {
			vRoleID.push_back(pool.rankPower[it->second - i].role().roleID());
		}
	}

	int randNum = Time_Random(0, 100);
	for (int i = 0; i < 5; ++i) {
		if (vRoleID.size() == 0){
			return -2;
		}

		int index = randNum % (int)vRoleID.size();
		*(ids + i) = vRoleID[index];
		vRoleID.erase(vRoleID.begin() + index);
	}

	return 0;
}

int PlayerPool_Blessing(struct PlayerEntity* player, NetProto_BlessCome* info) {
	if (info == NULL)
		return -1;

	if (player == NULL)
		return -1;

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return -2;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-8);
	if (begin == fixedMap->end()) {
		return -2;
	}

	struct Component *component = PlayerEntity_Component(player);
	int startTime = component->playerAtt->fixedEvent(67);
	if (info->count() == 0) {
		if (Time_TimeStamp() < startTime) {
			return -3;
		}
	}

	//const ItemPackage *package = Item_Package(component->item);
	//if (package->rmb() < info->count() * Config_Blessing()) {
	if (!Item_HasRMB(component->item, info->count() * Config_Blessing())) {
		return -4;
	}

	if (info->count() == 0) {
		int blessCount = component->playerAtt->fixedEvent(68);
		if ((blessCount & 0x0FFFF) >= 100) {
			return -5;
		}
		PlayerEntity_SetFixedEvent(player, 67, Time_TimeStamp() + 5 * 60);
		PlayerEntity_SetFixedEvent(player, 68, blessCount + 1);
	}

	Item_ModifyRMB(component->item, -info->count() * Config_Blessing(), false, 27, 0, 0);
	int delta = info->count();
	if (delta == 0)
		delta = 1;

	Item_ModifyBlessScore(component->item, delta);

	int32_t results[CONFIG_FIXEDARRAY];
	static NetProto_GetRes gr;
	gr.Clear();
	PB_ItemInfo *item = gr.add_items();
	item->set_type(PB_ItemInfo::GOODS);
	item->set_count(1);
	item->set_id(7);
	Item_Lottery(component->item, 7, results, CONFIG_FIXEDARRAY, true ,&gr);

	return 0;
}

void PlayerPool_SendActiveAward(NetProto_Rank::Type type, bool flag) {
	if (type == NetProto_Rank::BLESSCOME) {
		static vector<RecordInfo> vecBless;
		static int64_t blessIndex;
		if (flag) {
			vecBless.clear();
			vecBless = pool.rankBlessingCome;
			pool.rankBlessingCome.clear();
			pool.rankBlessingComeInfo.clear();
			blessIndex = 0;
		}

		if (blessIndex < (int64_t)vecBless.size()) {
			int range = Config_AwardRange(blessIndex);
			const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::BLESS, range);
			assert(award != NULL);

			struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(vecBless[blessIndex].role().roleID());
			while (entity != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());

				struct Component *component = PlayerEntity_Component(entity);
				const ItemPackage *package = Item_Package(component->item);
				Item_ModifyBlessScore(component->item, -package->blessActive());

				blessIndex++;
				if (blessIndex >= (int64_t)vecBless.size()) {
					return;
				}

				range = Config_AwardRange(blessIndex);
				award = AwardInfoManager_AwardInfo(AwardInfo::BLESS, range);
				assert(award != NULL);
				entity = PlayerEntity_PlayerByRoleID(vecBless[blessIndex].role().roleID());
			}

			if (entity == NULL) {
				static DCProto_SendMail dsm;
				dsm.Clear();
				dsm.set_id(-4);
				dsm.mutable_sm()->set_receiver(vecBless[blessIndex].role().roleID());
				dsm.mutable_sm()->mutable_mail()->set_title(award->name());
				dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
				dsm.mutable_sm()->mutable_mail()->set_content(award->content());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
				GCAgent_SendProtoToDCAgent(&dsm);
				blessIndex++;
			}
		}
	} else if (type == NetProto_Rank::AWARD_FROM_SKY) {
		static vector<RecordInfo> rankLevel;
		static int64_t blessIndex;
		if (flag) {
			rankLevel.clear();
			rankLevel = pool.rankLevel;
			blessIndex = 0;
		}

		if (blessIndex < (int64_t)rankLevel.size()) {
			const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::AWARD_FROM_SKY_FINAL, 0);
			assert(award != NULL);

			struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(rankLevel[blessIndex].role().roleID());
			while (entity != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());

				blessIndex++;
				if (blessIndex >= (int64_t)rankLevel.size()) {
					return;
				}

				entity = PlayerEntity_PlayerByRoleID(rankLevel[blessIndex].role().roleID());
			}

			if (entity == NULL) {
				static DCProto_PingPongAward proto;
				proto.Clear();
				proto.set_type(type);
				proto.set_roleID(rankLevel[blessIndex].role().roleID());
				GCAgent_SendProtoToDCAgent(&proto);
				blessIndex++;
			}
		}
	}else if (type == NetProto_Rank::LUCK) {
		static vector<RecordInfo> vecLuck;
		static int64_t luckIndex;
		if (flag) {
			vecLuck.clear();
			vecLuck = pool.rankLuck;
			pool.rankLuck.clear();
			pool.rankLuckInfo.clear();
			luckIndex = 0;
		}

		if (luckIndex < (int64_t)vecLuck.size()) {
			int range = Config_AwardRange(luckIndex);
			const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::LUCK, range);
			assert(award != NULL);

			struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(vecLuck[luckIndex].role().roleID());
			while (entity != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());

				luckIndex++;
				if (luckIndex >= (int64_t)vecLuck.size()) {
					return;
				}

				range = Config_AwardRange(luckIndex);
				award = AwardInfoManager_AwardInfo(AwardInfo::LUCK, range);
				assert(award != NULL);
				entity = PlayerEntity_PlayerByRoleID(vecLuck[luckIndex].role().roleID());
			}

			if (entity == NULL) {
				static DCProto_SendMail dsm;
				dsm.Clear();
				dsm.set_id(-5);
				dsm.mutable_sm()->set_receiver(vecLuck[luckIndex].role().roleID());
				dsm.mutable_sm()->mutable_mail()->set_title(award->name());
				dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
				dsm.mutable_sm()->mutable_mail()->set_content(award->content());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
				GCAgent_SendProtoToDCAgent(&dsm);
				luckIndex++;
			}
		}
	}else if (type == NetProto_Rank::GROUP_PURCHASE) {
		for (int i = 0; i < 10 && pool.rankPurchase.size(); ++i) {
			const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::GROUPPURCHASE, i);
			assert(award != NULL);
			struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(pool.rankPurchase[i].role().roleID());
			if (entity != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
			}else {
				static DCProto_SendMail dsm;
				dsm.Clear();
				dsm.mutable_sm()->set_receiver(pool.rankPurchase[i].role().roleID());
				dsm.mutable_sm()->mutable_mail()->set_title(award->name());
				dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
				dsm.mutable_sm()->mutable_mail()->set_content(award->content());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
				GCAgent_SendProtoToDCAgent(&dsm);
			}
		}
	} else if (type == NetProto_Rank::CONSUME) {
		static vector<RecordInfo> rankConsume;
		static int index;
		if (flag) {
			rankConsume.clear();
			rankConsume = pool.rankConsume;
			index = 0;
		}

		if (index < (int)rankConsume.size()) {
			struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(rankConsume[index].role().roleID());

			const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::CONSUME, index);
			assert(award != NULL);

			while (entity != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());

				index++;
				if (index >= (int)rankConsume.size()) {
					return;
				}

				award = AwardInfoManager_AwardInfo(AwardInfo::CONSUME, index);
				assert(award != NULL);

				entity = PlayerEntity_PlayerByRoleID(rankConsume[index].role().roleID());
			}

			if (entity == NULL) {
				static DCProto_SendMail dsm;
				dsm.Clear();
				dsm.set_id(-6);
				dsm.mutable_sm()->set_receiver(rankConsume[index].role().roleID());
				dsm.mutable_sm()->mutable_mail()->set_title(award->name());
				dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
				dsm.mutable_sm()->mutable_mail()->set_content(award->content());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
				GCAgent_SendProtoToDCAgent(&dsm);
				index++;
			}
		}

	}
}

void PlayerPool_ServerOpenTime(NetProto_ServerOpenTime* info) {
	if (info != NULL) {
		info->set_serverOpenTime(Config_ServerStartTime());
	}
}

int PlayerPool_GMRankStatistics(NetProto_GMRankStatistics* info) {
	if (info == NULL) {
		return -1;
	}

	if (info->count() <= 0) {
		return -2;
	}

	static vector<RecordInfo>* vec;
	vec = NULL;
	vec = CurRank(info->type());

	if (vec == NULL) {
		return -3;
	}

	for (int i = 0; i < info->count() && i < (int)vec->size(); ++i) {
		*info->add_rank() = (*vec)[i];
	}
	return 0;
}

void PlayerPool_GMRoleOnlineCount() {
	static vector<struct PlayerEntity *> allPlayers;
	allPlayers.clear();
	int allCount = PlayerPool_Players(&allPlayers);

	RecordInfo record;
	record.set_arg1((::google::protobuf::int64)Time_TimeStamp());
	record.mutable_role()->set_roleID((::google::protobuf::int64)allCount);

	DCProto_SaveSingleRecord save;
	save.set_mapID(-100);
	*save.mutable_record() = record;
	GCAgent_SendProtoToDCAgent(&save);
}

const map<std::string, int64_t>* PlayerPool_GetFactionFightInfo() {
	return &pool.factionFight;
}

void PlayerPool_UpdateFactionFight(const char* str, int64_t delta) {
	if (str == NULL)
		return;
	pool.factionFight[str] += delta;
}

int64_t PlayerPool_GetFactionFight(const char* str) {
	map<string, int64_t>::iterator it = pool.factionFight.find(str);
	if (it == pool.factionFight.end()) {
		return 0;
	}
	return it->second;
}

void PlayerPool_ClearFactionFightInfo() {
	pool.factionFight.clear();
}

void PlayerPool_SaveFactionFightInfo(const char* name, int save, int64_t hurt) {
	if (name != NULL) {
		pool.winFactionName = string(name);
		pool.winSave = save;
		pool.winFactionHurt = hurt;

		RecordInfo record;
		record.set_arg1((::google::protobuf::int64)hurt);
		record.mutable_role()->set_roleID((::google::protobuf::int64)save);
		record.mutable_role()->set_name(name);

		DCProto_SaveSingleRecord saveRecord;
		saveRecord.set_mapID(-11);
		*saveRecord.mutable_record() = record;
		GCAgent_SendProtoToDCAgent(&saveRecord);
		return;
	}

	for (map<string, int64_t>::iterator it = pool.factionFight.begin(); it != pool.factionFight.end(); ++it) {
		RecordInfo record;
		record.set_arg1((::google::protobuf::int64)it->second);
		record.mutable_role()->set_roleID((::google::protobuf::int64)save);
		record.mutable_role()->set_name(it->first.c_str());

		DCProto_SaveSingleRecord saveRecord;
		saveRecord.set_mapID(-12);
		*saveRecord.mutable_record() = record;
		GCAgent_SendProtoToDCAgent(&saveRecord);
	}
	pool.curSave = save;
}

bool PlayerPool_GetWinFactionFight(int save, string *name,  int64_t *hurt) {
	if (pool.winSave != save || name == NULL || hurt == NULL) {
		return false;
	}
	*name = pool.winFactionName;
	*hurt = pool.winFactionHurt;
	return true;
}

void PlayerPool_SetWinFactionRecord(const RecordInfo *record) {
	if (record == NULL) {
		return;
	}

	pool.winSave = (int)record->role().roleID();
	pool.winFactionName = record->role().name();
	pool.winFactionHurt = record->arg1();
}

void PlayerPool_SetFactionRecords(const google::protobuf::RepeatedPtrField<RecordInfo> *Records) {
	if (Records == NULL) {
		return;
	}

	for (int i = 0; i < Records->size(); ++i) {
		if (Records->Get(i).role().roleID() != MapPool_FactionWarNum()) {
			continue;
		}
		pool.curSave = MapPool_FactionWarNum();
		pool.factionFight.insert(make_pair(Records->Get(i).role().name(), Records->Get(i).arg1()));
	} 
}

int PlayerPool_CurFactionFight() {
	return pool.curSave;
}

const AwardInfo* PlayerPool_GodAwardInfoByRoleID(int64_t roleID) {
	map<int64_t, int64_t>::iterator it = pool.rankGodInfo.find(roleID);
	if(it == pool.rankGodInfo.end())
		return NULL;
	int64_t index = it->second;
	int range = Config_AwardRange(index);
	return AwardInfoManager_AwardInfo(AwardInfo::DAILY_GOD, range);
}
/*
   static bool Sort_Pet(const RecordInfo& rs1, const RecordInfo& rs2) {
   if (rs1.arg1() < rs2.arg1()) {
   return false;
   }
   return true;
   }

   void PlayerPool_UpdatePetRank(const char* str, int power) {
   if (str == NULL) 
   return;

   if ((int)pool.rankPetInfo.size() == Config_RankCount() && (power < (int)pool.rankPetInfo[Config_RankCount() - 1].arg1())) {
   return;		
   }

   int64_t begin = 0;
   int end = 0;

   for (size_t sz = 0; sz < pool.rankPetInfo.size(); ++sz) {
   if (!strcmp(pool.rankPetInfo[sz].role().name().c_str(), str)) {
   RecordInfo info = pool.rankPetInfo[sz];
   info.set_arg1(power);

   end = sz;
   pool.rankPetInfo[sz] = info;
   sort(pool.rankPetInfo.begin(), pool.rankPetInfo.end(), Sort_Pet);

   for (size_t i = 0; i < pool.rankPetInfo.size(); ++i) {
   if (!strcmp(pool.rankPetInfo[i].role().name().c_str(), str)) {
   begin = i;
   }
   }

   info.mutable_role()->set_roleID((begin << 32) + end);

   DCProto_SaveSingleRecord save;
   save.set_mapID(-13);
 *save.mutable_record() = info;
 GCAgent_SendProtoToDCAgent(&save);
 return;
 }
 }

 end = pool.rankPetInfo.size() - 1;
 if (end < Config_RankCount() - 1) {
 ++end;
 RecordInfo info;
 info.mutable_role()->set_name(str);
 info.set_arg1(power);
 pool.rankPetInfo.push_back(info);
 end = pool.rankPetInfo.size();

 sort(pool.rankPetInfo.begin(), pool.rankPetInfo.end(), Sort_Pet);

 for (size_t i = 0; i < pool.rankPetInfo.size(); ++i) {
 if (!strcmp(pool.rankPetInfo[i].role().name().c_str(), str)) {
 begin = i;
 }
 }

 info.mutable_role()->set_roleID((begin << 32) + end);

 DCProto_SaveSingleRecord save;
 save.set_mapID(-13);
 *save.mutable_record() = info;
 GCAgent_SendProtoToDCAgent(&save);
 return;
 }else {
 if (power > (int)pool.rankPetInfo[end].arg1()) {
 RecordInfo info;
info.mutable_role()->set_name(str);
info.set_arg1(power);
pool.rankPetInfo[end] = info;

sort(pool.rankPetInfo.begin(), pool.rankPetInfo.end(), Sort_Pet);

for (size_t i = 0; i < pool.rankPetInfo.size(); ++i) {
	if (!strcmp(pool.rankPetInfo[i].role().name().c_str(), str)) {
		begin = i;
	}
}

info.mutable_role()->set_roleID((begin << 32) + end);

DCProto_SaveSingleRecord save;
save.set_mapID(-13);
*save.mutable_record() = info;
GCAgent_SendProtoToDCAgent(&save);
return;
}
}
}
*/

void PlayerPool_InitDevilRank(DCProto_QueryRoleFaction* info) {
	if (info == NULL)
		return;

	for (int i = 0; i < info->roleID_size(); ++i) {
		map<int64_t, RecordInfo>::iterator it = pool.rankDevilInfo.find(info->roleID(i));
		if (it != pool.rankDevilInfo.end()){
			string name = it->second.role().name() + "," + info->faction(i);
			it->second.mutable_role()->set_name(name.c_str());
		}
	}
	PlayerPool_SendDevilAward();
	pool.isRankDevil = true;
}

static bool Sort_DevilRank(const RecordInfo& rsh1, const RecordInfo& rsh2) {
	int count1 = rsh1.arg1() >> 32;
	int count2 = rsh2.arg1() >> 32;
	int time1 = rsh1.arg1() & 0xFFFFFFFF;
	int time2 = rsh2.arg1() & 0xFFFFFFFF;

	if (count1 > count2) {
		return true;
	}else if (count1 == count2) {
		if (time1 < time2) {
			return true;
		}
	}
	return false;
}

void PlayerPool_SendDevilAward(bool send) {
	pool.vecDevilRank.clear();
	for (map<int64_t, RecordInfo>::iterator it = pool.rankDevilInfo.begin(); it != pool.rankDevilInfo.end(); ++it) {
		pool.vecDevilRank.push_back(it->second);
	}
	sort(pool.vecDevilRank.begin(), pool.vecDevilRank.end(), Sort_DevilRank);

	if (send) {
		int index = 0;
		for (vector<RecordInfo>::iterator it = pool.vecDevilRank.begin(); it != pool.vecDevilRank.end() && index < 10; ++it, ++index) {
			const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::DEVIL, index);
			if (award == NULL) {
				continue;
			}

			struct PlayerEntity* entity = PlayerEntity_PlayerByRoleID(it->role().roleID());
			if (entity != NULL) {
				PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
			}else {
				static DCProto_SendMail dsm;
				dsm.Clear();
				dsm.mutable_sm()->set_receiver(it->role().roleID());
				dsm.mutable_sm()->mutable_mail()->set_title(award->name());
				dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
				dsm.mutable_sm()->mutable_mail()->set_content(award->content());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
				dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
				GCAgent_SendProtoToDCAgent(&dsm);
			}
		}
	}
}

void PlayerPool_FactionPower(DCProto_FactionPower* info) {
	if (info == NULL) 
		return;

	for (int i = 0; i < (int)info->info_size(); ++i) {
		for (int j = 0; j < (int)pool.rankFactionInfo.size(); ++j) {
			if (info->info(i).str().compare(pool.rankFactionInfo[j].role().name()) == 0) {
				pool.rankFactionInfo[j].mutable_role()->set_roleID(info->info(i).power());
				break;
			}
		}	
	}

	pool.isRankFaction = true;
}

static bool Sort_ConsumeRank(const RecordInfo& rsh1, const RecordInfo& rsh2) {
	if (rsh1.arg1() < rsh2.arg1())
		return false;
	return true;
}

void PlayerPool_RankConsumeAdjust(int64_t roleID, int rmb) {
	int index = -1;
	const static int size = 10; 
	if ((int)pool.rankConsume.size() < size) {
		index = -2;
	}else {
		if (pool.rankConsume[size - 1].arg1() < rmb) {
			index = -3;
		}
	}
	if (index == -1)
		return;

	for (int i = 0; i < (int)pool.rankConsume.size(); ++i) {
		if (pool.rankConsume[i].role().roleID() == roleID) {
			index = i;
			break;
		}
	}

	RecordInfo record;
	record.mutable_role()->set_roleID((::google::protobuf::int64)roleID);
	struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	if (entity == NULL)
		return;

	const PlayerAtt* att = PlayerEntity_Att(entity);
	if (att == NULL)
		return;

	record.mutable_role()->set_name(att->att().baseAtt().name());
	record.mutable_role()->set_professionType((PB_ProfessionInfo::Type)att->att().baseAtt().professionType());

	if (index == -2) {
		record.set_arg1(rmb);
		pool.rankConsume.push_back(record);
		record.set_arg1(-1);
	}else if (index == -3) {
		record.set_arg1(rmb);
		pool.rankConsume[size - 1] = record;
		record.set_arg1(pool.rankConsume[size - 1].role().roleID());
	}else {
		if (index < size) {
			record.set_arg1(rmb);
			pool.rankConsume[index] = record;
			record.set_arg1(pool.rankConsume[index].role().roleID());
		}else {
			return;
		}
	}

	sort(pool.rankConsume.begin(), pool.rankConsume.end(), Sort_ConsumeRank);

	char ch[128];
	memset(ch, 0, sizeof(ch) / sizeof(char));
	SNPRINTF1(ch, "%s,%d", att->att().baseAtt().name(), rmb);
	record.mutable_role()->set_name(ch);

	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-15);
	*saveRecord.mutable_record() = record;
	GCAgent_SendProtoToDCAgent(&saveRecord);
}

void PlayerPool_AddRekooRole(int64_t roleID) {
	pool.rekooRole.insert(roleID);
}

bool PlayerPool_IsInRekooRole(int64_t roleID) {
	set<int64_t>::iterator it = pool.rekooRole.find(roleID);
	if (it != pool.rekooRole.end()) {
		return true;
	}
	return false;
}

int PlayerPool_AddInvateCode(int64_t roleID, NetProto_InvateCode * info) {
	if (info == NULL)
		return -1;

	map<int64_t, pair<string, string> >::iterator iter = pool.mapInviteCode.find(roleID);
	if (iter == pool.mapInviteCode.end())
		return -2;

	if (iter->second.first == "") {
		string str = PlayerEntity_SelfInviteCode(roleID);
		pool.mapCodeInfo[str] = roleID;
		iter->second.first = str;
	}

	if (iter->second.second != "") 
		return -3;

	map<string, int64_t>::iterator it = pool.mapCodeInfo.find(info->othercode());
	if (it == pool.mapCodeInfo.end())
		return -4;

	if (it->second == roleID)
		return -5;


	iter->second.second = info->othercode();

	PlayerEntity_OtherInviteCode(roleID, info->othercode().c_str(), it->second);

	return 0;
}

void PlayerPool_SysInvateCodeInfo(int64_t roleID, const char * str) {
	pool.mapCodeInfo[str] = roleID;
	struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	PlayerEntity_SysOnlineInvateCode(entity, str);

	map<int64_t, pair<string, string> >::iterator iter = pool.mapInviteCode.find(roleID);
	if (iter != pool.mapInviteCode.end()) {
		iter->second.first = str;
	}else {
		pool.mapInviteCode[roleID] = make_pair(str, "");
	}

}

void PlayerPool_InitInviteCode(DCProto_LoadInviteCode * info) {
	if (info == NULL)
		return;

	pool.mapInviteCode.clear();
	pool.mapCodeInfo.clear();

	for (int i = 0; i < info->info_size(); ++i) {
		pool.mapInviteCode[info->info(i).roleID()] = make_pair(info->info(i).selfCode(), info->info(i).otherCode());
		if (info->info(i).selfCode() != "") {
			pool.mapCodeInfo[info->info(i).selfCode()] = info->info(i).roleID();
		}
	}
}

void PlayerPool_AddInviteCode(const char * str) {
	if (str == NULL)
		return;

	map<string, int64_t>::iterator it = pool.mapCodeInfo.find(str);
	if (it != pool.mapCodeInfo.end()) {
		PlayerEntity_AddInviteCode(it->second);
	}
}

void PlayerPool_GMSetMaxRoleCountInfo(NetProto_GMLoginInfo * info) {
	if (info == NULL)
		return;

	pool.maxRoleCount = info->count();
	pool.maxRoleCountInfo = info->str();
}

bool PlayerPool_RoleFull(int32_t id) {
	if (id < 0) {
		if (pool.maxRoleCount == 0) 
			return false;

		if ( PlayerPool_TotalCount() < pool.maxRoleCount ) {
			return false;
		}
		return true;
	} else {
		static NetProto_Error error;
		error.set_content(pool.maxRoleCountInfo);
		GCAgent_SendProtoToClients(&id, 1, &error);
		return true;
	}
}

void PlayerPool_GrabRedEnvelope(int64_t roleID, int count) {
	struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	if (entity == NULL)
		return;

	const PlayerAtt* att = PlayerEntity_Att(entity);
	if (att == NULL)
		return;

	pool.giftCount = count;
	pool.maxGiftInGiftCount = count / 100;
	pool.giftRoleID.clear();

	static vector<struct PlayerEntity *> allPlayers;
	allPlayers.clear();
		
	char ch[1024];
	SNPRINTF1(ch, Config_Words(95), att->att().baseAtt().name(), count);

	static NetProto_GrabRedEnvelope proto;
	proto.Clear();
	proto.set_str(ch);

	int allCount = PlayerPool_Players(&allPlayers);
	for (int i = 0; i < allCount; i++) {
		int32_t id = PlayerEntity_ID(allPlayers[i]);
		GCAgent_SendProtoToClients(&id, 1, &proto);
	}
	
	static NetProto_Chat chat;
	chat.Clear();
	chat.set_channel(NetProto_Chat::SYSTEM);
	chat.set_content(ch);
	GCAgent_SendProtoToAllClients(&chat);
	
	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL)
		return;
	
	static NetProto_GetRes gr;
	gr.Clear();
	Item_AddToPackage(component->item, ItemInfo::GOODS, Config_GrabRedEnvelope(), count / 10, &gr);

	char ck[1024];
	SNPRINTF1(ck, "31-%d-%d", Config_GrabRedEnvelope(), count / 10);
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ck);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);
	int32_t id = PlayerEntity_ID(entity);
	GCAgent_SendProtoToClients(&id, 1, &gr);
}

void PlayerPool_GrabRedEnvelope(int64_t roleID) {
	struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	if (entity == NULL)
		return;
	
	int32_t id = PlayerEntity_ID(entity);

	if (pool.giftCount < 1) {
		static NetProto_Error error;
		error.set_content(Config_Words(97));
		GCAgent_SendProtoToClients(&id, 1, &error);
		return;
	} 

	if (pool.giftRoleID.find(roleID) != pool.giftRoleID.end()) {
		static NetProto_Error error;
		error.set_content(Config_Words(96));
		GCAgent_SendProtoToClients(&id, 1, &error);
		return;
	}

	int count = Time_Random(0, pool.maxGiftInGiftCount);
	count++;

	if (pool.giftCount < count) 
		count = pool.giftCount;

	pool.giftCount -= count;


	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL)
		return;

	pool.giftRoleID.insert(roleID);
	static NetProto_GetRes gr;
	gr.Clear();
	Item_AddToPackage(component->item, ItemInfo::GOODS, Config_GrabRedEnvelope(), count, &gr);

	char ch[1024];
	SNPRINTF1(ch, "32-%d-%d", Config_GrabRedEnvelope(), count);
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ch);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);
	GCAgent_SendProtoToClients(&id, 1, &gr);
}

static void CatGift(int64_t roleID, NetProto_GetRes * gr) {
	int count = 0;
	pool.catGiftCount++;

	if (pool.catGiftCount % 10 == 9) {
		count++;
		if (pool.catGiftCount % 100 == 99) {
			count++;
			if (pool.catGiftCount % 1000 == 999) {
				count++;
				if (pool.catGiftCount % 10000 == 9999) {
					count++;
				}
			}
		}
	}
	
	const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::CATGIFT, count);
	if (award == NULL)
		return;

	struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
	if (player == NULL)
		return;
	struct Component *component = PlayerEntity_Component(player);
	if (component == NULL)
		return;

	int32_t res[CONFIG_FIXEDARRAY];
	Item_Lottery(component->item, award->award(), res, CONFIG_FIXEDARRAY, false, gr);

	pool.catGiftCount = pool.catGiftCount % 10000;

	if (count == 3 || count == 4) {
		char ch[1024];
		static NetProto_Message message;
		message.Clear();
		message.set_content(ch);
		message.set_count(1);
		GCAgent_SendProtoToAllClients(&message);
	}
}

void PlayerPool_CatGift(int64_t roleID, NetProto_CatGift * info) {
	if (!Config_InActiveTime(27)) {
		pool.catGiftEndTime = 0;
		pool.catGiftCount = 0;
		return;
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-27);
	if (begin == fixedMap->end()) {
		return;
	}

	if ( pool.catGiftEndTime < (begin->second & 0xFFFFFFFF) ) {
		pool.catGiftCount = 0;
		pool.catGiftEndTime = begin->second & 0xFFFFFFFF;
	}

	struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
	if (player == NULL)
		return;
	int32_t id = PlayerEntity_ID(player);
	if (info->count() == 0) {
		info->set_allCount(pool.catGiftCount);
		GCAgent_SendProtoToClients(&id, 1, info);
		return;
	}

	if (info->count() != 1 && info->count() != 10) {
		return;
	}

	int rmb = Config_CatGift(info->count());
	if (rmb == 0)
		return;

	struct Component *component = PlayerEntity_Component(player);
	if (component == NULL)
		return;
	
	if (!Item_HasRMB(component->item, rmb, 1)) {
		return;
	}

	static NetProto_GetRes gr;
	gr.Clear();
	for (int i = 0; i < info->count(); ++i) {
		CatGift(roleID, &gr);
	}
	GCAgent_SendProtoToClients(&id, 1, &gr);
		
	info->set_allCount(pool.catGiftCount);
	GCAgent_SendProtoToClients(&id, 1, info);
			
	Item_ModifyRMB(component->item, -rmb, true, 33, 0, 0);
}

static int GroupPurchase() {
	if (pool.groupPurchase >= 1000) {
		return 9;
	}
	if (pool.groupPurchase >= 10000) {
		return 8;
	}
	if (pool.groupPurchase >= 30000) {
		return 7;
	}
	if (pool.groupPurchase >= 50000) {
		return 6;
	}
	return 10;
}

void SendGroupPurchaseMail(int64_t roleID) {
	struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
	if (player == NULL)
		return;

	struct Component *component = PlayerEntity_Component(player);
	if (component == NULL)
		return;
	
	int price = GroupPurchase();
	int subRMB = component->playerAtt->fixedEvent(103) * (10 - price) 
		+ component->playerAtt->fixedEvent(104) * (9 - price) 
		+ component->playerAtt->fixedEvent(105) * (8 - price) 
		+ component->playerAtt->fixedEvent(106) * (7 - price);


	if (subRMB != 0) {
		PlayerEntity_AddMail(player, ItemInfo::NONE, 0, 0, Config_Words(0), Config_Words(99), subRMB);
	}

	PlayerEntity_SetFixedEvent(player, 103, 0);
	PlayerEntity_SetFixedEvent(player, 104, 0);
	PlayerEntity_SetFixedEvent(player, 105, 0);
	PlayerEntity_SetFixedEvent(player, 106, 0);
	PlayerEntity_SetFixedEvent(player, 107, 0);
	PlayerEntity_SetFixedEvent(player, 109, 0);
}

void PlayerPool_GroupPurchase(int64_t roleID, NetProto_GroupPurchase * info) {
	struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
	if (player == NULL)
		return;

	struct Component *component = PlayerEntity_Component(player);
	if (component == NULL)
		return;
	if (info == NULL) {
		if (!Config_InActiveTime(29)) {
			SendGroupPurchaseMail(roleID);
		}
		return;
	}

	if (!Config_InActiveTime(29)) {
		return;
	}
		
	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-29);
	if (begin == fixedMap->end()) {
		return;
	}

	if (pool.groupPurchaseEndTime != (begin->second & 0xFFFFFFFF)) {
		pool.groupPurchaseEndTime = (begin->second & 0xFFFFFFFF);
		pool.groupPurchase = 0;
	}

	if (component->playerAtt->fixedEvent(107) != (begin->second & 0xFFFFFFFF)) {
		SendGroupPurchaseMail(roleID);
	}

	int32_t id = PlayerEntity_ID(player);
	if (info->allCount() == 0) {
		info->set_allCount(pool.groupPurchase);
		GCAgent_SendProtoToClients(&id, 1, info);
		return;
	}	
	int price = GroupPurchase();
	if (!Item_HasRMB(component->item, price, 1)) {
		return;
	}
	
	int32_t res[CONFIG_FIXEDARRAY];
	Item_Lottery(component->item, 134, res, CONFIG_FIXEDARRAY, true , NULL);
	pool.groupPurchase++;
	info->set_allCount(pool.groupPurchase);
	GCAgent_SendProtoToClients(&id, 1, info);
	
	Item_ModifyRMB(component->item, -price, true, 34, 0, 0);

	if (price == 10)		
		PlayerEntity_SetFixedEvent(player, 103, component->playerAtt->fixedEvent(103) + 1); 
	if (price == 9)		
		PlayerEntity_SetFixedEvent(player, 104, component->playerAtt->fixedEvent(104) + 1); 
	if (price == 8)		
		PlayerEntity_SetFixedEvent(player, 105, component->playerAtt->fixedEvent(105) + 1); 
	if (price == 7)		
		PlayerEntity_SetFixedEvent(player, 106, component->playerAtt->fixedEvent(106) + 1); 
		
	PlayerEntity_SetFixedEvent(player, 109, component->playerAtt->fixedEvent(109) + price); 
	PlayerEntity_SetFixedEvent(player, 107, begin->second & 0xFFFFFFFF); 
	pool.groupPurchaseEndTime = begin->second & 0xFFFFFFFF;
	return;
}

/*
static int ConfigTime(int id, int index) {
	if (id < 0 || id > 19) 
		return -1;

	if (index < 0 || index > 11) 
		return -2;

	PB_ReservationTime times = pool.all.times(id);
	
	switch (index) {
		case 0:
			return times.appointmentBeginHour();
		case 1:
			return times.appointmentBeginMinute();
		case 2:
			return times.appointmentEndHour();
		case 3:
			return times.appointmenEndMinute();
		case 4:
			return times.challengeEndHour();
		case 5:
			return times.challengeEndMinute();
		case 6:
			return times.enterBeginHour();
		case 7:
			return times.enterBeginMinute();
		case 8:
			return times.fightBeginHour();
		case 9:
			return times.fightBeginMinute();
		case 10:
			return times.fightEndHour();
		case 11:
			return times.fightEndMinute();
		default:
			return -3;
	}
	return -4;
}
*/

static void Negotiation(struct PlayerEntity * entity, NetProto_Reservation * info, int type) {
	if (entity == NULL || info == NULL) 
		return;
	
	const PlayerAtt* att = PlayerEntity_Att(entity);
	if ( att == NULL )
		return;
	
	int64_t roleID = PlayerEntity_RoleID(entity);
	int index = info->time();
	int begin = index < 8 ? 0 : 8;
	int end = index < 8 ? 8 : 20;
	
	if (type == 1 || type == 2) {
		if ( pool.reservation[index].roleID1 != -100 ) {
			return;
		}

		for (int i = begin; i < end; ++i) {
			if ( pool.reservation[i].roleID1 == roleID ) {
				return;
			}
		}	
		
		int rmb =  Config_ReservationCost(info->rmb(), true);
		struct Component *component = PlayerEntity_Component(entity); 
		if (component == NULL) 
			return;

		if (!Item_HasRMB(component->item, rmb, 1)) {
			return;
		}
		Item_ModifyRMB(component->item, -rmb, true, 36, 0, 0);
	
		pool.reservation[index].roleID1 = att->att().baseAtt().roleID();
		pool.reservation[index].name1 = att->att().baseAtt().name();
		pool.reservation[index].type1 = (PB_ProfessionInfo::Type)att->att().baseAtt().professionType();
		pool.reservation[index].rmb = info->rmb();

		if (type == 1) {
			struct PlayerEntity *entity2 = PlayerEntity_PlayerByRoleID(info->roleID());
			if (entity2 != NULL) {
				const PlayerAtt* att2 = PlayerEntity_Att(entity2);
				if ( att2 == NULL )
					return;
				pool.reservation[index].powerType = type;
				pool.reservation[index].roleID2 = info->roleID(); // nedd name and profession
				pool.reservation[index].name2 = att2->att().baseAtt().name();
				pool.reservation[index].type2 = (PB_ProfessionInfo::Type)att2->att().baseAtt().professionType();
			}else {
				pool.reservation[index].powerType = type;
			}
			return;
		}else if (type == 2) {
			pool.reservation[index].powerType = type;
			return;
		}else {
			return;
		}
	} else if (type == 3 || type == 4) {
		if ( pool.reservation[index].roleID1 == -100 ) {
			return;
		}
		
		if (type == 3) {
			if ( pool.reservation[index].roleID2 != roleID ) {
				return;
			}

			if (pool.reservation[index].powerType != 1) {
				return;
			}
		}else if (type == 4) {
			if ( pool.reservation[index].roleID2 != -100 ) {
				return;
			}

			if (pool.reservation[index].powerType != 2) {
				return;
			}
			
			for (int i = begin; i < end; ++i) {
				if ( pool.reservation[i].roleID2 == roleID ) {
					if ( (pool.reservation[i].powerType / 10) % 10 == 2 ) {
						return;
					}
				}
			}
		}else {
			return;
		}

		int rmb =  Config_ReservationCost(pool.reservation[index].rmb, true);
		struct Component *component = PlayerEntity_Component(entity);
		if (component == NULL) 
			return;

		if (!Item_HasRMB(component->item, rmb, 1)) {
			return;
		}
		Item_ModifyRMB(component->item, -rmb, true, 36, 0, 0);
		
		pool.reservation[index].roleID2 = att->att().baseAtt().roleID();
		pool.reservation[index].name2 = att->att().baseAtt().name();
		pool.reservation[index].type2 = (PB_ProfessionInfo::Type)att->att().baseAtt().professionType();
		if (type == 3) {
			pool.reservation[index].powerType = 31;
		}else if (type == 4) {
			pool.reservation[index].powerType = 21;
		}else {
			return;
		}
	} else if (type == 5) {
		if ( pool.reservation[index].roleID1 == -100 ) {
			return;
		}
		if ( pool.reservation[index].roleID2 == -100 ) {
			return;
		}
		if ( pool.reservation[index].powerType < 10 ) {
			return;
		}
		if ( pool.reservation[index].setRole1.find(roleID) != pool.reservation[index].setRole1.end() ) {
			return;
		}
		if ( pool.reservation[index].setRole2.find(roleID) != pool.reservation[index].setRole2.end() ) {
			return;
		}

		int rmb =  Config_ReservationCost(50, false);
		struct Component *component = PlayerEntity_Component(entity);
		if (component == NULL) 
			return;

		if (!Item_HasRMB(component->item, rmb, 1)) {
			return;
		}
		Item_ModifyRMB(component->item, -rmb, true, 36, 0, 0);

		if (info->rmb() == 0) {
			pool.reservation[index].setRole1.insert(roleID);
		}else {
			pool.reservation[index].setRole2.insert(roleID);
		}
	}

}

int PlayerPool_Reservation(struct PlayerEntity * entity, NetProto_Reservation * info) {
	if (entity == NULL || info == NULL) 
		return -1;
	
	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL) 
		return -2;

	if (info->time() < 0 || info->time() > 19) {
		return -3;
	}

	int hour = 0;
	int min = 0;
	Config_Reservation(Time_TimeStamp(), NULL, NULL, NULL, &hour, &min, NULL);
	if (hour == 0 && min == 0) 
		return -4;
	
	if (info->roleID() < 0) {
		if (info->roleID() == -1) {
			if (!Config_Reservation(info->roleID(), info->rmb())) {
				return -10;
			}
			
			hour = ConfigTime(info->time(), 0);
			min = ConfigTime(info->time(), 1);
			if (hour == -1 || min == -1)
				return -11;

			int begin = Config_ReservationTime(hour, min);

			hour = ConfigTime(info->time(), 2);
			min = ConfigTime(info->time(), 3);
			if (hour == -1 || min == -1)
				return -12;

			int end = Config_ReservationTime(hour, min);
			if (Time_TimeStamp() >= begin && Time_TimeStamp() <= end) {
				Negotiation(entity, info, 2);
				return 0;
			}
			return -13;
			/*			
			if (hour >= 0 && hour < 11) {
				if (info->time() >= 0 && info->time() < 8) {
					Negotiation(entity, info, 2);
					return 0;
				}
				return -11;
			}else if (hour >= 14 && hour < 18) {
				if (info->time() >= 8 && info->time() < 20) {
					Negotiation(entity, info, 2);
					return 0;
				}
				return -12;
			}
			*/
		}else if (info->roleID() == -2) {
			hour = ConfigTime(info->time(), 0);
			min = ConfigTime(info->time(), 1);
			if (hour == -1 || min == -1)
				return -11;

			int begin = Config_ReservationTime(hour, min);

			hour = ConfigTime(info->time(), 2);
			min = ConfigTime(info->time(), 3);
			if (hour == -1 || min == -1)
				return -12;

			int end = Config_ReservationTime(hour, min);
			if (Time_TimeStamp() >= begin && Time_TimeStamp() <= end) {
				Negotiation(entity, info, 3);
				return 0;
			}
			return -13;
		}else if (info->roleID() == -3) {
			hour = ConfigTime(info->time(), 2);
			min = ConfigTime(info->time(), 3);
			if (hour == -1 || min == -1)
				return -111;

			int begin = Config_ReservationTime(hour, min);

			hour = ConfigTime(info->time(), 4);
			min = ConfigTime(info->time(), 5);
			if (hour == -1 || min == -1)
				return -121;

			int end = Config_ReservationTime(hour, min);
			if (Time_TimeStamp() >= begin && Time_TimeStamp() <= end) {
				Negotiation(entity, info, 4);
				return 0;
			}
			return -131;
		}else if (info->roleID() == -4) {
			hour = ConfigTime(info->time(), 0);
			min = ConfigTime(info->time(), 1);
			if (hour == -1 || min == -1)
				return -112;

			int begin = Config_ReservationTime(hour, min);

			hour = ConfigTime(info->time(), 4);
			min = ConfigTime(info->time(), 5);
			if (hour == -1 || min == -1)
				return -122;

			int end = Config_ReservationTime(hour, min);
			if (Time_TimeStamp() >= begin && Time_TimeStamp() <= end) {
				Negotiation(entity, info, 5);
				return 0;
			}
			return -132;
		}
		
		/*		
		else if (info->roleID() == -2 || info->roleID() == -3 || info->roleID() == -4) {
			if (!Config_Reservation(info->roleID(), info->rmb())) {
				return -20;
			}
			
			
			if (hour >= 0 && hour < 11) {
				if (info->time() >= 0 && info->time() < 8) {
					if (info->roleID() == -2) {
						Negotiation(entity, info, 3);
					}else if (info->roleID() == -4) {
						Negotiation(entity, info, 5);
					}else {
						return -21;
					}
					return 0;
				}
				return -22;
			}else if (hour >= 11 && hour < 12) {
				if (info->time() >= 0 && info->time() < 8) {
					if (info->roleID() == -3) {
						Negotiation(entity, info, 4);
					}else if (info->roleID() == -4) {
						Negotiation(entity, info, 5);
					}else {
						return -23;
					}
					return 0;
				}
				return -22;
			}else if (hour >=12 && hour < 14) {
				return -23;
			}else if (hour >= 14 && hour < 18) {
				if (info->time() >= 8 && info->time() < 20) {
					if (info->roleID() == -2) {
						Negotiation(entity, info, 3);
					}else if (info->roleID() == -4) {
						Negotiation(entity, info, 5);
					}else {
						return -24;
					}
					return 0;
				}
				return -25;
			}else if (hour >= 18 && hour < 19) {
				if (info->time() >= 8 && info->time() < 20) {
					if (info->roleID() == -3) {
						Negotiation(entity, info, 4);
					}else if (info->roleID() == -4) {
						Negotiation(entity, info, 5);
					}else {
						return -26;
					}
					return 0;
				}
				return -27;
			}else {
				return -28;
			}

		}
	*/
	}else {
		if (!Config_Reservation(info->roleID(), info->rmb())) {
			return -22;
		}
		
		if (info->roleID() >= pool.cur) {
			return -23;
		}
	
		hour = ConfigTime(info->time(), 0);
		min = ConfigTime(info->time(), 1);
		if (hour == -1 || min == -1)
			return -24;

		int begin = Config_ReservationTime(hour, min);

		hour = ConfigTime(info->time(), 2);
		min = ConfigTime(info->time(), 3);
		if (hour == -1 || min == -1)
			return -25;

		int end = Config_ReservationTime(hour, min);
		DEBUG_LOG("MMMMMMMMMMMMMMMMM %d, %d, %d", begin, end, Time_TimeStamp());
		if (Time_TimeStamp() >= begin && Time_TimeStamp() <= end) {
			Negotiation(entity, info, 1);
			return 0;
		}
		return -26;
	/*
		if ((hour >= 0 && hour < 11)) {
			if (info->time() >= 0 && info->time() < 8) {
				Negotiation(entity, info, 1);
				return 0;
			}
			return -24;
		}else if (hour >= 14 && hour < 18) {
			if (info->time() >= 8 && info->time() < 20) {
				Negotiation(entity, info, 1);
				return 0;
			}
			return -25;
		}
		return -26;
	*/

	}
	return -30;
}

void PlayerPool_ReservationRefresh(int index, int64_t roleID) {
	if (index == -1) {
		Reservation tmp;
		for (int i = 0; i < 20; ++i) {
			pool.reservation[i] = tmp;
		}
		return;
	}else if (index == -2) {
		for (int i = 0; i < 8; ++i) {
			if (pool.reservation[i].roleID1 != -100) {
				if (pool.reservation[i].powerType == 1) {
					pool.reservation[i].powerType = 2;
					pool.reservation[i].roleID2 = -100;
				}
			}
		}
		return;
	}else if (index == -3) {
		for (int i = 8; i < 20; ++i) {
			if (pool.reservation[i].roleID1 != -100) {
				if (pool.reservation[i].powerType == 1) {
					pool.reservation[i].powerType = 2;
					pool.reservation[i].roleID2 = -100;
				}
			}
		}
		return;
	}else if (index == -4) {
		if (roleID == -1) 
			return;

		int hour = 0;
		int min = 0;
		for (int i = 0; i < 20; ++i) {
			hour = ConfigTime(i, 8);
			min = ConfigTime(i, 9);
			if (hour == -1 || min == -1)
				return;

			int begin = Config_ReservationTime(hour, min);

			hour = ConfigTime(i, 10);
			min = ConfigTime(i, 11);
			if (hour == -1 || min == -1)
				return;

			int end = Config_ReservationTime(hour, min);
			if (Time_TimeStamp() >= begin && Time_TimeStamp() <= end) {
				if ( pool.reservation[i].roleID1 == roleID || pool.reservation[i].roleID2 == roleID ) {
					if ( pool.reservation[i].powerType == 21 || pool.reservation[i].powerType == 31 ) {
						if ( pool.reservation[i].roleID1 == roleID ) {
							pool.reservation[i].powerType += 100;
						}else {
							pool.reservation[i].powerType += 200;
						}
					}
				}
			}
		}
		/*
		int hour = 0;
		int min = 0;
		int index = -1;
		Config_Reservation(Time_TimeStamp(), NULL, NULL, NULL, &hour, &min, NULL);
		
		if (hour == 12 || hour == 13 || hour == 19 || hour == 20 || hour == 21) {
			if ( (min % 15) >= 5 && (min % 15) < 10 ) {
				if (hour == 12)
					index = 0;
				if (hour == 13)
					index = 4;
				if (hour == 19)
					index = 8;
				if (hour == 20)
					index = 12;
				if (hour == 21)
					index = 16;

				index += min / 15;
				
				if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID ) {
					if ( pool.reservation[index].powerType == 21 || pool.reservation[index].powerType == 31 ) {
						if ( pool.reservation[index].roleID1 == roleID ) {
							pool.reservation[index].powerType += 100;
						}else record->role().name().str(), {
							pool.reservation[index].powerType += 200;
						}
					}
				}

			}
		}
		*/
		return;	
	}else if (index >= 0) {
		int res = ( pool.reservation[index].powerType % 1000 ) / 100;
		if ( res != 1 || res != 2 ) {
			int64_t roleID1 = pool.reservation[index].roleID1;
			int64_t roleID2 = pool.reservation[index].roleID2;
			struct PlayerEntity *entity1 = PlayerEntity_PlayerByRoleID(roleID1);
			struct PlayerEntity *entity2 = PlayerEntity_PlayerByRoleID(roleID2);

			if (entity1 != NULL && entity2 != NULL) {
				const PlayerAtt* att1 = PlayerEntity_Att(entity1);
				const PlayerAtt* att2 = PlayerEntity_Att(entity2);
				if (att1 != NULL && att2 != NULL) {
					if ( att1->att().movementAtt().mapID() == PlayerPool_ReservationMap() &&  att2->att().movementAtt().mapID() == PlayerPool_ReservationMap() ) {
						if (att1->att().fightAtt().hp() > att2->att().fightAtt().hp()) {
							pool.reservation[index].powerType += 100;
						}else {
							pool.reservation[index].powerType += 100;
						}
					}else if ( att1->att().movementAtt().mapID() == PlayerPool_ReservationMap() &&  att2->att().movementAtt().mapID() != PlayerPool_ReservationMap() ) {
						pool.reservation[index].powerType += 100;
					}else if ( att1->att().movementAtt().mapID() != PlayerPool_ReservationMap() &&  att2->att().movementAtt().mapID() == PlayerPool_ReservationMap() ) {
						pool.reservation[index].powerType += 200;
					}
				}	
			}else if (entity1 != NULL && entity2 == NULL) {
				const PlayerAtt* att1 = PlayerEntity_Att(entity1);
				if (att1 != NULL) {
					if ( att1->att().movementAtt().mapID() == PlayerPool_ReservationMap() ) {
						pool.reservation[index].powerType += 100;
					}
				}	
			}else if (entity1 == NULL && entity2 != NULL) {
				const PlayerAtt* att2 = PlayerEntity_Att(entity2);
				if (att2 != NULL) {
					if ( att2->att().movementAtt().mapID() == PlayerPool_ReservationMap() ) {
						pool.reservation[index].powerType += 200;
					}
				}	
			}

		}
	//------------------------------	
	// TODO:
	//---------------------------------
	}

//	Reservation():roleID1(-100), roleID2(-100), name1(""), name2(""), rmb(0), type1(PB_ProfessionInfo::NPC), type2(PB_ProfessionInfo::NPC), powerType(0) {setRole1.clear(); setRole2.clear();}

		

}

bool PlayerPool_ReservationEnterOrPower(int64_t roleID, int op) {
	/*
	int hour = 0;
	int min = 0;
	int index = -1;
	Config_Reservation(Time_TimeStamp(), NULL, NULL, NULL, &hour, &min, NULL);
	*/

	int hour = 0;
	int min = 0;
	for (int index = 0; index < 20; ++index) {
		hour = ConfigTime(index, 6);
		min = ConfigTime(index, 7);
		if (hour == -1 || min == -1)
			continue;

		int begin = Config_ReservationTime(hour, min);

		hour = ConfigTime(index, 8);
		min = ConfigTime(index, 9);
		if (hour == -1 || min == -1)
			continue;

		int end = Config_ReservationTime(hour, min);
		if (Time_TimeStamp() >= begin && Time_TimeStamp() <= end) {
			if (op == 1) {
				if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
					return true;
				}
			}else if (op == 2) {
				if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
					return true;
				}
			}else {
				continue;
			}
		}
	}

/*
	if (hour >= 12 && hour < 14) {
		if (hour == 12) {
			index = 0;
			index += min / 15;
			if (op == 1) {
				if ((min % 15) >= 0 && (min % 15) < 5) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			}else if (op == 2) {
				if ((min % 15) >= 5 && (min % 15) < 10) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			}
			return false;
		}else if (hour == 13) {
			index = 4;
			index += min / 15;
			if ((min % 15) >= 0 && (min % 15) < 5) {
				if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
					return true;
				}
			}
			return false;
		}
	}else if (hour >= 19 && hour < 22) {
		if (hour == 19) {
			index = 8;
			index += min / 15;
			if (op == 1) {
				if ((min % 15) >= 0 && (min % 15) < 5) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			}else if (op == 2) {
				if ((min % 15) >= 5 && (min % 15) < 10) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			} 
			return false;
		}else if (hour == 20) {
			index = 12;
			index += min / 15;
			if (op == 1) {
				if ((min % 15) >= 0 && (min % 15) < 5) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			}else if (op == 2) {
				if ((min % 15) >= 5 && (min % 15) < 10) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			}
			return false;
		}else if (hour == 21) {
			index = 16;
			index += min / 15;
			if (op == 1) {
				if ((min % 15) >= 0 && (min % 15) < 5) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			}else if (op == 2) {
				if ((min % 15) >= 5 && (min % 15) < 10) {
					if ( pool.reservation[index].roleID1 == roleID || pool.reservation[index].roleID2 == roleID	 ) {
						return true;
					}
				}
			}
			return false;
		}
	}
*/
	return false;
}

int PlayerPool_ReservationMap() {
	return pool.reservationMap;
}

int PlayerPool_ReservationList(NetProto_ReservationList * info) {
	if (info == NULL)
		return -1;

	for (int i = 0; i < 20; ++i) {
		if ( pool.reservation[i].roleID1 != -100 ) {
			NetProto_OneReservation *one = info->add_list();
			one->set_index(i);
			one->set_powerType(pool.reservation[i].powerType);
			one->set_roleID1(pool.reservation[i].roleID1);
			one->set_roleID2(pool.reservation[i].roleID2);
			one->set_name1(pool.reservation[i].name1);
			one->set_name2(pool.reservation[i].name2);
			one->set_fans1((int)(pool.reservation[i].setRole1.size()));
			one->set_fans2((int)(pool.reservation[i].setRole2.size()));
			one->set_rmb(pool.reservation[i].rmb);
			one->set_type1(pool.reservation[i].type1);
			one->set_type2(pool.reservation[i].type2);
		}
	}
	return 0;
}

void PlayerPool_SaveReservationToDB() {
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-24);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	static string buf;
	static  PB_ReservationToString proto;
	
	for (int i = 0; i < 20; ++i) {
		buf.clear();
		proto.Clear();

		{
			proto.set_powerType(pool.reservation[i].powerType);
			proto.set_roleID1(pool.reservation[i].roleID1);
			proto.set_roleID2(pool.reservation[i].roleID2);
			proto.set_name1(pool.reservation[i].name1);
			proto.set_name2(pool.reservation[i].name2);

			{
				for (set<int64_t>::iterator it = pool.reservation[i].setRole1.begin(); it != pool.reservation[i].setRole1.end(); ++it) {
					proto.add_setRole1(*it);
				}
				for (set<int64_t>::iterator it = pool.reservation[i].setRole2.begin(); it != pool.reservation[i].setRole2.end(); ++it) {
					proto.add_setRole2(*it);
				}
			}

			proto.set_rmb(pool.reservation[i].rmb);
			proto.set_type1(pool.reservation[i].type1);
			proto.set_type2(pool.reservation[i].type2);
		}

		saveRecord.mutable_record()->mutable_role()->set_roleID(i + 1);
		if (proto.SerializeToString(&buf)) {
			saveRecord.mutable_record()->mutable_role()->set_name(buf.c_str());
			GCAgent_SendProtoToDCAgent(&saveRecord);
		}
	}
}


