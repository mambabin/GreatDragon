#include "PlayerEntity.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "PlayerPool.hpp"
#include "Movement.hpp"
#include "Equipment.hpp"
#include "Fight.hpp"
#include "Status.hpp"
#include "StatusInfoManager.hpp"
#include "Func.hpp"
#include "Mission.hpp"
#include "ProfessionInfo.hpp"
#include "GCAgent.hpp"
#include "MapInfoManager.hpp"
#include "SkillInfo.pb.h"
#include "PlayerAtt.hpp"
#include "NPCEntity.hpp"
#include "NPCPool.hpp"
#include "Time.hpp"
#include "NetID.hpp"
#include "PlayerInfo.pb.h"
#include "RoleAtt.hpp"
#include "NetProto.pb.h"
#include "DCProto.pb.h"
#include "Math.hpp"
#include "ItemInfo.hpp"
#include "Debug.hpp"
#include "Item.hpp"
#include "EquipmentInfoManager.hpp"
#include "AccountPool.hpp"
#include "ConfigUtil.hpp"
#include "IDGenerator.hpp"
#include "GoodsInfoManager.hpp"
#include "DesignationInfoManager.hpp"
#include "GlobalMissionManager.hpp"
#include "NPCInfoManager.hpp"
#include "PlayOffManager.hpp"
#include "MapPool.hpp"
#include "ScribeClient.hpp"
#include "FactionPool.hpp"
#include "Event.hpp"
#include "GmSet.hpp"
#include <MD5.hpp>
#include <sys/types.h>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <climits>
#include <string>
#include <math.h>
#include <fstream>

using namespace std;

#define MSGST_A 20
#define MSGST_B 256

struct PlayerEntity{
	int32_t id;
	PlayerAtt att;
	struct Component component;
	int64_t lastWorldChatTime;
	int64_t lastSecretChatTime;
	int accountStatus;
	int64_t prevDropTime;
	int64_t prevDesignation;
	int64_t prevWing;
	int64_t prevSave;
	int32_t enhanceDelta[EquipAsset::ENHANCEDELTA_SIZE];
	int64_t enhanceEquip;
	int32_t fixedActiveTimes[3];
	int32_t hirePower[MAX_HIREPERSONNUMBER];
	uint32_t msgSt[MSGST_A][MSGST_B];
	int64_t prevMsgStTime;
	PlayerEntity() {
		id = -1;

		for(int i = 0; i < MSGST_A; i++)
		{
			for(int j = 0; j < MSGST_B; j++)
			{
				msgSt[i][j] = 0;
			}
		}
	}
};

static struct{
	int maxPlayers;
	vector<struct PlayerEntity *> playerRes;
	map<int64_t, struct PlayerEntity *> roleIDToPlayer;
	vector<struct StrongTable> strongTable;
	vector<int32_t> noticeEquip;
	vector<int32_t> noticeGoods;
}package;

/*
   static void Print(struct StrongTable *table) {
   DEBUG_LOG("1: %f", table->strongSuccess);
   DEBUG_LOG("2: %f", table->strongFail);
   DEBUG_LOG("3: %f", table->moneyStrongFactor);
   DEBUG_LOG("4: %lld", table->strongMoney);
   DEBUG_LOG("5: %f", table->strongMoneyFactor);
   DEBUG_LOG("6: %f", table->gemStrongFactor);
   DEBUG_LOG("7: %lld", table->strongGem);
   DEBUG_LOG("8: %f", table->strongGemFactor);
   DEBUG_LOG("9: %d", table->successTime);
   }
   */

static void PlayerEntity_ClearMsgSt(PlayerEntity *player)
{
	//if (!PlayerEntity_IsValid(player))
	//	return;

	for(int i = 0; i < MSGST_A; i++)
	{
		for(int j = 0; j < MSGST_B; j++)
		{
			if(player->msgSt[i][j] > 15)
				DEBUG_LOGERROR("role:%d, msgst->%d, %d, %ld", player->msgSt[i][j], i, j, player->att.att().baseAtt().roleID());
			//else if(player->msgSt[i][j] > 0)
			//	DEBUG_LOGERROR("xxxx %d, %d, %d", i, j, player->msgSt[i][j]);

			player->msgSt[i][j] = 0;
		}
	}

	player->prevMsgStTime = Time_ElapsedTime();
}

int PlayerEntity_AddMsgSt(struct PlayerEntity *player, uint32_t group, uint32_t unit)
{
	if (!PlayerEntity_IsValid(player))
		return -1;

	int64_t cur = Time_ElapsedTime();
	if(player->prevMsgStTime > cur || ((cur - player->prevMsgStTime) > 5000))
		PlayerEntity_ClearMsgSt(player);

	if(group < 0 || group >= MSGST_A)
		return -2;
	if(unit < 0 || unit >= MSGST_B)
		return -3;
	player->msgSt[group][unit]++;

	switch(group)
	{
		case 3:
			{
				switch(unit)
				{
					case 3:
						{
							if(player->msgSt[group][unit] >= 30)
								return 1;
						}
						break;
					case 34:
						{
							if(player->msgSt[group][unit] >= 45)
								return 1;
						}
						break;
				}
			}
			break;
		case 10:
			{
				switch(unit)
				{
					case 0:
					case 1:
					case 2:
					case 3:
						return 1;
				}
			}
			break;
	}

	return 0;
}

void PlayerEntity_Init() {
	package.maxPlayers = Config_MaxPlayers();
	package.playerRes.clear();
	package.roleIDToPlayer.clear();

	{
		package.strongTable.clear();
		string name = Config_DataPath() + string("/StrongTable.txt");
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
				// DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				// exit(EXIT_FAILURE);
				break;
			}

			char *tokens[CONFIG_FIXEDARRAY];
			int count = ConfigUtil_ExtraToken(line, tokens, CONFIG_FIXEDARRAY, ",");
			if (count == -1) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}
			struct StrongTable strongTable;
			strongTable.money = atoi(tokens[0]);
			strongTable.goods = atoi(tokens[1]);
			strongTable.strongSuccess = (float)atof(tokens[2]);
			strongTable.showSuccess = (float)atof(tokens[3]);
			strongTable.strongFail = (float)atof(tokens[4]);
			strongTable.att = (float)atof(tokens[5]);
			strongTable.protect = atoi(tokens[6]);
			package.strongTable.push_back(strongTable);
		}
		fclose(file);
	}
	{
		package.noticeEquip.clear();
		package.noticeGoods.clear();
		string name = Config_DataPath() + string("/MessageItems.txt");
		FILE *file = fopen(name.c_str(), "r");
		if (file == NULL) {
			DEBUG_LOGERROR("Failed to open file: %s", name.c_str());
			exit(EXIT_FAILURE);
		}
		{
			const char *key = "equip";
			char line[CONFIG_FIXEDARRAY];
			if (!ConfigUtil_ReadStr(file, key, line, CONFIG_FIXEDARRAY)) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}

			char *tokens[CONFIG_FIXEDARRAY];
			int count = ConfigUtil_ExtraToken(line, tokens, CONFIG_FIXEDARRAY, ",");
			if (count == -1) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}

			for (int i = 0; i < count; i++)
				package.noticeEquip.push_back(atoi(tokens[i]));
		}
		{
			const char *key = "goods";
			char line[CONFIG_FIXEDARRAY];
			if (!ConfigUtil_ReadStr(file, key, line, CONFIG_FIXEDARRAY)) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}

			char *tokens[CONFIG_FIXEDARRAY];
			int count = ConfigUtil_ExtraToken(line, tokens, CONFIG_FIXEDARRAY, ",");
			if (count == -1) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}

			for (int i = 0; i < count; i++)
				package.noticeGoods.push_back(atoi(tokens[i]));
		}
		fclose(file);
	}
}

static void InitEvent(struct PlayerEntity *entity) {
	time_t prev = (time_t)entity->att.prevLogout();
	time_t cur = Time_TimeStamp();

	int autoDurability = ((int)cur - (int)prev) / (60 * 20);
	if (autoDurability > 0) {
		Item_ModifyDurability(entity->component.item, autoDurability * Config_AutoRecoverDurability(), false);
	}

	//	int dayDelta = Time_DayDelta(prev, cur) + 1;
	int dayDelta = Time_DayDelta(prev, cur);
	int weekDelta = Time_WeekDelta(prev, cur);
	if (dayDelta <= 0) {
		if (entity->att.createTime() == entity->att.prevLogin() && entity->att.createTime() == entity->att.prevLogout())
			dayDelta = 1;
	}
	if (dayDelta > 0) {
		// DEBUG_LOG("%s prev, year: %d, mon: %d, day: %d", entity->att.att().baseAtt().name(), prevTM.tm_year, prevTM.tm_mon, prevTM.tm_mday);
		// DEBUG_LOG("%s cur, year: %d, mon: %d, day: %d", entity->att.att().baseAtt().name(), curTM.tm_year, curTM.tm_mon, curTM.tm_mday);
		PlayerEntity_ResetDayEvent(entity, dayDelta, weekDelta);
		GlobalMissionManager_ApplyDailyMission(entity->component.movement);
	}

	entity->att.set_lastLoginIP(NetID_IP(entity->id));
	int loginDelta = Time_DayDelta((time_t)entity->att.prevLogin(), cur);
	if (loginDelta > 0) {
		entity->att.set_totalNum(entity->att.totalNum() + 1);
		if (loginDelta == 1)
			entity->att.set_loginNum(entity->att.loginNum() + 1);
		else
			entity->att.set_loginNum(0);
	}

	if (weekDelta > 0) {
		if (weekDelta == 1) {
			struct tm curTM;
			Time_LocalTime(cur, &curTM);
			if (curTM.tm_wday != 1)
				PlayerEntity_SetDayEvent(entity, 6, 0);
		} else {
			PlayerEntity_SetDayEvent(entity, 6, 0);
		}
	}

	if (Time_PassHour(prev, cur, Config_HellBegin() + Config_HellTime() / (1000 * 3600))) {
		PlayerEntity_SetFixedEvent(entity, 18, 0);
	}

	const vector<MailGift> *mailGift = Config_MailGift(MailGift::EVERY_DAY);
	if (mailGift != NULL) {
		for (size_t i = 0; i < mailGift->size(); i++) {
			const MailGift *unit = &(*mailGift)[i];
			bool ok = false;
			if (unit->arg1() == 0 && unit->arg2() == 0 && unit->arg3() == 0) {
				ok = Time_PassHour(prev, cur, unit->arg4());
			} else {
				tm curTM;
				Time_LocalTime(cur, &curTM);
				if (curTM.tm_year + 1900 == unit->arg1()
						&& curTM.tm_mon + 1 == unit->arg2()
						&& curTM.tm_mday == unit->arg3())
					ok = Time_PassHour(prev, cur, unit->arg4());
			}
			if (ok) {
				static PB_MailInfo mail;
				mail = unit->mail();
				PlayerEntity_AddMail(entity, &mail);
			}
		}
	}
	mailGift = Config_MailGift(MailGift::COMPENSATION);
	if (mailGift != NULL) {
		for (size_t i = 0; i < mailGift->size(); i++) {
			const MailGift *unit = &(*mailGift)[i];
			if (entity->att.createTime() < unit->arg3() && (entity->att.fixedEvent(unit->arg1()) & (1 << unit->arg2())) == 0) {
				static PB_MailInfo mail;
				mail = unit->mail();
				PlayerEntity_AddMail(entity, &mail);
				PlayerEntity_SetFixedEvent(entity, unit->arg1(), entity->att.fixedEvent(unit->arg1()) | (1 << unit->arg2()));
			}
		}
	}
	mailGift = Config_MailGift(MailGift::RECHARGE);
	if ((entity->att.fixedEvent(108) & (1 << 0)) == 0) {
		if (mailGift != NULL) {
			for (size_t i = 0; i < mailGift->size(); i++) {
				const ItemPackage *package = Item_Package(entity->component.item);
				if (package == NULL)
					return;
				const MailGift *unit = &(*mailGift)[i];
				DEBUG_LOG("createtime:%d, shijianchuo:%d, totalRMB():%d", entity->att.createTime(), unit->arg3(), package->totalRMB());
				if ( unit->arg2() <= package->totalRMB() && (entity->att.fixedEvent(unit->arg1()) & (1 << unit->arg4())) == 0)  {
					static PB_MailInfo mail;
					mail = unit->mail();
					PlayerEntity_AddMail(entity, &mail);
					PlayerEntity_SetFixedEvent(entity, unit->arg1(), entity->att.fixedEvent(unit->arg1()) | (1 << unit->arg4()));
				}
			}
		}
		PlayerEntity_SetFixedEvent(entity, 108, entity->att.fixedEvent(108) | (1 << 0));
	}

	Event_CheckFactionWarWinnerDesignation(entity, false);

	// playofftest
	// PlayerEntity_SetFixedEvent(entity, 46, 1);
	// PlayerEntity_SetFixedEvent(entity, 47, 0);
	for (int i = 0; ; i++) {
		const PlayOff *info = PlayOffManager_PlayOff(i);
		if (info == NULL)
			break;
		int day, pass, turn, overTime;
		if (PlayOffManager_CurData(i, &day, &pass, &turn, &overTime) == 0) {
			int right = PlayerEntity_CanEnterPlayOff(entity, i, day, pass);
			if (right == 0 || right == -2) {
				static NetProto_PlayOffInfo proto;
				proto.Clear();
				if (right == 0)
					proto.set_res(0);
				else if (right == -2)
					proto.set_res(-3);
				proto.set_id(i);
				proto.set_day(day);
				proto.set_pass(pass);
				proto.set_overTime(overTime);
				proto.set_turn(turn);
				proto.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(-1);
				int64_t roleID = entity->component.baseAtt->roleID();
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
				GCAgent_SendProtoToClients(&entity->id, 1, &proto);

				if (right == 0) {
					static NetProto_Message proto;
					proto.set_content(Config_Words(61));
					proto.set_count(2);
					GCAgent_SendProtoToClients(&entity->id, 1, &proto);
				}
			}
		}
	}

	entity->att.set_prevLogin((int64_t)cur);
}

static void ClearData(PlayerAtt *data) {
	for (int i = 0; i < data->att().fightAtt().properties_size(); i++) {
		data->mutable_att()->mutable_fightAtt()->set_properties(i, 0);
	}
	for (int i = 0; i < data->att().fightAtt().propertiesDelta_size(); i++) {
		FightPropertyDelta *delta = data->mutable_att()->mutable_fightAtt()->mutable_propertiesDelta(i);
		delta->set_delta(0);
		delta->set_percent(0);
	}
}

static void AddDesignationEffect(struct PlayerEntity *entity, int32_t id, bool add) {
	const DesignationInfo *info = DesignationInfoManager_DesignationInfo(id);
	assert(info != NULL);
	for (int i = 0; i < info->properties_size(); i++) {
		if (info->propertiesDelta(i) > 0)
			Fight_ModifyProperty(entity->component.fight, (FightAtt::PropertyType)info->properties(i), add ? info->propertiesDelta(i) : -info->propertiesDelta(i));
	}
}

struct PlayerEntity * PlayerEntity_Create(int32_t id, const PB_PlayerAtt *att) {
	if (id < 0 || id >= package.maxPlayers || att == NULL) {
		return NULL;
	}

	struct PlayerEntity *entity = SearchBlock(package.playerRes, id);
	if (entity->id != -1) {
		return NULL;
	}

	// assert(package.roleIDToPlayer.find(att->att().baseAtt().roleID()) == package.roleIDToPlayer.end());
	if (package.roleIDToPlayer.find(att->att().baseAtt().roleID()) != package.roleIDToPlayer.end()) {
		DEBUG_LOGERROR("Role has not destroyed, roleid: %lld", (long long)att->att().baseAtt().roleID());
		return NULL;
	}

	entity->id = id;
	entity->att.FromPB(att);
	ClearData(&entity->att);
	entity->component.player = entity;
	entity->component.npc = NULL;
	entity->component.playerAtt = &entity->att;
	entity->component.npcAtt = NULL;
	entity->component.baseAtt = entity->att.mutable_att()->mutable_baseAtt();
	entity->component.roleAtt = entity->att.mutable_att();
	entity->component.movement = Movement_Create(entity->att.mutable_att()->mutable_movementAtt(), &entity->component);
	entity->component.equipment = Equipment_Create(entity->att.mutable_att()->mutable_equipmentAtt(), &entity->component);
	entity->component.fight = Fight_Create(entity->att.mutable_att()->mutable_fightAtt(), &entity->component);
	entity->component.status = Status_Create(&entity->component);
	entity->component.ai = NULL;
	entity->component.func = NULL;
	entity->component.mission = Mission_Create(entity->att.mutable_missions(), &entity->component);
	entity->component.item = Item_Create(entity->att.mutable_itemPackage(), entity->att.mutable_alt(), &entity->component);
	entity->lastWorldChatTime = CONFIG_INIT_USE_TIME;
	entity->accountStatus = ACCOUNT_ONLINE;
	entity->prevDesignation = Time_ElapsedTime();
	entity->prevDesignation = entity->prevDesignation;
	entity->prevSave = Time_ElapsedTime();
	entity->enhanceEquip = -1;

	package.roleIDToPlayer[att->att().baseAtt().roleID()] = entity;

	PlayerPool_Add(entity);

	Equipment_Prepare(entity->component.equipment);
	Fight_Prepare(entity->component.fight);
	Mission_Prepare(entity->component.mission);
	Item_Prepare(entity->component.item);
	AI_Prepare(entity->component.ai);

	int64_t cur = (int64_t)Time_TimeStamp();
	for (int i = 0; i < entity->att.designationRecord_size(); i++) {
		DesignationRecord *record = entity->att.mutable_designationRecord(i);
		if (record->has()) {
			const DesignationInfo *info = DesignationInfoManager_DesignationInfo(i);
			assert(info != NULL);
			if (info->time() > 0) {
				if (cur - record->start() > info->time()) {
					record->set_has(false);
					continue;
				}
			}
			AddDesignationEffect(entity, i, true);
		}
	}

	for (int i = 0; i < MAX_HIREPERSONNUMBER; ++i) {
		entity->hirePower[i] = -1;		
	}

	InitEvent(entity);
	PlayerPool_FilterDesignation(entity);

	// DEBUG_LOGRECORD("Role login, point: %p, account: %s, roleID: %lld, total roles: %d", entity, info->account(), (long long)entity->att.att().baseAtt().roleID(), PlayerPool_TotalCount());

	return entity;
}

bool PlayerEntity_IsValid(struct PlayerEntity *entity) {
	if (entity == NULL || entity->id < 0 || entity->id >= package.maxPlayers)
		return false;

	return true;
}

struct PlayerEntity * PlayerEntity_Player(int32_t id) {
	if (id < 0 || id >= package.maxPlayers)
		return NULL;

	struct PlayerEntity *entity = SearchBlock(package.playerRes, id);
	return entity->id == -1 ? NULL : entity;
}

struct PlayerEntity * PlayerEntity_PlayerByRoleID(int64_t roleID) {
	map<int64_t, struct PlayerEntity *>::iterator it = package.roleIDToPlayer.find(roleID);
	return it == package.roleIDToPlayer.end() ? NULL : it->second;
}

void PlayerEntity_Destroy(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return;

	entity->id = -1;
}

void PlayerEntity_Finalize(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return;

	package.roleIDToPlayer.erase(entity->att.att().baseAtt().roleID());
	PlayerPool_Del(entity);
}

int32_t PlayerEntity_ID(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	return entity->id;
}

const PlayerAtt * PlayerEntity_Att(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return NULL;

	return &entity->att;
}

void PlayerEntity_ResetDayEvent(struct PlayerEntity *entity, int dayDelta, int weekDelta) {
	if (!PlayerEntity_IsValid(entity))
		return;
	if (dayDelta <= 0)
		return;

	struct Component *component = PlayerEntity_Component(entity);
	if (component == NULL)
		return;
	int event = component->playerAtt->dayEvent(29) & 0xFFFF;

	for (int i = 0; i < entity->att.dayEvent_size(); i++)
		PlayerEntity_SetDayEvent(entity, i, 0);

	//time_t cur = Time_TimeStamp();
	Event_ResetDoubleRecharge(entity);
	Event_RecieveRoses(entity);
	Event_RealGod(entity, event);

	entity->att.mutable_att()->mutable_fightAtt()->set_curTower(1);

	PlayerEntity_SetFixedEvent(entity, 0, 0);

	for (int i = 0; i < entity->att.friends_size(); ++i) {
		entity->att.mutable_friends(i)->set_loveHeart(false);
		entity->att.mutable_friends(i)->set_flag(false);
	}
	for (int i = 0; i < entity->att.fans_size(); ++i) {
		entity->att.mutable_fans(i)->set_loveHeart(false);
		entity->att.mutable_fans(i)->set_flag(false);
	}

	int count = entity->att.fixedEvent(31) & 0xffff;
	if (count >= 31) {
		count = 0;
		PlayerEntity_SetFixedEvent(entity, 44, 0);
	}
	PlayerEntity_SetFixedEvent(entity, 31, (count + 1) | (entity->att.fixedEvent(31) & 0x7fff0000));

	count = entity->att.fixedEvent(12) & 0xffff;
	if (count < 7) {
		PlayerEntity_SetFixedEvent(entity, 12, (count + 1) | (entity->att.fixedEvent(12) & 0x7fff0000));
	}

	if (weekDelta > 0) {
		PlayerEntity_SetFixedEvent(entity, 15, entity->component.playerAtt->fixedEvent(15) & (~1));
		PlayerEntity_SetFixedEvent(entity, 47, 0);
	}

	int d = Time_DayDelta(entity->att.fixedEvent(30), (int)Time_TimeStamp());
	if (d >= 2) {
		entity->att.set_fixedEvent(1, 0);
	} else if (d == 1) {
		bool done = true;
		for (int i = 0; i < 7; i++) {
			if ((entity->att.fixedEvent(1) & (1 << i)) == 0) {
				done = false;
				break;
			}
		}
		if (done) {
			entity->att.set_fixedEvent(1, 0);
		}
	}

	if (!Config_InActiveTime(9) || !Config_SameActiveInterval(9, (int)entity->att.prevLogout(), (int)Time_TimeStamp())) {
		PlayerEntity_SetFixedEvent(entity, 94, entity->att.fixedEvent(94) & 0xff000000);
		PlayerEntity_SetFixedEvent(entity, 35, entity->att.fixedEvent(35) & 0x0000ffff);
	}
	if (!Config_InActiveTime(10) || !Config_SameActiveInterval(10, (int)entity->att.prevLogout(), (int)Time_TimeStamp())) {
		PlayerEntity_SetFixedEvent(entity, 9, 0);
		PlayerEntity_SetFixedEvent(entity, 8, 0);
	}
	if (!Config_InActiveTime(16) || !Config_SameActiveInterval(16, (int)entity->att.prevLogout(), (int)Time_TimeStamp())) {
		PlayerEntity_SetFixedEvent(entity, 86, 0);
		PlayerEntity_SetFixedEvent(entity, 87, 0);
	}
	if (!Config_InActiveTime(20) || !Config_SameActiveInterval(20, (int)entity->att.prevLogout(), (int)Time_TimeStamp())) {
		PlayerEntity_SetFixedEvent(entity, 88, 0);
	}
	if (!Config_InActiveTime(22) || !Config_SameActiveInterval(22, (int)entity->att.prevLogout(), (int)Time_TimeStamp())) {
		PlayerEntity_SetFixedEvent(entity, 15, entity->att.fixedEvent(15) & 0xFFFFF7FF);
	}
	if (!Config_InActiveTime(18) || !Config_SameActiveInterval(18, (int)entity->att.prevLogout(), (int)Time_TimeStamp())) {
		for (int index = 3; index < 10; ++index) {
			if (entity->att.fixedEvent(15) & (1 << index))
				PlayerEntity_SetFixedEvent(entity, 15, entity->att.fixedEvent(15) & (~(1 << index)));
		}   
		PlayerEntity_SetFixedEvent(entity, 15, component->playerAtt->fixedEvent(15) & (~(1 << 10)));
	}
}

void PlayerEntity_SetDayEvent(struct PlayerEntity *entity, int event, int32_t value, bool send) {
	if (!PlayerEntity_IsValid(entity))
		return;
	if (event < 0 || event >= entity->att.dayEvent_size())
		return;
	entity->att.set_dayEvent(event, value);

	if (send) {
		static NetProto_SetDayEvent sde;
		sde.set_id(event);
		sde.set_v(value);
		GCAgent_SendProtoToClients(&entity->id, 1, &sde);
	}
}

void PlayerEntity_SetFixedEvent(struct PlayerEntity *entity, int event, int32_t value, bool send) {
	if (!PlayerEntity_IsValid(entity))
		return;
	if (event < 0 || event >= entity->att.fixedEvent_size())
		return;
	entity->att.set_fixedEvent(event, value);

	if (send) {
		static NetProto_SetFixedEvent sfe;
		sfe.set_id(event);
		sfe.set_v(value);
		GCAgent_SendProtoToClients(&entity->id, 1, &sfe);
	}
}

int PlayerEntity_Check(struct PlayerEntity *entity, int index, int64_t time, const char *res) {
	if (!PlayerEntity_IsValid(entity) || res == NULL)
		return -1;
	static const char *key[] = {"F6E4E1FA-1DE6-EAFD-8535-FA09A073743F",
		"80C35F74-B37A-2248-3088-53C62C6C7795",
		"E20C1C5F-0893-80AD-924C-D26099D24C3D",
		"5108FE26-06D9-CB19-6C99-418A3102F14D",
		"1BB97A7E-E533-DF31-F3E4-E213D36F9EED",
		"312322F7-8E0A-0BEA-70BA-A610BC492967",
		"955606A4-C407-FAEC-27A8-2C6C08E649F1",
		"DBE2AC58-92BA-D809-2B2B-AB710EEDBB12",
		"93C9AB85-8B83-BEF9-961A-AFF80B25AF6B",
		"D26BD519-896F-B103-4FF8-EF0DF3B812CD"};
	static int count = (int)(sizeof(key) / sizeof(key[0]));
	if (index < 0 || index >= count)
		return -2;

	if (Time_ElapsedTime() - time > Config_HeartbeatTime())
		return -3;

	int32_t map = Movement_Att(entity->component.movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
	if (mapInfo == NULL)
		return -4;
	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	if (blockInfo == NULL)
		return -5;

	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, "%d%s%d%lld", blockInfo->logicLength(), key[index], blockInfo->logicWidth(), (long long)time);
	const string &final = md5(buf);
	return strcmp(final.c_str(), res) == 0 ? 0 : -6;
}

int PlayerEntity_CheckCombatRecord(struct PlayerEntity *entity, const CombatRecord& combatRecord)
{
	if (!PlayerEntity_IsValid(entity))
		return -1;

	int32_t mapId = Movement_Att(entity->component.movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapId));
	
	//time
	int64_t endTime = Time_ElapsedTime();
	int64_t beginTime = Movement_Time(entity->component.movement);
	if(endTime < (beginTime + mapInfo->minTime() * 1000)
			|| endTime > (beginTime + mapInfo->maxTime() * 1000)
	  )
	{
			return -2;
	}
	if(combatRecord.combatTurns_size() > (endTime - beginTime)/1000 * 5 )
	{
		return -3;
	}
	if((combatRecord.combatTurns(0).time() + 500) < beginTime
			|| (combatRecord.combatTurns(combatRecord.combatTurns_size() - 1).time() - 500) > endTime
	  )
	{
		return -4;
	}
	for(int i = 1; i < combatRecord.combatTurns_size(); i++)
	{
		if(combatRecord.combatTurns(i).time() <= combatRecord.combatTurns(i-1).time())
		{
			return -5;
		}
	}
	
	//transform
	vector<int64_t> transeformTime;
	for(int i = 1; i < combatRecord.combatTurns_size(); i++)
	{
		const CombatTurn& combatTurn = combatRecord.combatTurns(i);
		if(combatTurn.type() == CombatTurn_Type_TRANSFORM)
		{
			transeformTime.push_back(combatTurn.time());
		}
	}
	if(transeformTime.size() > 2)
		return -11;
	for(size_t i = 1; i < transeformTime.size(); i++)
	{
		if((transeformTime[i] - transeformTime[i-1]) < (30 * 1000))
		{
			return -12;
		}
	}

	return 0;
}

int PlayerEntity_CheckCombatRecordDamage(struct PlayerEntity *entity, const CombatRecord& combatRecord, bool isPvp)
{
	if (!PlayerEntity_IsValid(entity))
		return -1;

	if(isPvp)
	{
		int roleMadeDmg = 0;
		int petMadeDmg = 0;
		int enermyDmg = 0;
		int enermyPetDmg = 0;

		const FightAtt* fightAtt = Fight_Att(entity->component.fight);
		int32_t atk = Fight_FinalProperty(fightAtt, FightAtt::ATK);
		//int32_t def = Fight_FinalProperty(fightAtt, FightAtt::DEF);
		int32_t maxHp = Fight_FinalProperty(fightAtt, FightAtt::MAXHP);

		int64_t enermyId = PlayerPool_GodRankEnermyId(entity->id);
		PlayerEntity* enermyEntity = PlayerEntity_Player(enermyId);
		if (!PlayerEntity_IsValid(enermyEntity))
			return -1;

		const FightAtt* enermyFightAtt = Fight_Att(enermyEntity->component.fight);
		//int32_t enermyAtk = Fight_FinalProperty(enermyFightAtt, FightAtt::ATK);
		int32_t enermyDef = Fight_FinalProperty(enermyFightAtt, FightAtt::DEF);
		int32_t enermyMaxHp = Fight_FinalProperty(enermyFightAtt, FightAtt::MAXHP);

		for(int i = 1; i < combatRecord.combatTurns_size(); i++)
		{
			const CombatTurn& combatTurn = combatRecord.combatTurns(i);
			if(combatTurn.type() == CombatTurn_Type_ROLE_ATTACK)
			{
				roleMadeDmg += combatTurn.damage();

				const StatusInfo* statusInfo = StatusInfoManager_StatusInfo(combatTurn.status());
				//	((attack-def) > att * 1/20 ? old , 1/20 * status.percent + status.value) *3  <====> damage
				int damage = atk - enermyDef;
				if(damage < int(atk * 0.05))
				{
					damage = int(atk * 0.05);
				}
				damage = (int)((damage * statusInfo->percent() + statusInfo->value()) * 3);
				if(combatTurn.damage() > damage)
					return -2;
			}
			else if(combatTurn.type() == CombatTurn_Type_PET_ATTACK)
			{
				petMadeDmg += combatTurn.damage();
			}
			else if(combatTurn.type() == CombatTurn_Type_ATTACKED_ENERMY)
			{
				enermyDmg += combatTurn.damage();
			}
			else if(combatTurn.type() == CombatTurn_Type_ATTACKED_ENERMYPET)
			{
				enermyPetDmg += combatTurn.damage();
			}
		}

		if((roleMadeDmg + petMadeDmg) < enermyMaxHp)
		{
			return -3;
		}
		if((enermyDmg + enermyPetDmg) > maxHp)
		{
			return -4;
		}
	}
	else
	{
		return 0;
	}
}

void PlayerEntity_CompleteGuide(struct PlayerEntity *entity, int id) {
	if (!PlayerEntity_IsValid(entity))
		return;
	int index = id / 31;
	int bit = id % 31;
	if (index < 0 || index >= entity->att.passGuide_size())
		return;
	entity->att.set_passGuide(index, entity->att.passGuide(index) | (1 << bit));
}

void PlayerEntity_CompleteGuide(PB_PlayerAtt *att, int id) {
	if (att == NULL)
		return;
	int index = id / 31;
	int bit = id % 31;
	if (index < 0 || index >= att->passGuide_size())
		return;
	att->set_passGuide(index, att->passGuide(index) | (1 << bit));
}

bool PlayerEntity_HasDesignation(struct PlayerEntity *entity, int32_t id) {
	if (!PlayerEntity_IsValid(entity))
		return false;

	if (id < 0 || id >= entity->att.designationRecord_size())
		return false;

	const DesignationInfo *info = DesignationInfoManager_DesignationInfo(id);
	if (info == NULL)
		return false;

	DesignationRecord *record = entity->att.mutable_designationRecord(id);
	return record->has();
}

int PlayerEntity_AddDesignation(struct PlayerEntity *entity, int32_t id) {
	if (!PlayerEntity_IsValid(entity))
		return -1;
	if (id < 0 || id >= entity->att.designationRecord_size())
		return -1;

	const DesignationInfo *info = DesignationInfoManager_DesignationInfo(id);
	if (info == NULL)
		return -1;

	DesignationRecord *record = entity->att.mutable_designationRecord(id);
	if (record->has())
		return -1;

	record->set_has(true);
	record->set_start((int64_t)Time_TimeStamp());

	AddDesignationEffect(entity, id, true);

	static NetProto_AddDesignation ad;
	ad.Clear();
	ad.set_id(id);
	GCAgent_SendProtoToClients(&entity->id, 1, &ad);

	int empty = 0;
	if (entity->att.showDesignation(empty) == -1)
		PlayerEntity_ShowDesignation(entity, id);

	return 0;
}

void PlayerEntity_DelDesignation(struct PlayerEntity *entity, int32_t id, bool notice) {
	if (!PlayerEntity_IsValid(entity))
		return;
	if (id < 0 || id >= entity->att.designationRecord_size())
		return;
	if (DesignationInfoManager_DesignationInfo(id) == NULL)
		return;

	DesignationRecord *record = entity->att.mutable_designationRecord(id);
	record->set_has(false);

	AddDesignationEffect(entity, id, false);

	PlayerEntity_UnshowDesignation(entity, id, notice);

	if (notice) {
		static NetProto_DelDesignation dd;
		dd.Clear();
		dd.set_id(id);
		GCAgent_SendProtoToClients(&entity->id, 1, &dd);
	}
}

int PlayerEntity_ShowDesignation(struct PlayerEntity *entity, int32_t id) {
	if (!PlayerEntity_IsValid(entity))
		return -1;
	if (id < 0 || id >= entity->att.designationRecord_size())
		return -1;

	const DesignationInfo *info = DesignationInfoManager_DesignationInfo(id);
	if (info == NULL)
		return -1;

	DesignationRecord *record = entity->att.mutable_designationRecord(id);
	if (!record->has())
		return -1;

	/*
	   int empty = -1;
	   for (int i = 0; i < Config_MaxShowDesignation(); i++) {
	   if (entity->att.showDesignation(i) == -1) {
	   empty = i;
	   break;
	   }
	   }
	   if (empty == -1)
	   return -1;
	   */
	int empty = 0;
	entity->att.set_showDesignation(empty, id);

	static NetProto_ShowDesignation sd;
	sd.Clear();
	sd.set_player(entity->id);
	sd.set_id(id);
	GCAgent_SendProtoToAllClients(&sd);
	return 0;
}

void PlayerEntity_UnshowDesignation(struct PlayerEntity *entity, int32_t id, bool notice) {
	if (!PlayerEntity_IsValid(entity))
		return;

	bool find = false;
	for (int i = 0; i < Config_MaxShowDesignation(); i++) {
		if (entity->att.showDesignation(i) == id) {
			entity->att.set_showDesignation(i, -1);
			find = true;
			break;
		}
	}
	if (!find)
		return;

	if (notice) {
		static NetProto_UnshowDesignation ud;
		ud.Clear();
		ud.set_player(entity->id);
		ud.set_id(id);
		GCAgent_SendProtoToAllClients(&ud);
	}
}

void PlayerEntity_ToSceneData(const PlayerAtt *src, PB_PlayerAtt *dest) {
	if (src == NULL || dest == NULL)
		return;

	for (int i = 0; i < src->designationRecord_size(); i++)
		src->designationRecord(i).ToPB(dest->add_designationRecord());
	for (int i = 0; i < src->showDesignation_size(); i++)
		dest->add_showDesignation(src->showDesignation(i));

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
	dest->mutable_att()->mutable_fightAtt()->set_bloodLevel(src->att().fightAtt().bloodLevel());
	dest->mutable_att()->mutable_fightAtt()->set_fightingPet(src->att().fightAtt().fightingPet());
	for (int i = 0; i < src->att().fightAtt().skills_size(); i++)
		src->att().fightAtt().skills(i).ToPB(dest->mutable_att()->mutable_fightAtt()->add_skills());
	dest->mutable_att()->mutable_fightAtt()->set_transformID(src->att().fightAtt().transformID());

	dest->mutable_itemPackage()->set_vip(src->itemPackage().vip());
	if (!src->att().equipmentAtt().baseWing()) {
		for (int i = 0; i <= src->att().equipmentAtt().wing(); i++)
			dest->mutable_itemPackage()->add_wings(src->itemPackage().wings(i));
	}
	for (int i = 0; i <= src->att().equipmentAtt().fashion(); i++) {
		PB_FashionAsset *asset = dest->mutable_itemPackage()->add_fashions();
		if (i == src->att().equipmentAtt().fashion())
			src->itemPackage().fashions(i).ToPB(asset);
	}
	for (int i = 0; i <= src->att().fightAtt().fightingPet(); i++) {
		PB_PetAsset *asset = dest->add_pets();
		if (i == src->att().fightAtt().fightingPet())
			src->pets(i).ToPB(asset);
	}
	dest->mutable_att()->mutable_fightAtt()->set_baseWingLevel(src->att().fightAtt().baseWingLevel());
	dest->mutable_att()->mutable_fightAtt()->set_baseWingDegree(src->att().fightAtt().baseWingDegree());
	dest->set_faction(src->faction());

	for (int i = 0, n = 0; i < src->att().equipmentAtt().equipments_size(); i++) {
		int64_t id = src->att().equipmentAtt().equipments(i);
		if (id < 0)
			continue;
		dest->mutable_att()->mutable_equipmentAtt()->set_equipments(i, n);
		src->itemPackage().equips(id).ToPB(dest->mutable_itemPackage()->add_equips());
		n++;
	}
}

void PlayerEntity_ToSceneData(const PB_PlayerAtt *src, PB_PlayerAtt *dest) {
	if (src == NULL || dest == NULL)
		return;

	*dest->mutable_designationRecord() = src->designationRecord();
	*dest->mutable_showDesignation() = src->showDesignation();

	dest->mutable_att()->mutable_baseAtt()->set_name(src->att().baseAtt().name());
	dest->mutable_att()->mutable_baseAtt()->set_professionType((PB_ProfessionInfo::Type)src->att().baseAtt().professionType());
	dest->mutable_att()->mutable_baseAtt()->set_male(src->att().baseAtt().male());
	dest->mutable_att()->mutable_baseAtt()->set_roleID(src->att().baseAtt().roleID());
	dest->mutable_att()->mutable_baseAtt()->set_height(src->att().baseAtt().height());

	*dest->mutable_att()->mutable_movementAtt() = src->att().movementAtt();

	*dest->mutable_att()->mutable_equipmentAtt() = src->att().equipmentAtt();

	dest->mutable_att()->mutable_fightAtt()->set_selfFaction(src->att().fightAtt().selfFaction());
	dest->mutable_att()->mutable_fightAtt()->set_friendlyFaction(src->att().fightAtt().friendlyFaction());
	dest->mutable_att()->mutable_fightAtt()->set_hp(src->att().fightAtt().hp());
	dest->mutable_att()->mutable_fightAtt()->set_mana(src->att().fightAtt().mana());
	for (int i = 0; i < src->att().fightAtt().properties_size(); i++) {
		dest->mutable_att()->mutable_fightAtt()->add_properties(src->att().fightAtt().properties(i));
		*dest->mutable_att()->mutable_fightAtt()->add_propertiesDelta() = src->att().fightAtt().propertiesDelta(i);
	}
	dest->mutable_att()->mutable_fightAtt()->set_level(src->att().fightAtt().level());
	dest->mutable_att()->mutable_fightAtt()->set_energy(src->att().fightAtt().energy());
	dest->mutable_att()->mutable_fightAtt()->set_bloodLevel(src->att().fightAtt().bloodLevel());
	dest->mutable_att()->mutable_fightAtt()->set_fightingPet(src->att().fightAtt().fightingPet());
	*dest->mutable_att()->mutable_fightAtt()->mutable_skills() = src->att().fightAtt().skills();
	dest->mutable_att()->mutable_fightAtt()->set_transformID(src->att().fightAtt().transformID());

	dest->mutable_itemPackage()->set_vip(src->itemPackage().vip());
	if (!src->att().equipmentAtt().baseWing()) {
		for (int i = 0; i <= src->att().equipmentAtt().wing(); i++)
			dest->mutable_itemPackage()->add_wings(src->itemPackage().wings(i));
	}
	for (int i = 0; i <= src->att().equipmentAtt().fashion(); i++) {
		PB_FashionAsset *asset = dest->mutable_itemPackage()->add_fashions();
		if (i == src->att().equipmentAtt().fashion())
			*asset = src->itemPackage().fashions(i);
	}
	for (int i = 0; i <= src->att().fightAtt().fightingPet(); i++) {
		PB_PetAsset *asset = dest->add_pets();
		if (i == src->att().fightAtt().fightingPet())
			*asset = src->pets(i);
	}
	dest->mutable_att()->mutable_fightAtt()->set_baseWingLevel(src->att().fightAtt().baseWingLevel());
	dest->mutable_att()->mutable_fightAtt()->set_baseWingDegree(src->att().fightAtt().baseWingDegree());
	dest->set_faction(src->faction());

	for (int i = 0, n = 0; i < src->att().equipmentAtt().equipments_size(); i++) {
		int64_t id = src->att().equipmentAtt().equipments(i);
		if (id < 0)
			continue;
		dest->mutable_att()->mutable_equipmentAtt()->set_equipments(i, n);
		*dest->mutable_itemPackage()->add_equips() = src->itemPackage().equips(id);
		n++;
	}
}

void PlayerEntity_CopyAsNPC(const PlayerAtt *src, NPCAtt *dest, float power, bool randomName, int faction, int reviveTime) {
	if (src == NULL || dest == NULL)
		return;

	if (randomName)
		dest->mutable_att()->mutable_baseAtt()->set_name(Config_RandomName());
	else
		dest->mutable_att()->mutable_baseAtt()->set_name(src->att().baseAtt().name());
	dest->mutable_att()->mutable_baseAtt()->set_professionType(src->att().baseAtt().professionType());
	dest->mutable_att()->mutable_baseAtt()->set_male(src->att().baseAtt().male());
	dest->mutable_att()->mutable_baseAtt()->set_roleID(src->att().baseAtt().roleID());
	dest->mutable_att()->mutable_baseAtt()->set_height(src->att().baseAtt().height());

	*dest->mutable_att()->mutable_movementAtt() = src->att().movementAtt();

	*dest->mutable_att()->mutable_equipmentAtt() = src->att().equipmentAtt();

	dest->mutable_att()->mutable_fightAtt()->set_selfFaction(faction);
	dest->mutable_att()->mutable_fightAtt()->set_friendlyFaction(faction);
	dest->mutable_att()->mutable_fightAtt()->set_hp(src->att().fightAtt().hp());
	dest->mutable_att()->mutable_fightAtt()->set_mana(src->att().fightAtt().mana());
	dest->mutable_att()->mutable_fightAtt()->set_reviveTime(reviveTime);
	for (int i = 0; i < src->att().fightAtt().properties_size(); i++) {
		dest->mutable_att()->mutable_fightAtt()->set_properties(i, (int)(src->att().fightAtt().properties(i) * power));
		FightPropertyDelta *delta = dest->mutable_att()->mutable_fightAtt()->mutable_propertiesDelta(i);
		*delta = src->att().fightAtt().propertiesDelta(i);
		delta->set_delta((int)(delta->delta() * power));
		if (delta->percent() != 0)
			delta->set_percent(delta->percent() * power);
	}
	dest->mutable_att()->mutable_fightAtt()->set_level(src->att().fightAtt().level());
	dest->mutable_att()->mutable_fightAtt()->set_energy(src->att().fightAtt().energy());
	for (int i = src->att().fightAtt().skills_size() - 1, j = 0; i >= 0; i--, j++) {
		*dest->mutable_att()->mutable_fightAtt()->mutable_skills(j) = src->att().fightAtt().skills(i);
	}
	dest->mutable_att()->mutable_fightAtt()->set_transformID(src->att().fightAtt().transformID());

	for (int i = 0, n = 0; i < src->att().equipmentAtt().equipments_size(); i++) {
		int64_t id = src->att().equipmentAtt().equipments(i);
		if (id < 0)
			continue;
		dest->mutable_att()->mutable_equipmentAtt()->set_equipments(i, n);
		*dest->mutable_equips(n) = src->itemPackage().equips(id);
		n++;
	}

	AIAtt *ai = dest->mutable_att()->mutable_aiAtt();
	const int radius = 300, interval = 1000;
	ai->set_status(AIAtt::IDLE);
	ai->set_moveRadius(radius);
	ai->set_moveType(AIAtt::FREE);
	ai->set_followRadius(radius);
	ai->set_searchRadius(radius);
	ai->set_searchType(AIAtt::MINDIST);
	ai->set_fleeType(AIAtt::DONTFLEE);
	ai->set_canAttackBack(false);
	ai->set_moveInterval(interval);
	ai->set_searchInterval(interval);
	ai->set_aiType(AIAtt::NORMAL);
}

void PlayerEntity_FilterData(PlayerAtt *att) {
	if (att == NULL)
		return;
	for (int i = 0; i < att->itemPackage().fashions_size(); i++) {
		FashionAsset *asset = att->mutable_itemPackage()->mutable_fashions(i);
		for (int j = 0; j < asset->runes_size(); j++)
			asset->set_runes(j, j < 2 ? -1 : -2);
	}
}

void PlayerEntity_FilterData(PB_PlayerAtt *att) {
	if (att == NULL)
		return;
	for (int i = att->itemPackage().fashions_size(); i < ItemPackage::FASHIONS_SIZE; i++) {
		PB_FashionAsset *asset = att->mutable_itemPackage()->add_fashions();
		for (int j = 0; j < FashionAsset::RUNES_SIZE; j++)
			asset->add_runes(j < 2 ? -1 : -2);
	}
}

int PlayerEntity_AccountStatus(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return ACCOUNT_OFFLINE;
	return entity->accountStatus;
}

void PlayerEntity_ReLogin(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return;

	entity->accountStatus = ACCOUNT_ONLINE;
}

int64_t PlayerEntity_RoleID(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	return entity->att.att().baseAtt().roleID();
}

struct Component * PlayerEntity_Component(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return NULL;

	return &entity->component;
}

void PlayerEntity_Drop(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return;
	if (entity->accountStatus != ACCOUNT_ONLINE)
		return;

	DEBUG_LOG("Role drop: %d", entity->id);
	entity->accountStatus = ACCOUNT_DROP;
	entity->prevDropTime = Time_ElapsedTime();
	GCAgent_SendOrderToNetAgent(ORDER_PROTECT_ID, entity->id, -1);
}

void PlayerEntity_Save(struct PlayerEntity *entity, DCProto_SaveRoleData *proto) {
	if (!PlayerEntity_IsValid(entity) || proto == NULL)
		return;

	entity->att.set_prevLogout((int64_t)Time_TimeStamp());
	entity->att.set_dayEvent(3, entity->att.dayEvent(3) + (int)(entity->att.prevLogout() - entity->att.prevLogin()));
	// DEBUG_LOG("Total online time: %d", entity->att.dayEvent(3));

	entity->att.ToPB(proto->add_data());
	PlayerInfo *add = proto->add_info();
	const PlayerInfo *info = AccountPool_IDToAccount(entity->id);
	if (info != NULL)
		*add = *info;
	else
		DEBUG_LOGERROR("Failed to get account info");
}

void PlayerEntity_Logout(struct PlayerEntity *entity, bool notice) {
	if (!PlayerEntity_IsValid(entity))
		return;

	int32_t id = entity->id;
	if (notice) {
		static NetProto_Logout logout;
		logout.Clear();
		logout.set_id(id);
		GCAgent_SendProtoToAllClientsInSameScene(Movement_Att(entity->component.movement)->mapID(), entity, &logout);
	}

	static DCProto_SaveRoleData saveRoleData;
	saveRoleData.Clear();
	PlayerEntity_Save(entity, &saveRoleData);
	GCAgent_SendProtoToDCAgent(&saveRoleData);

	// DEBUG_LOGRECORD("Role logout, point: %p, roleID: %lld, total roles: %d", entity, (long long)entity->att.att().baseAtt().roleID(), PlayerPool_TotalCount());
	// Debug_LogCallStack();

	if (notice) {
		Component_Destroy(&entity->component);
		if (entity->accountStatus == ACCOUNT_DROP)
			GCAgent_SendOrderToNetAgent(ORDER_COLLECT_ID, id, -1);
	}

	{
		// faction
		FactionPool_Signin(entity->att.att().baseAtt().roleID(), false);
	}


	{
		static char buf[CONFIG_FIXEDARRAY];
		char *index = buf;
		if (Config_ScribeHost() != NULL) {
			memset(buf, 0, sizeof(buf) / sizeof(char));
			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)entity->att.att().baseAtt().roleID());
			index += strlen(index);
			for (int i = 0; i < entity->att.att().fightAtt().skills_size(); ++i) {
				if (entity->att.att().fightAtt().skills(i).id() != -1) {
					SNPRINTF2(buf, index, "\",\"skillID%d\":\"%d", entity->att.att().fightAtt().skills(i).id(), entity->att.att().fightAtt().skills(i).level());
					index += strlen(index);
				}
			}
			SNPRINTF2(buf, index, "\",\"time\":\"%d\"}", (int)Time_TimeStamp());
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(entity)), PlayerEntity_ID(entity), "Skills", buf));
		}
	}

}

bool PlayerEntity_DoWorldChat(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return false;

	int64_t cur = Time_ElapsedTime();
	if (cur - entity->lastWorldChatTime <= Config_WorldChatInterval())
		return false;

	entity->lastWorldChatTime = cur;
	return true;
}

bool PlayerEntity_DoSecretChat(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return false;

	int64_t cur = Time_ElapsedTime();
	if (cur - entity->lastSecretChatTime <= Config_SecretChatInterval())
		return false;

	entity->lastSecretChatTime = cur;
	return true;
}

int PlayerEntity_FriendsCount(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return 0;

	int ret = 0;
	for (int i = 0; i < entity->att.friends_size(); i++) {
		if (entity->att.friends(i).roleID() >= 0)
			ret++;
	}
	return ret;
}

int PlayerEntity_FansCount(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity))
		return 0;

	int ret = 0;
	for (int i = 0; i < entity->att.fans_size(); i++) {
		if (entity->att.fans(i).roleID() >= 0)
			ret++;
	}
	return ret;
}

bool PlayerEntity_AddFriend(struct PlayerEntity *entity, int64_t roleID, const char *name, ProfessionInfo::Type professionType) {
	if (!PlayerEntity_IsValid(entity))
		return false;

	FriendInfo *info = NULL;
	for (int i = 0; i < entity->att.friends_size(); i++) {
		if (entity->att.friends(i).roleID() == roleID)
			return false;

		if (entity->att.friends(i).roleID() == -1) {
			if (info == NULL)
				info = entity->att.mutable_friends(i);
		}
	}
	if (info == NULL) {
		DEBUG_LOG("the friend has full");
		return false;
	}

	FriendInfo *infoFan = NULL;
	PlayerEntity *fan = PlayerEntity_PlayerByRoleID(roleID);
	if (fan == entity) {
		static NetProto_Error error;
		error.set_content(Config_Words(38));
		GCAgent_SendProtoToClients(&entity->id, 1, &error);
		return false;
	}
	if (fan != NULL) {
		for (int i = 0; i < fan->att.fans_size(); i++) {
			if (fan->att.fans(i).roleID() == entity->att.att().baseAtt().roleID())
				return false;

			if (fan->att.fans(i).roleID() == -1) {
				if (infoFan == NULL)
					infoFan = fan->att.mutable_fans(i);
			}
		}
		if (infoFan == NULL) {
			static NetProto_Error error;
			error.set_content(Config_Words(39));
			GCAgent_SendProtoToClients(&entity->id, 1, &error);
			DEBUG_LOG("the rileID = %ld fan has full", roleID);
			return false;
		}

		info->set_roleID(roleID);
		info->set_name(name);
		info->set_professionType(professionType);
		info->set_loveHeart(false);

		infoFan->set_roleID(entity->att.att().baseAtt().roleID());
		infoFan->set_name(entity->att.att().baseAtt().name());
		infoFan->set_professionType(entity->att.att().baseAtt().professionType());
		infoFan->set_loveHeart(false);
	}else {
		static DCProto_AddOutLineFriends proto;
		proto.Clear();
		proto.set_roleID1(entity->att.att().baseAtt().roleID());
		proto.set_roleID2(roleID);
		proto.set_name(entity->att.att().baseAtt().name());
		proto.set_professionType((::PB_ProfessionInfo_Type)entity->att.att().baseAtt().professionType());
		GCAgent_SendProtoToDCAgent(&proto);
		return false;
	}
	return true;
}

bool PlayerEntity_AddOutLineFriend(struct PlayerEntity *entity, int64_t roleID, const char *name, ProfessionInfo::Type professionType) {
	if (!PlayerEntity_IsValid(entity))
		return false;

	FriendInfo *info = NULL;
	for (int i = 0; i < entity->att.friends_size(); i++) {
		if (entity->att.friends(i).roleID() == roleID)
			return false;

		if (entity->att.friends(i).roleID() == -1) {
			if (info == NULL)
				info = entity->att.mutable_friends(i);
		}
	}
	if (info == NULL) {
		DEBUG_LOG("the friend has full");
		return false;
	}

	info->set_roleID(roleID);
	info->set_name(name);
	info->set_professionType(professionType);
	info->set_loveHeart(false);

	static NetProto_AddFriend addFriend;
	addFriend.Clear();
	addFriend.set_roleID(roleID);
	addFriend.set_name(name);
	addFriend.set_professionType((::PB_ProfessionInfo_Type)professionType);
	int32_t id =  PlayerEntity_ID(entity);
	if (-1 == id) 
		return false;
	GCAgent_SendProtoToClients(&id, 1, &addFriend);
	return true;
}

bool PlayerEntity_DelFriend(struct PlayerEntity *entity, int64_t roleID, bool flag) {
	if (!PlayerEntity_IsValid(entity))
		return false;

	PlayerEntity *fan = PlayerEntity_PlayerByRoleID(roleID);
	if (fan == entity) {
		DEBUG_LOG("the entity and the fan is the same one!");
		return false;
	}

	FriendInfo *info = NULL;
	if (flag) {
		// del mine friend;
		for (int i = 0; i < entity->att.friends_size(); i++) {
			info = entity->att.mutable_friends(i);
			if (info->roleID() == roleID) {
				info->set_roleID(-1);
				info->set_name("");
				info->set_loveHeart(false);
				break;
			}
		}
		// del friend fan
		if (fan != NULL) {
			for (int i = 0; i < fan->att.fans_size(); i++) {
				info = fan->att.mutable_fans(i);
				if (info->roleID() == entity->att.att().baseAtt().roleID()) {
					info->set_roleID(-1);
					info->set_name("");
					info->set_loveHeart(false);
					break;
				}
			}
		}
	} else {
		for (int i = 0; i < entity->att.fans_size(); ++i) {
			info = entity->att.mutable_fans(i);
			if (info->roleID() == roleID) {
				info->set_roleID(-1);
				info->set_name("");
				info->set_loveHeart(false);
				break;
			}
		}
		if (fan != NULL) {
			for (int i = 0; i < fan->att.friends_size(); ++i) {
				info = fan->att.mutable_friends(i);
				if (info->roleID() == entity->att.att().baseAtt().roleID()) {
					info->set_roleID(-1);
					info->set_name("");
					info->set_loveHeart(false);
					break;
				}
			}
		}
	}

	if (fan == NULL) {
		static DCProto_LoadPlayerAtt loadPlayerAtt;
		loadPlayerAtt.set_roleID(roleID);
		loadPlayerAtt.set_flag(flag);
		loadPlayerAtt.set_roleID2(entity->att.att().baseAtt().roleID());
		GCAgent_SendProtoToDCAgent(&loadPlayerAtt);
	}
	return true;;
}

int PlayerEntity_FindEmptyMail(PlayerAtt *att) {
	int ret = -1;
	for (int i = 0; i < att->mails_size(); i++) {
		if (att->mails(i).time() == 0) {
			ret = i;
			break;
		}
	}
	if (ret == -1) {
		int64_t earliest = LLONG_MAX;
		for (int i = 0; i < att->mails_size(); i++) {
			if (att->mails(i).time() < earliest) {
				earliest = att->mails(i).time();
				ret = i;
			}
		}
		assert(ret != -1);
	}
	return ret;
}
int PlayerEntity_FindEmptyMail(PB_PlayerAtt *att) {
	int ret = -1;
	for (int i = 0; i < att->mails_size(); i++) {
		if (att->mails(i).time() == 0) {
			ret = i;
			break;
		}
	}
	if (ret == -1) {
		int64_t earliest = LLONG_MAX;
		for (int i = 0; i < att->mails_size(); i++) {
			if (att->mails(i).time() < earliest) {
				earliest = att->mails(i).time();
				ret = i;
			}
		}
		assert(ret != -1);
	}
	return ret;
}

int PlayerEntity_AddMail(struct PlayerEntity *entity, PB_MailInfo *mail) {
	if (!PlayerEntity_IsValid(entity) || mail == NULL)
		return -1;

	int equipID = -1;
	if (mail->item().type() == PB_ItemInfo::EQUIPMENT) {
		equipID = Item_AddEquip(entity->component.item, EquipmentInfoManager_EquipmentInfo(mail->item().id()));
		if (equipID == -1)
			return -1;
	}

	mail->set_time((int64_t)Time_TimeStamp());
	mail->set_read(false);
	if (equipID != -1)
		mail->mutable_item()->set_id(equipID);

	int index = PlayerEntity_FindEmptyMail(&entity->att);
	PlayerEntity_DelMail(entity, index, true);
	MailInfo *info = entity->att.mutable_mails(index);
	info->FromPB(mail);
	return index;
}

int64_t PlayerEntity_AddMail(struct PlayerEntity *entity, ItemInfo::Type type, int64_t id, int count, const char *title, const char *content, int subrmb, bool isRmb) {
	if (!PlayerEntity_IsValid(entity))
		return -1;
	if (type != ItemInfo::NONE && id == -1)
		return -1;

	int equipID = -1;
	if (type == ItemInfo::EQUIPMENT) {
		equipID = Item_AddEquip(entity->component.item, EquipmentInfoManager_EquipmentInfo(id));
		if (equipID == -1)
			return -1;
	}
	int index = PlayerEntity_FindEmptyMail(&entity->att);
	PlayerEntity_DelMail(entity, index, true);
	MailInfo *info = entity->att.mutable_mails(index);
	info->set_title(title);
	info->set_content(content);
	info->set_sender(Config_Words(1));
	info->set_time((int64_t)Time_TimeStamp());
	info->set_read(false);
	info->mutable_item()->set_type(type);
	if (equipID != -1)
		info->mutable_item()->set_id(equipID);
	else
		info->mutable_item()->set_id(id);
	info->mutable_item()->set_count(count);
	info->set_rmb(subrmb);
	info->set_isRmb(isRmb);

	static NetProto_SendMail sm;
	sm.Clear();
	sm.set_receiver(entity->att.att().baseAtt().roleID());
	sm.set_pos(index);
	info->ToPB(sm.mutable_mail());
	GCAgent_SendProtoToClients(&entity->id, 1, &sm);
	return info->item().id();
}

void PlayerEntity_DelMail(struct PlayerEntity *entity, int32_t id, bool delItem) {
	if (!PlayerEntity_IsValid(entity))
		return;
	if (id < 0 || id >= entity->att.mails_size())
		return;

	MailInfo *info = entity->att.mutable_mails(id);
	if (info->time() != 0) {
		info->set_time(0);
		if (info->item().type() == ItemInfo::EQUIPMENT)
			Item_DelEquip(entity->component.item, info->item().id());
	}
}

void PlayerEntity_DelFromMail(struct PlayerEntity *entity, int32_t select, int64_t id) {
	if (!PlayerEntity_IsValid(entity))
		return;

	DEBUG_LOG("propID == %d", id);
	DEBUG_LOG("select == %d", select);
	ItemInfo::Type type;
	if (select == 1) {
		type = ItemInfo::EQUIPMENT;
	} else if (select == 2) {
		type = ItemInfo::GOODS;
	} else {
		return;
	}
	DEBUG_LOG("type == %d", type);

	for (int i = 0; i < entity->att.mails_size(); i++) {
		MailInfo *info = entity->att.mutable_mails(i);
		if (info->time() != 0) {
			if (info->item().type() == type && info->item().id() == id) {
				info->set_time(0);
				if (type == ItemInfo::EQUIPMENT)
					Item_DelEquip(entity->component.item, info->item().id());
			}
		}
	}
}

void PlayerEntity_ReadMail(struct PlayerEntity *entity, int32_t id) {
	if (!PlayerEntity_IsValid(entity))
		return;
	if (id < 0 || id >= entity->att.mails_size())
		return;

	entity->att.mutable_mails(id)->set_read(true);
}

int PlayerEntity_GetMailItem(struct PlayerEntity *entity, int32_t id) {
	if (!PlayerEntity_IsValid(entity))
		return -1;
	if (id < 0 || id >= entity->att.mails_size())
		return -2;

	MailInfo *info = entity->att.mutable_mails(id);
	if (info->time() == 0 || (info->item().type() == ItemInfo::NONE && info->rmb() <= 0))
		return -3;

	switch(info->item().type()) {
		case ItemInfo::RIDES:
			{
				int32_t remain = Item_RidesEmptyCount(entity->component.item);
				if(remain < info->item().count())
				{
					return -100;
				}

				int re = Item_GenRides_Whole(entity->component.item, info->item().id(), info->item().count(), NULL);
				if(re < 0)
					DEBUG_LOGERROR("mail get rides error->%d", re);
			}
			break;
		case ItemInfo::EQUIPMENT:
			{
				int32_t empty = Item_EmptyPos(entity->component.item, ItemPackage::EQUIPMENT, -1);
				if (empty == -1) {
					DEBUG_LOG("Failed to get mail equip, id: %d, count: %d", (int)info->item().id(), info->item().count());
					return -4;
				}

				/*
				   Item_AddToPackage(entity->component.item, info->item().type(), info->item().id(), info->item().count());
				   info->mutable_item()->set_type(ItemInfo::NONE);
				   */

				Item_AddToPackage(entity->component.item, &info->item(), empty, NULL);

				int64_t roleID = PlayerEntity_RoleID(entity);
				char ch[1024];
				SNPRINTF1(ch, "19-%d-%d", (int)info->item().id(), info->item().count());
				DCProto_SaveSingleRecord saveRecord;
				saveRecord.set_mapID(-23);
				saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
				saveRecord.mutable_record()->mutable_role()->set_name(ch);
				saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
				GCAgent_SendProtoToDCAgent(&saveRecord);

				static NetProto_GetItem getItem;
				getItem.Clear();
				info->item().ToPB(getItem.mutable_item());
				getItem.set_pos(empty);
				GCAgent_SendProtoToClients(&entity->id, 1, &getItem);
				info->mutable_item()->set_type(ItemInfo::NONE);
			}
			break;
		case ItemInfo::GOODS:
			{
				if (!Item_IsEnough(entity->component.item, info->item().type(), info->item().id(), info->item().count())) {
					DEBUG_LOG("Failed to get mail goods, id: %d, count: %d, empty: %d", (int)info->item().id(), info->item().count(), Item_EmptyPos(entity->component.item, ItemPackage::GOODS, info->item().id()));
					return -5;
				}

				Item_AddToPackage(entity->component.item, info->item().type(), info->item().id(), info->item().count());

				int64_t roleID = PlayerEntity_RoleID(entity);
				char ch[1024];
				SNPRINTF1(ch, "20-%d-%d", (int)info->item().id(), info->item().count());
				DCProto_SaveSingleRecord saveRecord;
				saveRecord.set_mapID(-23);
				saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
				saveRecord.mutable_record()->mutable_role()->set_name(ch);
				saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
				GCAgent_SendProtoToDCAgent(&saveRecord);

				/*
				   ItemPackage::Begin begin;
				   if (GoodsInfoManager_IsGem(info->item().id()))
				   begin = ItemPackage::GEM;
				   else
				   begin = ItemPackage::GOODS;

				   int count = info->item().count();
				   while (count > 0) {
				   int32_t empty = Item_EmptyPos(entity->component.item, begin, info->item().id());
				   if (empty == -1)
				   break;

				   info->mutable_item()->set_count(count);

				   int last = Item_AddToPackage(entity->component.item, &info->item(), empty, NULL);
				   if (last > 0)
				   info->mutable_item()->set_count(count - last);

				   static NetProto_GetItem getItem;
				   getItem.Clear();
				   info->item().ToPB(getItem.mutable_item());
				   getItem.set_pos(empty);
				   GCAgent_SendProtoToClients(&entity->id, 1, &getItem);

				   count = last;
				   }
				   */

				Mission_CompleteTarget(entity->component.mission, MissionTarget::GETGOODS, info->item().id(), info->item().count());
				info->mutable_item()->set_type(ItemInfo::NONE);
			}
			break;
		default:
			break;
	}

	if (info->rmb() > 0) {
	
		if (info->isRmb()) {
			bool first = false;
			if (Config_DoubleRecharge(info->rmb()) && !(entity->att.fixedEvent(15) & (1 << Config_DoubleRechargeLevel(info->rmb())))) {
				first = true;
			}
			Item_ModifyRMB(entity->component.item, info->rmb(), true, -1, 0, 0);

			static NetProto_GetRes gr;
			gr.Clear();
			PB_ItemInfo *item = gr.add_items();
			item->set_type(PB_ItemInfo::RMB);
			item->set_count(info->rmb());
			if (first) {
				item = gr.add_items();
				item->set_type(PB_ItemInfo::SUBRMB);
				item->set_count(info->rmb());
			}
			GCAgent_SendProtoToClients(&entity->id, 1, &gr);
		}else {
			Item_ModifyRMB(entity->component.item, info->rmb(), false, -6, 0, 0);
		}

	/*	
		{
		// as subrmb
		Item_ModifyRMB(entity->component.item, info->rmb(), false, -6, 0, 0);

		}
		

		{
			// as rmb
			bool first = false;
			if (Config_DoubleRecharge(info->rmb()) && !(entity->att.fixedEvent(15) & (1 << Config_DoubleRechargeLevel(info->rmb())))) {
				first = true;
			}
			Item_ModifyRMB(entity->component.item, info->rmb(), true, -1, 0, 0);

			static NetProto_GetRes gr;
			gr.Clear();
			PB_ItemInfo *item = gr.add_items();
			item->set_type(PB_ItemInfo::RMB);
			item->set_count(info->rmb());
			if (first) {
				item = gr.add_items();
				item->set_type(PB_ItemInfo::SUBRMB);
				item->set_count(info->rmb());
			}
			GCAgent_SendProtoToClients(&entity->id, 1, &gr);
		}
*/
		info->set_rmb(0);
	}

	PlayerEntity_DelMail(entity, id, false);
	return 0;
}

const struct StrongTable * PlayerEntity_StrongTable(int level) {
	if (level < 0 || level >= (int)package.strongTable.size())
		return NULL;
	return &package.strongTable[level];
}

int PlayerEntity_MaxStrongLevel() {
	return (int)package.strongTable.size();
}

bool PlayerEntity_EquipNeedNotice(int32_t id) {
	for (size_t i = 0; i < package.noticeEquip.size(); i++)
		if (package.noticeEquip[i] == id)
			return true;
	return false;
}

bool PlayerEntity_GoodsNeedNotice(int32_t id) {
	for (size_t i = 0; i < package.noticeGoods.size(); i++)
		if (package.noticeGoods[i] == id)
			return true;
	return false;
}

static void NoticeStrong(struct PlayerEntity *entity, const EquipmentInfo *info, const EquipAsset *asset) {
	if (!Config_CanNoticeStrong(info, asset))
		return;

	static NetProto_Message message;
	message.Clear();
	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, Config_Words(2), entity->component.baseAtt->name(), Config_EquipColorStr(info->colorType()), info->name().c_str(), Config_EquipColorStr(info->colorType()), asset->strongLevel());
	message.set_content(buf);
	message.set_count(1);
	GCAgent_SendProtoToAllClients(&message);
}

static int64_t Equip(struct PlayerEntity *entity, bool onBody, int32_t id, const EquipmentInfo **info, EquipAsset **asset) {
	int64_t ret = -1;
	if (onBody) {
		const EquipmentAtt *att = Equipment_Att(entity->component.equipment);
		if (id < 0 || id >= att->equipments_size())
			return -1;
		if (att->equipments(id) == -1)
			return -1;
		*info = Item_EquipInfo(entity->component.item, att->equipments(id));
		*asset = Item_EquipAsset(entity->component.item, att->equipments(id));
		ret = att->equipments(id);
	} else {
		const ItemInfo *item = Item_PackageItem(entity->component.item, id);
		if (item == NULL || item->type() != ItemInfo::EQUIPMENT)
			return -1;
		*info = Item_EquipInfo(entity->component.item, item->id());
		*asset = Item_EquipAsset(entity->component.item, item->id());
		ret = item->id();
	}
	return ret;
}

int PlayerEntity_Strong(struct PlayerEntity *entity, bool onBody, int32_t id, bool protect) {
	if (!PlayerEntity_IsValid(entity))
		return -1;
	const EquipmentInfo *equip = NULL;
	EquipAsset *asset = NULL;
	int64_t equipID = Equip(entity, onBody, id, &equip, &asset);
	if (equipID == -1)
		return -1;

	if (asset->strongLevel() >= PlayerEntity_MaxStrongLevel())
		return -1;
	
	const FightAtt * fightAtt = Fight_Att(entity->component.fight);
	if (asset->strongLevel() >= fightAtt->level())
		return -1;

	const struct StrongTable *strongTable = PlayerEntity_StrongTable(asset->strongLevel());
	assert(strongTable != NULL);

	int32_t cost = strongTable->money;

	int strongPercent = GmSet_GetPercent(GmSet_Type_STRONG);
	if(strongPercent != -1)
		cost = int(cost * strongPercent / 100.0f);

	if (cost > Item_Package(entity->component.item)->money())
		return -1;
	if (!Item_HasItem(entity->component.item, ItemInfo::GOODS, Config_StrongStone(), strongTable->goods))
		return -1;
	if (protect && strongTable->protect && !Item_HasItem(entity->component.item, ItemInfo::GOODS, Config_StrongProtect(), strongTable->protect))
		return -1;

	static char buf[CONFIG_FIXEDARRAY];
	char *index = buf;
	if (Config_ScribeHost() != NULL) {
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipID\":\"%lld", (long long int)equip->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipName\":\"%s", equip->name().c_str());
		index += strlen(index);
	}
	Item_ModifyMoney(entity->component.item, -cost);
	Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, Config_StrongStone(), strongTable->goods);

	if (Config_ScribeHost() != NULL) {
		SNPRINTF2(buf, index, "\",\"itemID1\":\"%d", (int)Config_StrongStone());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"itemCount1\":\"%d", cost);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"strongLevelFirst\":\"%d", asset->strongLevel());
		index += strlen(index);
	}
	Mission_CompleteTarget(entity->component.mission, MissionTarget::STRONG, -1, -1);

	if (protect) {
		Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, Config_StrongProtect(), strongTable->protect);
		if (Config_ScribeHost() != NULL) {
			SNPRINTF2(buf, index, "\",\"itemID2\":\"%d", (int)Config_StrongProtect());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"itemCount2\":\"%d", strongTable->protect);
			index += strlen(index);
		}
	}

	int ret = -1;
	if (Time_CanPass(strongTable->strongSuccess)) { // Success
		if (onBody)
			Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)id);
		asset->set_strongLevel(asset->strongLevel() + 1);
		if (onBody)
			Equipment_Wear(entity->component.equipment, equipID);
		NoticeStrong(entity, equip, asset);
		if (Config_ScribeHost() != NULL) {
			SNPRINTF2(buf, index, "\",\"strongLevelLast\":\"%d", asset->strongLevel());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"success\":\"true\"}");
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "StrongEquip", buf));
		}
		ret = 0;

		if (Config_InActiveTime(22)) {
			int value = entity->att.fixedEvent(15);
			if (!(value & 0x800)) {
				if (asset->strongLevel() == 15) {
					const EquipmentInfo * weapon = EquipmentInfoManager_EquipmentInfo(asset->mode());
					if (weapon != NULL) {
						if (weapon->type() == EquipmentInfo::WEAPON) {
							Event_AwardStrongWeapon(entity);
							PlayerEntity_SetFixedEvent(entity, 15, value | 0x800);
						}
					}
				}
			}
		}
	} else if (Time_CanPass(strongTable->strongFail)) { // Failure
		if (asset->strongLevel() >= 1) {
			if (protect) {
				//Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, Config_StrongProtect(), strongTable->protect);
			} else {
				if (onBody)
					Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)id);
				asset->set_strongLevel(asset->strongLevel() - 1);
				if (onBody)
					Equipment_Wear(entity->component.equipment, equipID);
			}
		}
		if (Config_ScribeHost() != NULL) {
			SNPRINTF2(buf, index, "\",\"strongLevelLast\":\"%d", asset->strongLevel());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"success\":\"false\"}");
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "StrongEquip", buf));
		}
		ret = 2;
	} else { // Nothing
		if (Config_ScribeHost() != NULL) {
			SNPRINTF2(buf, index, "\",\"strongLevelLast\":\"%d", asset->strongLevel());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"success\":\"false\"}");
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "StrongEquip", buf));
		}
		ret = 1;
	}

	Config_AdjustGemHole(asset);
	return ret;
}

int PlayerEntity_ClearStrong(struct PlayerEntity *entity, bool onBody, int32_t id) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	const EquipmentInfo *equip = NULL;
	EquipAsset *asset = NULL;
	int64_t equipID = Equip(entity, onBody, id, &equip, &asset);
	if (equipID == -1)
		return -1;

	if (onBody)
		Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)id);
	asset->set_strongLevel(0);
	if (onBody)
		Equipment_Wear(entity->component.equipment, equipID);

	return 0;
}

int PlayerEntity_Mount(struct PlayerEntity *entity, bool onBody, int32_t id, int32_t mountPos, int32_t gemPos) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	const ItemInfo *curItem = Item_PackageItem(entity->component.item, gemPos);
	if (curItem == NULL || curItem->type() != ItemInfo::GOODS)
		return -1;
	const GoodsInfo *gem = GoodsInfoManager_GoodsInfo((int32_t)curItem->id());
	if (gem == NULL || gem->type() != GoodsInfo::GEM)
		return -1;

	const EquipmentInfo *equip = NULL;
	EquipAsset *asset = NULL;
	int64_t equipID = Equip(entity, onBody, id, &equip, &asset);
	if (equipID == -1)
		return -1;

	if (mountPos < 0 || mountPos >= asset->gemModel_size())
		return -1;

	int gemType = asset->gemType(mountPos);
	if (gemType != gem->arg(0))
		return -1;

	int prevGem = asset->gemModel(mountPos);
	if (prevGem < -1)
		return -1;

	if(prevGem >= 0)
	{
		const GoodsInfo *pGem = GoodsInfoManager_GoodsInfo((int32_t)prevGem);
		if(pGem == NULL || pGem->type() != GoodsInfo::GEM)
			return -1;
		int cost = Config_UnmountCost(pGem->arg(3));
		if (entity->att.itemPackage().money() < cost)
			return -1;
		Item_ModifyMoney(entity->component.item, -cost);
	}

	static char buf[CONFIG_FIXEDARRAY];
	char *index = buf;
	if (Config_ScribeHost() != NULL) {
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipID\":\"%lld", (long long int)equip->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipName\":\"%s", equip->name().c_str());
		index += strlen(index);
	}
	if (onBody)
		Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)id);
	asset->set_gemModel(mountPos, (int32_t)curItem->id());
	if (onBody)
		Equipment_Wear(entity->component.equipment, equipID);

	Item_DelFromPackage(entity->component.item, gemPos, 1);
	if (prevGem >= 0)
	{
		Item_AddToPackage(entity->component.item, ItemInfo::GOODS, prevGem, 1);

		int64_t roleID = PlayerEntity_RoleID(entity);
		char ch[1024];
		SNPRINTF1(ch, "21-%d-%d", prevGem, 1);
		DCProto_SaveSingleRecord saveRecord;
		saveRecord.set_mapID(-23);
		saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
		saveRecord.mutable_record()->mutable_role()->set_name(ch);
		saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
		GCAgent_SendProtoToDCAgent(&saveRecord);
	}

	if (Config_ScribeHost() != NULL) {
		SNPRINTF2(buf, index, "\",\"itemID1\":\"%d", gemPos);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"itemCount1\":\"1\"}");
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "Mount", buf));
	}

	return 0;
}

int PlayerEntity_Unmount(struct PlayerEntity *entity, bool onBody, int32_t id, int32_t mountPos) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	const EquipmentInfo *equip = NULL;
	EquipAsset *asset = NULL;
	int64_t equipID = Equip(entity, onBody, id, &equip, &asset);
	if (equipID == -1)
		return -1;

	if (mountPos < 0 || mountPos >= asset->gemModel_size())
		return -1;

	int32_t gem = asset->gemModel(mountPos);
	if (gem < 0)
		return -1;
	const GoodsInfo *info = GoodsInfoManager_GoodsInfo(gem);
	assert(info != NULL);

	int cost = Config_UnmountCost(info->arg(3));
	if (entity->att.itemPackage().money() < cost)
		return -1;
	Item_ModifyMoney(entity->component.item, -cost);

	Item_AddToPackage(entity->component.item, ItemInfo::GOODS, gem, 1);

	int64_t roleID = PlayerEntity_RoleID(entity);
	char ch[1024];
	SNPRINTF1(ch, "22-%d-%d", gem, 1);
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

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipID\":\"%lld", (long long int)equip->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipName\":\"%s", equip->name().c_str());
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "UnMount", buf));
	}

	if (onBody)
		Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)id);
	asset->set_gemModel(mountPos, -1);
	if (onBody)
		Equipment_Wear(entity->component.equipment, equipID);

	return 0;
}

int PlayerEntity_EnhanceDelta(struct PlayerEntity *entity, bool onBody, int32_t id, bool ten, int32_t *delta, size_t size) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	const EquipmentInfo *equip = NULL;
	EquipAsset *asset = NULL;
	int64_t equipID = Equip(entity, onBody, id, &equip, &asset);
	if (equipID == -1)
		return -1;

	if (!Item_HasItem(entity->component.item, ItemInfo::GOODS, Config_EnhanceStone(), ten ? 10 : 1))
		return -1;

	int valid[EquipAsset::ENHANCEDELTA_SIZE];
	int count = 0;
	for (int i = 0; i < asset->enhanceDelta_size(); i++) {
		if (asset->enhanceDelta(i) < equip->enhanceLimit(i))
			valid[count++] = i;
	}
	if (count <= 0)
		return -1;

	for (int i = 0; i < EquipAsset::ENHANCEDELTA_SIZE; i++)
		entity->enhanceDelta[i] = 0;
	for (int j = 0; j < (ten ? 10 : 1); j++) {
		for (int i = 0; i < count; i++) {
			int index = valid[i];
			int v = Config_EnhanceDelta(equip->enhanceType(index));
			entity->enhanceDelta[index] += v;
		}
	}
	for (int i = 0; i < EquipAsset::ENHANCEDELTA_SIZE; i++) {
		if (asset->enhanceDelta(i) + entity->enhanceDelta[i] < 0)
			entity->enhanceDelta[i] = -asset->enhanceDelta(i);
		else if (asset->enhanceDelta(i) + entity->enhanceDelta[i] > equip->enhanceLimit(i))
			entity->enhanceDelta[i] =  equip->enhanceLimit(i) - asset->enhanceDelta(i);
	}

	for (int i = 0; i < EquipAsset::ENHANCEDELTA_SIZE; i++) {
		if (i >= (int)size)
			return -1;
		delta[i] = entity->enhanceDelta[i];
	}

	Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, Config_EnhanceStone(), ten ? 10 : 1);
	entity->enhanceEquip = equipID;

	Mission_CompleteTarget(entity->component.mission, MissionTarget::ENHANCE, ten ? 10 : 1, 0);
	return EquipAsset::ENHANCEDELTA_SIZE;
}

int PlayerEntity_Enhance(struct PlayerEntity *entity, bool onBody, int32_t id) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	if (entity->enhanceEquip == -1)
		return -1;

	// const EquipmentInfo *equip = NULL;
	// EquipAsset *asset = NULL;
	// int64_t equipID = Equip(entity, onBody, id, &equip, &asset);
	int64_t equipID = entity->enhanceEquip;
	if (equipID == -1)
		return -1;
	EquipAsset *asset = Item_EquipAsset(entity->component.item, (int32_t)equipID);
	const EquipmentInfo *equip = Item_EquipInfo(entity->component.item, (int32_t)equipID);
	if (asset == NULL || equip == NULL)
		return -1;

	if (entity->enhanceEquip != equipID)
		return -1;

	static char buf[CONFIG_FIXEDARRAY];
	char *index = buf;
	if (Config_ScribeHost() != NULL) {
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipID\":\"%lld", (long long int)equip->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"equipName\":\"%s", equip->name().c_str());
		index += strlen(index);
	}
	if (onBody)
		Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)id);
	for (int i = 0; i < EquipAsset::ENHANCEDELTA_SIZE; i++) {
		if (Config_ScribeHost() != NULL) {
			SNPRINTF2(buf, index, "\",\"preEquipAttr%d\":\"%d", i + 1,asset->enhanceDelta(i));
			index += strlen(index);
		}
		asset->set_enhanceDelta(i, asset->enhanceDelta(i) + entity->enhanceDelta[i]);
		if (Config_ScribeHost() != NULL) {
			SNPRINTF2(buf, index, "\",\"finalEquipAttr%d\":\"%d", i + 1,asset->enhanceDelta(i));
			index += strlen(index);
		}
	}
	if (Config_ScribeHost() != NULL) {
		SNPRINTF2(buf, index, "\"}");
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "Enhance", buf));
	}
	if (onBody)
		Equipment_Wear(entity->component.equipment, equipID);

	entity->enhanceEquip = -1;
	return 0;
}

int PlayerEntity_Inherit(struct PlayerEntity *entity, bool parentBody, int32_t parentID, bool childBody, int32_t childID, bool useRMB, bool useStone) {
	if (!PlayerEntity_IsValid(entity))
		return -1;
	const EquipmentInfo *parent = NULL;
	EquipAsset *parentAsset = NULL;
	int64_t parentEquipID = Equip(entity, parentBody, parentID, &parent, &parentAsset);
	if (parentEquipID == -1)
		return -1;
	const EquipmentInfo *child = NULL;
	EquipAsset *childAsset = NULL;
	int64_t childEquipID = Equip(entity, childBody, childID, &child, &childAsset);
	if (childEquipID == -1)
		return -1;
	if (parentEquipID == childEquipID)
		return -1;

	if (parent->type() != child->type())
		return -1;

	float inheritRate = 1.0f;
	if(!useStone) {
		inheritRate = Config_EquipInheritPercent() / 100.0f;
	}

	if (floor(parentAsset->strongLevel() * inheritRate) < childAsset->strongLevel())
		return -1;
	for (int i = 0; i < parentAsset->gemModel_size(); i++)
		if (parentAsset->gemModel(i) >= 0)
			return -1;
	for (int i = 0; i < EquipAsset::ENHANCEDELTA_SIZE; i++)
		if (parent->enhanceLimit(i) > child->enhanceLimit(i))
			return -1;

	int64_t goods = -1;
	int count = 0;
	Config_InheritStone(&goods, parent, parentAsset, &count);
	DEBUG_LOG("goodsid: %d", goods);
	int cur = Item_Count(entity->component.item, ItemInfo::GOODS, goods);

	static char buf[CONFIG_FIXEDARRAY];
	char *index = buf;
	if (Config_ScribeHost() != NULL) {
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
		index += strlen(index);
	}
	
	if(useStone){
		if (cur >= count) {
			Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, goods, count);
			if (Config_ScribeHost() != NULL) {
				SNPRINTF2(buf, index, "\",\"itemID1\":\"%d", (int)goods);
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"itemCount1\":\"%d", count);
				index += strlen(index);
			}
		} else if (useRMB) {
			int need = count - cur;
			const GoodsInfo *info = GoodsInfoManager_GoodsInfo(goods);
			assert(info != NULL);
			int cost = info->rmb() * need;
			//const ItemPackage *package = Item_Package(entity->component.item);
			//		if (package->rmb() < cost)
			if (!Item_HasRMB(entity->component.item, cost))
				return -1;
			Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, goods, count);
			Item_ModifyRMB(entity->component.item, -cost, false, 12, 0, 0);
			if (Config_ScribeHost() != NULL) {
				SNPRINTF2(buf, index, "\",\"itemID1\":\"%d", (int)goods);
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"itemCount1\":\"%d", count);
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"YuanBao\":\"%d", cost);
				index += strlen(index);
			}
		} else {
			return -1;
		}
	}

	if (childBody)
		Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)childID);
	childAsset->set_strongLevel(floor(parentAsset->strongLevel() * inheritRate));

	if (Config_ScribeHost() != NULL) {
		SNPRINTF2(buf, index, "\",\"parentEquipName\":\"%s", parent->name().c_str());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"childEquipName\":\"%s", child->name().c_str());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"parentEquipID\":\"%lld", (long long int)parent->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"childEquipID\":\"%lld", (long long int)child->id());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"parentEquipStrongLevel\":\"%d", (int)floor(parentAsset->strongLevel() * inheritRate));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"childEquipStrongLevel\":\"%d", childAsset->strongLevel());
		index += strlen(index);
	}
	for (int i = 0; i < EquipAsset::ENHANCEDELTA_SIZE; i++) {
		if (floor(parentAsset->enhanceDelta(i) * inheritRate) > childAsset->enhanceDelta(i)) {
			childAsset->set_enhanceDelta(i, floor(parentAsset->enhanceDelta(i) * inheritRate));
			if (Config_ScribeHost() != NULL) {
				SNPRINTF2(buf, index, "\",\"parentEquipAttr%d\":\"%d", i + 1, (int)floor(parentAsset->enhanceDelta(i) * inheritRate));
				index += strlen(index);
				SNPRINTF2(buf, index, "\",\"childEquipAttr%d\":\"%d", i + 1, childAsset->enhanceDelta(i));
				index += strlen(index);
			}
		}
	}
	Config_AdjustGemHole(childAsset);
	if (childBody)
		Equipment_Wear(entity->component.equipment, childEquipID);

	if (parentBody)
		Equipment_Unwear(entity->component.equipment, (EquipmentInfo::Type)parentID);
	else
		Item_DelFromPackage(entity->component.item, ItemInfo::EQUIPMENT, parentEquipID, 1, false);

	if (Config_ScribeHost() != NULL) {
		SNPRINTF2(buf, index, "\"}");
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "Inherit", buf));
	}
	return 0;
}

void PlayerEntity_LoginToPlayerInfo(const NetProto_Login *login, PlayerInfo *info) {
	if (login == NULL || info == NULL)
		return;

	info->set_account(login->account());
	info->set_password(login->password());
	info->set_platform(login->platform());
	info->set_deviceID(login->deviceID());
	info->set_deviceAddTime(login->deviceAddTime());
	info->set_idfa(login->idfa());
	info->set_addTime(login->addTime());
	info->set_osversion(login->osversion());
	info->set_phonetype(login->phonetype());
	info->set_imei(login->imei());

	if (info->platform() == "QQ"
			|| info->platform() == "WX") {
		info->set_session_id(login->session_id());
		info->set_session_type(login->session_type());
		info->set_openid(login->openid());
		info->set_openkey(login->openkey());
		info->set_pay_token(login->pay_token());
		info->set_pf(login->pf());
		info->set_pfkey(login->pfkey());
	}
}

int PlayerEntity_CanEnterPlayOff(struct PlayerEntity *entity, int playoff, int day, int pass) {
	if (!PlayerEntity_IsValid(entity))
		return -1;

	const PlayOff *info = PlayOffManager_PlayOff(playoff);
	if (info == NULL)
		return -1;

	switch(info->condition()) {
		case PlayOff::EVENT:
			{
				if ((entity->att.fixedEvent(info->arg1()) & (1 << info->arg2())) == 0)
					return -1;
			}
			break;
		default:
			return -1;
	}

	if (info->over(day) == 1 && pass == 0) {
		int index = PlayOffManager_CurIndex(playoff, day, pass);
		if ((entity->att.fixedEvent(47) & (1 << index)) == 0) {
			index = PlayOffManager_CurIndex(playoff, day, pass + 1);
			if (entity->att.fixedEvent(47) & (1 << index))
				return -2;
			else
				return -1;
		}
	} else if (day > 0 || pass > 0) {
		int index = PlayOffManager_CurIndex(playoff, day, pass);
		if ((entity->att.fixedEvent(47) & (1 << index)) == 0)
			return -1;
	}

	struct PlayOffUnit *unit = MapPool_SelectPlayOff(entity->component.baseAtt->roleID());
	if (unit != NULL) {
		int door = PlayOffManager_Count(info->over(day)) / 2 + 1;
		if (door > 1) {
			if (unit->lWin >= door || unit->rWin >= door) {
				if ((unit->lWin >= door && unit->lhs == entity->component.baseAtt->roleID()) || (unit->rWin >= door && unit->rhs == entity->component.baseAtt->roleID())) {
					return 1;
				} else {
					if (info->over(day) == 2) {
						int begin = day == 0 ? info->limit() : info->over(day - 1);
						int end = info->over(day);
						int totalPass = PlayOffManager_TotalPass(begin, end);
						if (pass >= totalPass - 1)
							return 1;
					}
					return 2;
				}
			}
		}
	}

	return 0;
}

void PlayerEntity_Update(struct PlayerEntity *entity) {
	assert(PlayerEntity_IsValid(entity));

	/*
	   static int64_t prev = 0;
	   if (Time_ElapsedTime() - prev > 1000)
	   {
	   Fight_ModifyExp(entity->component.fight, 200);
	   prev = Time_ElapsedTime();
	   }
	   */

	if (entity->accountStatus == ACCOUNT_ONLINE) {
		int64_t cur = Time_ElapsedTime();
		if (cur - entity->prevDesignation > Config_DesignationInterval()) {
			int64_t curTime = (int64_t)Time_TimeStamp();
			for (int i = 0; i < entity->att.designationRecord_size(); i++) {
				DesignationRecord *record = entity->att.mutable_designationRecord(i);
				if (record->has()) {
					const DesignationInfo *info = DesignationInfoManager_DesignationInfo(i);
					assert(info != NULL);
					if (info->time() > 0) {
						if (curTime - record->start() > info->time()) {
							PlayerEntity_DelDesignation(entity, i);
						}
					}
				}
			}
			entity->prevDesignation = cur;
		}
		if (cur - entity->prevWing > Config_WingInterval()) {
			int curTime = (int)Time_TimeStamp();
			for (int i = 0; i < entity->att.itemPackage().wings_size(); i++) {
				int v = entity->att.itemPackage().wings(i);
				if (v > 0) {
					if (curTime >= v) {
						Item_UseWing(entity->component.item, i, false);
						entity->att.mutable_itemPackage()->set_wings(i, 0);
						if (entity->att.att().equipmentAtt().wing() == i) {
							Equipment_UnwearWing(entity->component.equipment);
							static NetProto_UnwearWing proto;
							proto.Clear();
							GCAgent_SendProtoToClients(&entity->id, 1, &proto);
						}
					}
				}
			}
			for (int i = 0; i < entity->att.itemPackage().fashions_size(); i++) {
				FashionAsset *asset = entity->att.mutable_itemPackage()->mutable_fashions(i);
				int v = asset->v();
				if (v > 0) {
					if (curTime >= v) {
						Item_UseFashion(entity->component.item, i, false);
						asset->set_v(0);
						if (entity->att.att().equipmentAtt().fashion() == i) {
							Equipment_UnwearFashion(entity->component.equipment);
							static NetProto_ShiftItem proto;
							proto.set_prevType(NetProto_ShiftItem::FASHION);
							proto.set_newType(NetProto_ShiftItem::PACKAGE);
							GCAgent_SendProtoToClients(&entity->id, 1, &proto);
						}
					}
				}
			}
			entity->prevWing = cur;
		}
		if (entity->att.fixedEvent(69) != 0) { 
			if (Time_TimeStamp() > entity->att.fixedEvent(69)) {
				PlayerEntity_SetFixedEvent(entity, 69, 0, true);
				Item_ModifyVIP(entity->component.item, -entity->att.itemPackage().vip());
			}    
		}
		if (cur - entity->prevSave > Config_SaveInterval()) {
			int32_t map = entity->att.att().movementAtt().mapID();
			const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
			if (mapInfo != NULL && mapInfo->mapType() == MapInfo::PEACE) {
				static DCProto_SaveRoleData saveRoleData;
				saveRoleData.Clear();
				PlayerEntity_Save(entity, &saveRoleData);
				GCAgent_SendProtoToDCAgent(&saveRoleData);
				entity->prevSave = cur;
			}
		}
		// TODO
		// the first day of year will be wrong
		if (entity->att.att().fightAtt().worldBossNum() != 0 && entity->att.att().fightAtt().worldBossNum() != MapPool_WorldBossNum()) {
			entity->att.mutable_att()->mutable_fightAtt()->set_worldBossHurt(0);
			if (entity->att.fixedEvent(19) != 0)
				PlayerEntity_SetFixedEvent(entity, 19, 0, true);
		}
		if (entity->att.fixedEvent(40) != 0 && cur > entity->att.fixedEvent(40)) {
			PlayerEntity_SetFixedEvent(entity, 17, 0, true);
		}
	}
	else if (entity->accountStatus == ACCOUNT_DROP) {
		int64_t cur = Time_ElapsedTime();
		if (cur - entity->prevDropTime > Config_DropToLogout()) {
			AccountPool_Logout(entity->id);
			return;
		}
	}
	else {
		assert(0);
	}
}

int PlayerEntity_ObtainPet(struct PlayerEntity *entity, int32_t id) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}
	const NPCAtt* pet = NPCInfoManager_Pet(id, 0, 0);
	if (NULL == pet) {
		return -2;
	}

	int petSize = entity->att.pets_size();
	int	empty = -1;
	for (int i = 0; i < petSize; ++i) {
		if (id == entity->att.pets(i).id()) {
			return -3;
		}

		if (-1 == entity->att.pets(i).id()) {
			if (empty == -1) {
				empty = i;
			}
		}
	}

	if (empty == -1) {
		return -4;
	}


	int64_t goodsID = pet->goodsID();
	int goodsCount = pet->goodsCount();	
	DEBUG_LOG("goodsid: %d", goodsID);
	int count = Item_Count(entity->component.item, ItemInfo::GOODS, goodsID);
	if (count < goodsCount) {
		return -5;
	}
	Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, goodsID, goodsCount, true);

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"petID\":\"%d\"}", id);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "ObtainPet", buf));
	}

	PetAsset *addPet = entity->att.mutable_pets(empty);
	addPet->set_id(id);
	addPet->set_name(pet->att().baseAtt().name());
	addPet->set_pet_level(1);
	addPet->set_quality(0);
	addPet->set_level(0);

	return empty;
}


bool PlayerEntity_SetPetFight(struct PlayerEntity *entity, int32_t index) {
	if (!PlayerEntity_IsValid(entity)) {
		return false;
	}
	if (index < 0 || index >= entity->att.pets_size()) {
		DEBUG_LOGERROR("PlayerEntity_SetPetFight: the pet index has not exist");
		return false;
	}

	if (entity->att.pets(index).id() < 0) {
		DEBUG_LOGERROR("PlayerEntity_SetPetFight: the pet has not exist");
		return false;
	}

	Fight_SetFightPet(entity->component.fight, index);
	return true;
}

bool PlayerEntity_SetPetRest(struct PlayerEntity *entity, int32_t index, bool flag) {
	if (!PlayerEntity_IsValid(entity)) {
		return false;
	}
	if (index < 0 || index >= entity->att.pets_size()) {
		DEBUG_LOGERROR("PlayerEntity_SetPetRest: the pet index has not exist");
		return false;
	}

	if (entity->att.pets(index).id() < 0) {
		DEBUG_LOGERROR("PlayerEntity_SetPetRest: the pet has not exist");
		return false;
	}

	Fight_SetPetRest(entity->component.fight, flag);
	return true;
}

bool PlayerEntity_SetPetAttach(struct PlayerEntity *entity, int32_t index) {
	return true;
}


int PlayerEntity_SetPetLevelUp(struct PlayerEntity *entity, int32_t index) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}
	if (index < 0 || index >= entity->att.pets_size()) {
		return -2;
	}

	if (entity->att.pets(index).id() < 0) {
		return -3;
	}

	int level = entity->att.att().fightAtt().level();
	int petLevel = entity->att.pets(index).pet_level();
	if (petLevel >= level) {
		return -4;
	}

	int cost = Config_MoneyPetLevelUp(petLevel + 1);
	DEBUG_LOG("money:%d", cost);
	if (cost > Item_Package(entity->component.item)->money()) {
		return -5;
	}
	Item_ModifyMoney(entity->component.item, -cost);

	entity->att.mutable_pets(index)->set_pet_level(petLevel + 1);

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *str = buf;

		PB_PlayerAtt playerAtt;
		entity->att.ToPB(&playerAtt);
		int power = Fight_PetPower(&playerAtt, index);
		if (power >= 0) {
			SNPRINTF2(buf, str, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"petLevel\":\"%d", entity->att.pets(index).pet_level());
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"petID\":\"%d", entity->att.pets(index).id());
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"petPower\":\"%d", power);
			str += strlen(str);
			SNPRINTF2(buf, str, "\"}");
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "Pet", buf));
		}
	}
	return 1;
}


int PlayerEntity_PetLearnSkill(struct PlayerEntity *entity, int32_t index, int32_t skillID, bool flag) {
	/*
	   if (!PlayerEntity_IsValid(entity)) {
	   return -1;
	   }
	   if (index < 0 || index >= entity->att.pets_size()) {
	   DEBUG_LOGERROR("PlayerEntity_PetLearnSkill: the pet index has not exist");
	   return -1;
	   }

	   if (entity->att.pets(index).id() < 0 || skillID < 0) {
	   DEBUG_LOGERROR("PlayerEntity_SetLearnSkill: the pet has not exist");
	   return -1;
	   }

	   int count = 0;
	   int skills = 0;
	   int curNum = -1;
	   int randNum = Time_Random(0, 100);
	   PetAsset *addPet = entity->att.mutable_pets(index);

	   for (int i =0; i < addPet->extraSkills_size(); ++i) {
	   if (addPet->extraSkills(i).id() != -1) {
	   count ++;
	   }
	   if (addPet->extraSkills(i).id() == skillID) {
	   DEBUG_LOGERROR("PlayerEntity_PetLearnSkill: the Pet Skill has exist!");
	   return -1;
	   }
	   }

	   int forgetProbably = Config_ForgetProbably(count);
	   if (randNum < forgetProbably ) {
	   curNum = randNum % count;
	   }

	   bool learnFlag = true;
	   bool forgetFlag = (curNum == -1) ? false : true;
	   for (int i =0; i < addPet->extraSkills_size(); ++i) {
	   if (0 <=  addPet->extraSkills(i).id() && forgetFlag && flag) {
	   if (curNum == 0) {
	   skills |= i;
	   if ( !PlayerEntity_PetForgetSkills(entity, index, (1 << i)) ) {
	   return -1;
	   }
	   forgetFlag = false;
	   }
	   curNum--;
	   }

	   if (-1 == addPet->extraSkills(i).id() && learnFlag) {
	   skills |= (i << 16);
	   learnFlag = false;
	   addPet->mutable_extraSkills(i)->set_id(skillID);
	   addPet->mutable_extraSkills(i)->set_level(1);
	   }
	   }

	   return skills;
	   */
	return 0;
}

bool PlayerEntity_PetInherit(struct PlayerEntity *entity, int32_t indexPre, int32_t indexAfter, bool flag) {
	/*
	   if (!PlayerEntity_IsValid(entity)) {
	   return false;
	   }

	   if (indexPre == indexAfter) {
	   DEBUG_LOGERROR("PlayerEntity_PetInherit: two pet the index is equal!");
	   return false;
	   }

	   if (indexPre < 0 || indexPre >= entity->att.pets_size()) {
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the pet indexPre has not exist");
	   return false;
	   }

	   if (entity->att.pets(indexPre).id() < 0) {
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the pet has not exist");
	   return false;
	   }

	   if (indexAfter < 0 || indexAfter >= entity->att.pets_size()) {
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the pet indexAfter has not exist");
	   return false;
	   }

	   if (entity->att.pets(indexAfter).id() < 0) {
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the pet has not exist");
	   return false;
	   }


	   PetAsset *addPetPre = entity->att.mutable_pets(indexPre);
	   PetAsset *addPetAfter = entity->att.mutable_pets(indexAfter);

	   int level = Config_PetInheritExpTransform(addPetPre->level());
	   int count = 0;
	   for (int i = 0; i < addPetPre->extraSkills_size(); ++i) {
	   if (addPetPre->extraSkills(i).id() >= 0) {
	   count++;
	   }
	   }

	   int yuanbao = Config_PetInheritSkillsTransform(count);
	//const ItemPackage *package = Item_Package(entity->component.item);
	//if (package->rmb() < yuanbao) {
	if (!Item_HasRMB(entity->component.item, yuanbao)) {
	DEBUG_LOGERROR("PlayerEntity_PetInherit: yuanbao is not enough!");
	return false;
	}

	Item_ModifyRMB(entity->component.item, -yuanbao, false, 16, addPetPre->id(), addPetAfter->id());
	addPetAfter->set_level(addPetAfter->level() + level);

	for (int i = 0; i < addPetPre->extraSkills_size(); ++i) {
	if (addPetPre->extraSkills(i).id() < 0) {
	continue;
	}
	PlayerEntity_PetLearnSkill(entity, indexAfter, addPetPre->extraSkills(i).id(), false);
	}

	if (!PlayerEntity_DelPet(entity, indexPre)) {
	return false;
	}

	static NetProto_PetInherit proto;
	addPetAfter->ToPB(proto.mutable_pet());	
	GCAgent_SendProtoToClients(&(entity->id), 1, &proto);
	*/
	return true;
}

int PlayerEntity_PetAdvance(struct PlayerEntity *entity, int32_t index) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}
	if (index < 0 || index >= entity->att.pets_size()) {
		return -2;
	}

	int quality = entity->att.pets(index).quality();
	int level = entity->att.pets(index).level();
	const NPCAtt* pet =  NPCInfoManager_Pet(entity->att.pets(index).id(), quality, level);
	if (NULL == pet) {
		return -3;
	}


	pet =  NPCInfoManager_Pet(entity->att.pets(index).id(), quality, level + 1);
	if (pet == NULL) {
		pet =  NPCInfoManager_Pet(entity->att.pets(index).id(), quality + 1, 0);
		if (pet != NULL) {
			quality++;
			level = 0;
		}else {
			return -6;
		}
	}else {
		level++;
	}

	pet =  NPCInfoManager_Pet(entity->att.pets(index).id(), quality, level);
	if (NULL == pet) {
		return -7;
	}

	int goodsID = pet->goodsID();
	int goodsCount = pet->goodsCount();	
	DEBUG_LOG("goodsid: %d", goodsID);
	int count = Item_Count(entity->component.item, ItemInfo::GOODS, goodsID);
	if (count < goodsCount) {
		return -8;
	}

	Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, (int64_t)goodsID, goodsCount, true);
	entity->att.mutable_pets(index)->set_quality(quality);
	entity->att.mutable_pets(index)->set_level(level);

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *str = buf;

		PB_PlayerAtt playerAtt;
		entity->att.ToPB(&playerAtt);
		int power = Fight_PetPower(&playerAtt, index);
		if (power >= 0) {
			SNPRINTF2(buf, str, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"petLevel\":\"%d", entity->att.pets(index).pet_level());
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"petID\":\"%d", entity->att.pets(index).id());
			str += strlen(str);
			SNPRINTF2(buf, str, "\",\"petPower\":\"%d", power);
			str += strlen(str);
			SNPRINTF2(buf, str, "\"}");
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "Pet", buf));
		}
	}

	return 0;
}

bool PlayerEntity_PetForgetSkills(struct PlayerEntity *entity, int32_t index, int16_t bitSkills) {
	/*
	   if (!PlayerEntity_IsValid(entity)) {
	   return false;
	   }
	   if (index < 0 || index >= entity->att.pets_size()) {
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the pet index has not exist");
	   return false;
	   }

	   if (entity->att.pets(index).id() < 0) {
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the pet has not exist");
	   return false;
	   }

	   PetAsset *addPet = entity->att.mutable_pets(index);
	   for (int i = 0; i < addPet->extraSkills_size(); ++i) {
	   if (-1 == addPet->extraSkills(i).id()) {
	   if ((1 << i) & bitSkills ) {
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the (index = %d) Pet has no skill (skill index = %d)", index, i);
	   }
	   break;
	   }

	   if ( addPet->extraSkills(i).id() >= 0 && ((1 << i) & bitSkills) ) {
	   addPet->mutable_extraSkills(i)->set_id(-1);
	   addPet->mutable_extraSkills(i)->set_level(0);
	   DEBUG_LOGERROR("PlayerEntity_PetForgetSkills: the (index = %d)Pet Skill has forget (skill index = %d)", index, i);
	   }
	   }
	   */
	return true;
}

bool PlayerEntity_DelPet(struct PlayerEntity *entity, int32_t index) {
	/*
	   if (!PlayerEntity_IsValid(entity)) {
	   return false;
	   }

	   if (entity->att.pets(index).id() < 0 ) {
	   DEBUG_LOGERROR("PlayerEntity_DelPet: has not exist Pet");
	   return false;
	   }

	   entity->att.mutable_pets(index)->set_id(-1);
	   entity->att.mutable_pets(index)->set_name("");
	   entity->att.mutable_pets(index)->set_level(0);

	   for (int i = 0; i < entity->att.pets_size(); ++i) {
	   entity->att.mutable_pets(index)->mutable_extraSkills(i)->set_id(-1);
	   entity->att.mutable_pets(index)->mutable_extraSkills(i)->set_level(0);
	   }
	   */
	return true;
}


bool PlayerEntity_FriendsLove(struct PlayerEntity *entity, int64_t roleID, bool flag) {
	if (!PlayerEntity_IsValid(entity)) {
		return false;
	}

	FriendInfo *info = NULL;
	if (flag) {
		static int maxLoveHeartNum[MAX_LOVEHEART];
		static int curLoveHeartNum;
		curLoveHeartNum = 0;
		if (roleID == -1) {
			int sendNum = Config_SendFriendsLove() - ((entity->att.dayEvent(27) & 0x0000FF00) >> 8);
			DEBUG_LOG("send love to friends start!");
			for (int i = 0;  sendNum > 0 && i < entity->att.friends_size(); ++i) {
				info = 	entity->att.mutable_friends(i);
				if (info->roleID() != -1 && !info->loveHeart()) {
					info->set_loveHeart(true);
					info->set_flag(true);
					sendNum--;

					PlayerEntity *loveHeartPlayer = PlayerEntity_PlayerByRoleID(info->roleID());
					if (NULL != loveHeartPlayer) {
						maxLoveHeartNum[curLoveHeartNum++] = loveHeartPlayer->id;
					}

					DEBUG_LOG("send friends love success ,friend ID = %ld", info->roleID());
					PlayerEntity_SynFriendsLoveFans(entity, info->roleID());
				}
			}
			if ( sendNum == Config_SendFriendsLove() - ((entity->att.dayEvent(27) & 0x0000FF00) >> 8) ) {
				return false;
			}
			sendNum = ((Config_SendFriendsLove() - sendNum) << 8);
			int value = (entity->att.dayEvent(27) & 0xFFFF00FF) | sendNum;
			PlayerEntity_SetDayEvent(entity, 27, value, false);
		} else {
			int sendNum = Config_SendFriendsLove() - ((entity->att.dayEvent(27) & 0x0000FF00) >> 8);
			DEBUG_LOG("send love to friends start!");
			for (int i = 0; sendNum > 0 && i < entity->att.friends_size(); ++i) {
				info = entity->att.mutable_friends(i);
				if (info->roleID() == roleID && !info->loveHeart()) {
					info->set_loveHeart(true);
					info->set_flag(true);
					sendNum--;

					PlayerEntity *loveHeartPlayer = PlayerEntity_PlayerByRoleID(info->roleID());
					if (NULL != loveHeartPlayer) {
						maxLoveHeartNum[curLoveHeartNum++] = loveHeartPlayer->id;
					}

					DEBUG_LOG("send signal friends love success ,friend ID = %ld", info->roleID());
					PlayerEntity_SynFriendsLoveFans(entity, info->roleID());
					break;
				}
			}
			if ( sendNum == Config_SendFriendsLove() - ((entity->att.dayEvent(27) & 0x0000FF00) >> 8) ) {
				return false;
			}
			sendNum = ((Config_SendFriendsLove() - sendNum) << 8);
			int value = (entity->att.dayEvent(27) & 0xFFFF00FF) | sendNum;
			PlayerEntity_SetDayEvent(entity, 27, value, false);
		}
		if (curLoveHeartNum > 0) {
			static NetProto_AddFansLove fanLove;
			fanLove.Clear();
			fanLove.set_roleID(entity->att.att().baseAtt().roleID());
			GCAgent_SendProtoToClients(maxLoveHeartNum, curLoveHeartNum, &fanLove);
		}
		DEBUG_LOG("srver send love to client , loveheart count = %d", curLoveHeartNum);
	} else {
		if (roleID == -1) {
			int recvNum = Config_HarvestFriendsLove() - ((entity->att.dayEvent(27) & 0x00FF0000) >> 16);
			int num = 0;
			DEBUG_LOG("recv love to friends start!");
			for (int i = 0;  recvNum - num > 0 && i < entity->att.fans_size(); ++i) {
				info = 	entity->att.mutable_fans(i);
				DEBUG_LOG(" recvNum = %d,  num = %d, ID = %d, flag = %d", recvNum, num, info->roleID(), info->loveHeart());
				if (info->roleID() != -1 && info->loveHeart()) {
					info->set_loveHeart(false);
					info->set_flag(true);
					num++;
					DEBUG_LOG("recv friends love success ,friend ID = %ld", info->roleID());
				}
			}
			if (num == 0) {
				DEBUG_LOG("recv love count == 0");
				return false;
			}
			Item_ModifyLovePoint(entity->component.item, num * 5);
			Item_ModifyDurability(entity->component.item, num);
			recvNum = ((((entity->att.dayEvent(27) & 0x00FF0000) >> 16)  + num) << 16);
			int value = (entity->att.dayEvent(27) & 0xFF00FFFF) | recvNum;
			PlayerEntity_SetDayEvent(entity, 27, value, false);
		} else {
			int recvNum = Config_HarvestFriendsLove() - ((entity->att.dayEvent(27) & 0x00FF0000) >> 16);
			int num = 0;
			DEBUG_LOG("recv love to friends start!");
			for (int i = 0;  recvNum - num > 0 && i < entity->att.fans_size(); ++i) {
				info = 	entity->att.mutable_fans(i);
				if (info->roleID() == roleID && info->loveHeart()) {
					info->set_loveHeart(false);
					info->set_flag(true);
					num++;
					DEBUG_LOG("recv signal friends love success ,friend ID = %ld", info->roleID());
					break;
				}
			}
			if (num == 0) {
				DEBUG_LOG("recv love count == 0");
				return false;
			}
			Item_ModifyLovePoint(entity->component.item, num * 5);
			Item_ModifyDurability(entity->component.item, num);
			recvNum = ((((entity->att.dayEvent(27) & 0x00FF0000) >> 16)  + num) << 16);
			int value = (entity->att.dayEvent(27) & 0xFF00FFFF) | recvNum;
			PlayerEntity_SetDayEvent(entity, 27, value, false);
		}

	}
	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		char *index = buf;
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", entity->att.att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"sendNum\":\"%d", (entity->att.dayEvent(27) & 0x0000FF00) >> 8);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"recvNum\":\"%d", (entity->att.dayEvent(27) & 0x00FF0000) >> 16);
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(entity->id), entity->id, "FriendLove", buf));
	}
	return true;
}


void PlayerEntity_SynFriendsLoveFans(struct PlayerEntity *entity, int64_t roleID) {
	if (!PlayerEntity_IsValid(entity)) {
		return;
	}

	PlayerEntity *fan = PlayerEntity_PlayerByRoleID(roleID);
	if (fan == entity) {
		DEBUG_LOG("can not send love to self!");
		return;
	}

	if (fan != NULL) {
		FriendInfo *info = NULL;
		for (int i = 0; i < fan->att.fans_size(); i++) {
			info = fan->att.mutable_fans(i);
			if (info->roleID() == entity->att.att().baseAtt().roleID()) {
				info->set_loveHeart(true);
				break;
			}
		}
	} else {
		static DCProto_LoadFriendsFans proto;
		proto.Clear();
		proto.set_roleID(roleID);
		proto.set_roleID2(entity->att.att().baseAtt().roleID());
		GCAgent_SendProtoToDCAgent(&proto);
	}
}

int PlayerEntity_LoginObtRMB(struct PlayerEntity *entity) {
	return -1;
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	int randNum = Time_Random(0, 1000);
	if (randNum < 90) {
		randNum = 1;
	} else if (randNum < 990) {
		randNum = 2;
	} else if (randNum < 999) {
		randNum = 4;
	} else {
		randNum = 10;
	}

	if (Time_WithinHour(Time_TimeStamp(), Config_GetRMBBegin1(), Config_GetRMBBegin1() + Config_GetRMBTime() / (1000 * 3600))) {
		if (!(entity->att.dayEvent(27) & 0x01000000)) {
			int value = entity->att.dayEvent(27) | 0x01000000;
			PlayerEntity_SetDayEvent(entity, 27, value, false);
			int64_t v = randNum * Config_LoginObtRMB();
			Item_ModifyRMB(entity->component.item, v, false, -7, 0, 0);

			static NetProto_GetRes gr;
			gr.Clear();
			PB_ItemInfo *unit = gr.add_items();
			unit->set_type(PB_ItemInfo::SUBRMB);
			unit->set_count(v);
			GCAgent_SendProtoToClients(&entity->id, 1, &gr);
			return randNum;
		}
	}

	if (Time_WithinHour(Time_TimeStamp(), Config_GetRMBBegin2(), Config_GetRMBBegin2() + Config_GetRMBTime() / (1000 * 3600))) {
		if (!(entity->att.dayEvent(27) & 0x02000000)) {
			int value = entity->att.dayEvent(27) | 0x02000000;
			PlayerEntity_SetDayEvent(entity, 27, value, false);
			int64_t v = randNum * Config_LoginObtRMB();
			Item_ModifyRMB(entity->component.item, v, false, -7, 0, 0);

			static NetProto_GetRes gr;
			gr.Clear();
			PB_ItemInfo *unit = gr.add_items();
			unit->set_type(PB_ItemInfo::SUBRMB);
			unit->set_count(v);
			GCAgent_SendProtoToClients(&entity->id, 1, &gr);
			return randNum;
		}
	}
	return -1;	
}

int PlayerEntity_ActiveGenRequest(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	if (entity->component.playerAtt->fixedEvent(37) < Time_TimeStamp()) {
		DEBUG_LOG("active1 has init!");
		PlayerEntity_SetFixedEvent(entity, 32, 0);
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return -1;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-1);
	if (begin == fixedMap->end()) {
		PlayerEntity_SetFixedEvent(entity, 32, 0);
		DEBUG_LOG("time has not coming!");
		return -1;
	}

	PlayerEntity_SetFixedEvent(entity, 37, (begin->second) & 0x0FFFFFFFF);
	int event = entity->component.playerAtt->fixedEvent(32);
	int randNum = Time_Random(0, 10000);
	int value = randNum % 3 + 1;
	if (event) {
		if (event & 0x03F) {
			DEBUG_LOG("event is not equip 0!");
			return -1;
		} else {
			value |= ((randNum / 5) % 3 + 1) << 2;
			value |= ((randNum / 7) % 3 + 1) << 4;
			event = (event & 0xFFFFFFC0) | value;
			PlayerEntity_SetFixedEvent(entity, 32, event);
		}
	} else {
		value |= (((randNum % 7) * randNum) % 3 + 1) << 2;
		value |= (((randNum % 11) * randNum) % 3 + 1) << 4;
		//	value |= 0x0500; Init doubleCount
		value |= 0x0280;
		//	value |= 0x04000; init gey count
		value |= 0x02800;
		event = (event & 0xFFFFFFC0) | value;
		PlayerEntity_SetFixedEvent(entity, 32, event);
	}
	return event;	
}


bool PlayerEntity_ActiveDoubleGen(struct PlayerEntity *entity) {
	if (!PlayerEntity_IsValid(entity)) {
		return false;
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return false;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-1);
	if (begin == fixedMap->end()) {
		PlayerEntity_SetFixedEvent(entity, 32, 0);
		DEBUG_LOG("time has not coming!");
		return false;
	}

	int event = entity->component.playerAtt->fixedEvent(32);
	if (event & 0x00008000) {
		DEBUG_LOG("event has already double!");
		return false;
	}

	if (!(event & 0x000007C0)) {
		DEBUG_LOG("event has not double to use !");
		return false;
	}

	event = event - (1 << 6) + (1 << 15);
	PlayerEntity_SetFixedEvent(entity, 32, event);
	return true;
}


bool PlayerEntity_ActiveUpGradeGen(struct PlayerEntity *entity, int index) {
	if (!PlayerEntity_IsValid(entity)) {
		return false;
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return false;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-1);
	if (begin == fixedMap->end()) {
		PlayerEntity_SetFixedEvent(entity, 32, 0);
		DEBUG_LOG("time has not coming!");
		return false;
	}

	int event = entity->component.playerAtt->fixedEvent(32);
	if (!(event & 0x03F)) {
		DEBUG_LOG("event is equip 0!");
		return false;
	}

	if (index < 0 || index > 2) {
		return false;
	}

	int addNum = 0;
	int value = 0;
	if (index == 0) {
		value = 0x03;
		addNum = 0x01;
	} else if (index == 1) {
		value = 0x0C;
		addNum = 0x04;
	} else {
		value = 0x030;
		addNum = 0x010;
	}

	if ((event & value) == value) {
		DEBUG_LOG("event has already 3!");
		return false;
	}

	if (!Item_HasRMB(entity->component.item, 100)) {
		DEBUG_LOG("RMB is not enough!");
		return false;
	}
	Item_ModifyRMB(entity->component.item, -100, false, 19, 0, 0);

	event += addNum;
	PlayerEntity_SetFixedEvent(entity, 32, event);
	return true;
}

bool PlayerEntity_ActiveGetGen(struct PlayerEntity *entity, bool bFlag) {
	if (!PlayerEntity_IsValid(entity)) {
		return false;
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return false;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-1);
	if (begin == fixedMap->end()) {
		PlayerEntity_SetFixedEvent(entity, 32, 0);
		DEBUG_LOG("time has not coming!");
		return false;
	}

	int event = entity->component.playerAtt->fixedEvent(32);
	if (!(event & 0x03F)) {
		DEBUG_LOG("event is equip 0!");
		return false;
	}

	bool flag = false;
	int costRMB = 0;
	if (!(event & 0x07800)) {
		costRMB = 20 + 50 * ((event & 0x0FF0000) >> 16);
		if (!Item_HasRMB(entity->component.item, costRMB)) {
			DEBUG_LOG("RMB is not enough!");
			return false;
		}
		flag = true;
	}

	if (flag) {
		Item_ModifyRMB(entity->component.item, -costRMB, false, 19, 0, 0);
		if ((event & 0x0FF0000) != 0x0FF0000) {
			PlayerEntity_SetFixedEvent(entity, 32, event + 0x10000);
		}
	} else {
		event -= 0x0800;
		PlayerEntity_SetFixedEvent(entity, 32, event);
	}

	if (bFlag) {
		PlayerEntity_ActiveDoubleGen(entity);
	}
	event = entity->component.playerAtt->fixedEvent(32);
	int count = (event & 0x08000) > 0 ? 2 : 1;
	int grade[3] = {1,1,1};
	grade[0] = event & 0x03;
	grade[1] = (event & 0x0C) >> 2;
	grade[2] = (event & 0x030) >> 4;
	static NetProto_GetRes gr;
	gr.Clear();

	if (grade[0] == grade[1] && grade[1] == grade[2]) {
		int gem = 0;
		if (grade[0] == 3) {
			gem = 26;
		} else if (grade[0] == 2) {
			gem = 25;
		} else {
			gem = 24;
		}

		PB_ItemInfo *item = gr.add_items();
		item->set_type(PB_ItemInfo::GOODS);
		item->set_count(2 * 3 * count);
		item->set_id(gem);
		Item_AddToPackage(entity->component.item, ItemInfo::GOODS, gem, 2 * 3 * count);

		int64_t roleID = PlayerEntity_RoleID(entity);
		char ch[1024];
		SNPRINTF1(ch, "23-%d-%d", gem, 2 * 3 * count);
		DCProto_SaveSingleRecord saveRecord;
		saveRecord.set_mapID(-23);
		saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
		saveRecord.mutable_record()->mutable_role()->set_name(ch);
		saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
		GCAgent_SendProtoToDCAgent(&saveRecord);
	} else {
		int gem = 0;
		for (int i = 0; i < (int)(sizeof(grade) / sizeof(int)); ++i) {
			if (grade[i] == 3) {
				gem = 26;
			} else if (grade[i] == 2) {
				gem = 25;
			} else {
				gem = 24;
			}
			Item_AddToPackage(entity->component.item, ItemInfo::GOODS, gem, count, &gr);

			int64_t roleID = PlayerEntity_RoleID(entity);
			char ch[1024];
			SNPRINTF1(ch, "24-%d-%d", gem, count);
			DCProto_SaveSingleRecord saveRecord;
			saveRecord.set_mapID(-23);
			saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
			saveRecord.mutable_record()->mutable_role()->set_name(ch);
			saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
			GCAgent_SendProtoToDCAgent(&saveRecord);
		}
	}

	GCAgent_SendProtoToClients(&entity->id, 1, &gr);

	event = event & 0xFFFF7FC0;
	PlayerEntity_SetFixedEvent(entity, 32, event);
	return true;
}

bool PlayerEntity_ActiveFight(struct PlayerEntity *entity, bool flag) {
	if (!PlayerEntity_IsValid(entity)) {
		return false;
	}

	if (entity->component.playerAtt->fixedEvent(38) < Time_TimeStamp()) {
		DEBUG_LOG("active2 has init!");
		PlayerEntity_SetFixedEvent(entity, 33, 0);
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	assert(fixedMap != NULL);

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-2);
	if (begin == fixedMap->end()) {
		PlayerEntity_SetFixedEvent(entity, 33, 0);
		DEBUG_LOG("time has not coming!");
		return false;
	}

	PlayerEntity_SetFixedEvent(entity, 38, (begin->second) & 0xFFFFFFFF);
	int event = entity->component.playerAtt->fixedEvent(33);
	if (!flag) {
		if ((event & 0x0FF) >= 80) {
			event = (event & 0xFFFFFF00) + 80;
			DEBUG_LOG("event has already more than 80!");
			return false;
		}

		event += 1;
		PlayerEntity_SetFixedEvent(entity, 33, event);
		/*
		   if ((event & 0x0FF) == 10) {
		   PlayerEntity_SetFixedEvent(entity, 33, event | 0x0100);
		   } else if ((event & 0x0FF) == 20) {
		   PlayerEntity_SetFixedEvent(entity, 33, event | 0x0200);
		   } else if ((event & 0x0FF) == 40) {
		   PlayerEntity_SetFixedEvent(entity, 33, event | 0x0400);
		   } else if ((event & 0x0FF) == 60) {
		   PlayerEntity_SetFixedEvent(entity, 33, event | 0x0800);
		   } else if ((event & 0x0FF) == 80) {
		   PlayerEntity_SetFixedEvent(entity, 33, event | 0x01000);
		   }
		   */
		return true;
	} else {
		static NetProto_GetRes gr;
		gr.Clear();
		int total = 0;
		if ((event & 0x0FF) >= 10 && (event & 0x0100) == 0) {
			PlayerEntity_SetFixedEvent(entity, 33, event | 0x0100);
			Item_AddToPackage(entity->component.item, ItemInfo::GOODS, Config_StrongStone(), 10, &gr);

			int64_t roleID = PlayerEntity_RoleID(entity);
			char ch[1024];
			SNPRINTF1(ch, "25-%d-%d", (int)Config_StrongStone(), 10);
			DCProto_SaveSingleRecord saveRecord;
			saveRecord.set_mapID(-23);
			saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
			saveRecord.mutable_record()->mutable_role()->set_name(ch);
			saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
			GCAgent_SendProtoToDCAgent(&saveRecord);
			total += 5;
		}
		event = entity->component.playerAtt->fixedEvent(33);
		if ((event & 0x0FF) >= 20 && (event & 0x0200) == 0) {
			PlayerEntity_SetFixedEvent(entity, 33, event | 0x0200);
			Item_AddToPackage(entity->component.item, ItemInfo::GOODS, Config_StrongStone(), 20, &gr);

			int64_t roleID = PlayerEntity_RoleID(entity);
			char ch[1024];
			SNPRINTF1(ch, "26-%d-%d", (int)Config_StrongStone(), 20);
			DCProto_SaveSingleRecord saveRecord;
			saveRecord.set_mapID(-23);
			saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
			saveRecord.mutable_record()->mutable_role()->set_name(ch);
			saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
			GCAgent_SendProtoToDCAgent(&saveRecord);
			total += 10;
		}
		event = entity->component.playerAtt->fixedEvent(33);
		if ((event & 0x0FF) >= 40&& (event & 0x0400) == 0) {
			PlayerEntity_SetFixedEvent(entity, 33, event | 0x0400);
			Item_AddToPackage(entity->component.item, ItemInfo::GOODS, Config_StrongStone(), 50, &gr);

			int64_t roleID = PlayerEntity_RoleID(entity);
			char ch[1024];
			SNPRINTF1(ch, "27-%d-%d", (int)Config_StrongStone(), 50);
			DCProto_SaveSingleRecord saveRecord;
			saveRecord.set_mapID(-23);
			saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
			saveRecord.mutable_record()->mutable_role()->set_name(ch);
			saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
			GCAgent_SendProtoToDCAgent(&saveRecord);
			total += 25;
		}
		event = entity->component.playerAtt->fixedEvent(33);
		if ((event & 0x0FF) >= 60 && (event & 0x0800) == 0) {
			PlayerEntity_SetFixedEvent(entity, 33, event | 0x0800);
			Item_AddToPackage(entity->component.item, ItemInfo::GOODS, Config_StrongStone(), 80, &gr);

			int64_t roleID = PlayerEntity_RoleID(entity);
			char ch[1024];
			SNPRINTF1(ch, "28-%d-%d", (int)Config_StrongStone(), 80);
			DCProto_SaveSingleRecord saveRecord;
			saveRecord.set_mapID(-23);
			saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
			saveRecord.mutable_record()->mutable_role()->set_name(ch);
			saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
			GCAgent_SendProtoToDCAgent(&saveRecord);
			total += 40;
		}
		event = entity->component.playerAtt->fixedEvent(33);
		if ((event & 0x0FF) >= 80&& (event & 0x01000) == 0) {
			PlayerEntity_SetFixedEvent(entity, 33, event | 0x01000);
			Item_AddToPackage(entity->component.item, ItemInfo::GOODS, Config_StrongStone(), 100, &gr);

			int64_t roleID = PlayerEntity_RoleID(entity);
			char ch[1024];
			SNPRINTF1(ch, "29-%d-%d", (int)Config_StrongStone(), 100);
			DCProto_SaveSingleRecord saveRecord;
			saveRecord.set_mapID(-23);
			saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
			saveRecord.mutable_record()->mutable_role()->set_name(ch);
			saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
			GCAgent_SendProtoToDCAgent(&saveRecord);
			total += 50;
		}
		if (total > 0) {
			GCAgent_SendProtoToClients(&entity->id, 1, &gr);
		}
		return true;
	}
	return false;
}

int PlayerEntity_ActiveStrongeSolider(struct PlayerEntity *entity, bool flag) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return -1;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-3);
	if (begin == fixedMap->end()) {
		if (entity->component.playerAtt->fixedEvent(34)) {
			PlayerEntity_SetFixedEvent(entity, 35, entity->component.playerAtt->fixedEvent(34) >> 21);
			PlayerEntity_SetFixedEvent(entity, 34, 0);
		}
		if (entity->component.playerAtt->fixedEvent(39)) {
			PlayerEntity_SetFixedEvent(entity, 36, entity->component.playerAtt->fixedEvent(39) + 3600 * 24 * 3);
			PlayerEntity_SetFixedEvent(entity, 39, 0);
		}

		if (Time_TimeStamp() < entity->component.playerAtt->fixedEvent(36) && entity->component.playerAtt->fixedEvent(34) == 0) {
			int level = entity->att.att().fightAtt().level();
			int count = (entity->component.playerAtt->fixedEvent(35) & 0x0FFFF);
			int detail = level * count * (1 + count /100);
			Fight_ModifyPropertyDelta(entity->component.fight, FightAtt::ATK, detail, 0);
			Fight_ModifyPropertyDelta(entity->component.fight, FightAtt::DEF, detail, 0);
			DEBUG_LOG("ATK or DEF has already add to body, percent = %d", detail);
		}
		return -1;
	}

	if (entity->component.playerAtt->fixedEvent(39) != (begin->second & 0xFFFFFFFF) && entity->component.playerAtt->fixedEvent(39) != 0) {
		if (entity->component.playerAtt->fixedEvent(34)) {
			PlayerEntity_SetFixedEvent(entity, 35, entity->component.playerAtt->fixedEvent(34) >> 21);
			PlayerEntity_SetFixedEvent(entity, 34, 0);
			DEBUG_LOG("active3 has init!");
		}
		if (entity->component.playerAtt->fixedEvent(39)) {
			PlayerEntity_SetFixedEvent(entity, 36, entity->component.playerAtt->fixedEvent(39) + 3600 * 24 * 3);
			PlayerEntity_SetFixedEvent(entity, 39, 0);
		}

		if (Time_TimeStamp() < entity->component.playerAtt->fixedEvent(36)) {
			int level = entity->att.att().fightAtt().level();
			int count = (entity->component.playerAtt->fixedEvent(35) & 0x0FFFF);
			int detail = level * count * (1 + count /100);
			Fight_ModifyPropertyDelta(entity->component.fight, FightAtt::ATK, detail, 0);
			Fight_ModifyPropertyDelta(entity->component.fight, FightAtt::DEF, detail, 0);
			DEBUG_LOG("ATK or DEF has already add to body, detail = %d", detail);
		}
	}

	if (flag) {
		return -1;
	}
	PlayerEntity_SetFixedEvent(entity, 39, (begin->second) & 0x0FFFFFFFF);
	int event = entity->component.playerAtt->fixedEvent(34);
	if (!((event & 0x07F) > 0 && (event & 0x03F80) > 0 && (event & 0x01FC000) > 0x07000)) {
		DEBUG_LOG("item has not enough!");
		return -1;
	}

	event = event - 0x01 - 0x080 - 0x08000 + 0x0200000;
	PlayerEntity_SetFixedEvent(entity, 34, event);

	int randNum = Time_Random(0, 100);
	int count = 0;
	if (randNum < 20) {
		count = 1;
	} else if (randNum < 80) {
		count = 2;
	} else if (randNum < 95) {
		count = 3;
	} else {
		count = 4;
	}

	static NetProto_GetRes gr;
	gr.Clear();
	//Item_AddToPackage(entity->component.item, ItemInfo::GOODS, gem, count);
	Item_AddToPackage(entity->component.item, ItemInfo::GOODS, 24, count * Config_ActiveObtItem(), &gr);

	int64_t roleID = PlayerEntity_RoleID(entity);
	char ch[1024];
	SNPRINTF1(ch, "30-%d-%d", 24, count * Config_ActiveObtItem());
	DCProto_SaveSingleRecord saveRecord;
	saveRecord.set_mapID(-23);
	saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
	saveRecord.mutable_record()->mutable_role()->set_name(ch);
	saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
	GCAgent_SendProtoToDCAgent(&saveRecord);
	GCAgent_SendProtoToClients(&entity->id, 1, &gr);
	return count;
}

void PlayerEntity_ActiveAddGoods(struct PlayerEntity *entity, int index) {
	if (!PlayerEntity_IsValid(entity)) {
		return;
	}
	if (entity->component.playerAtt->fixedEvent(39) < Time_TimeStamp()) {
		PlayerEntity_SetFixedEvent(entity, 34, 0);
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return;
	}
	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-3);
	if (begin == fixedMap->end()) {
		PlayerEntity_SetFixedEvent(entity, 35, entity->component.playerAtt->fixedEvent(34));
		PlayerEntity_SetFixedEvent(entity, 34, 0);
		DEBUG_LOG("time has not coming!");
		return;
	}

	PlayerEntity_SetFixedEvent(entity, 39, (begin->second) & 0x0FFFFFFFF);
	if (index > 0 && index < 4 ) {
		int event = entity->component.playerAtt->fixedEvent(34);
		if (index == 1) {
			if ((event & 0x07F) < 0x07F) {
				PlayerEntity_SetFixedEvent(entity, 34, ++event);
			}
		} else if (index == 2) {
			if ((event & 0x03F80) < 0x03F80) {
				event += 0x080;	
				PlayerEntity_SetFixedEvent(entity, 34, event);
			}
		} else {
			if ((event & 0x01FC000) < 0x01FC000) {
				event += 0x04000;	
				PlayerEntity_SetFixedEvent(entity, 34, event);
			}
		}
	}
}


void PlayerEntity_SysFaction(struct PlayerEntity *entity, const char* str) {
	if (!PlayerEntity_IsValid(entity)) {
		return;
	}

	entity->att.set_faction(str);
}

void PlayerEntity_SysFactionMem(int64_t roleID, const char* str) {
	PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
	if (player != NULL) {
		player->att.set_faction(str);
	}

	static DCProto_SysFactionMemInfo info;
	info.set_roleID(roleID);
	info.set_str(str);
	GCAgent_SendProtoToDCAgent(&info);
}

void PlayerEntity_SaveGodRankRecords(int64_t roleID1, int64_t roleID2, bool flag, int up) {
	PlayerEntity *player1 = PlayerEntity_PlayerByRoleID(roleID1);
	PlayerEntity *player2 = PlayerEntity_PlayerByRoleID(roleID2);

	FriendInfo info;
	static DCProto_SaveGodRankInfoRecord infoRecord;
	if (player1 != NULL && player2 != NULL) {
		player1->att.set_godDueDate(Time_TimeStamp());
		info.set_roleID(roleID2);
		info.set_name(player2->att.att().baseAtt().name());
		info.set_professionType(player2->att.att().baseAtt().professionType());
		info.set_loveHeart(flag);
		int64_t arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10) + player2->att.att().fightAtt().level();
		DEBUG_LOG("MMMMMMMMMMMMMMM%d",player1->att.godRecordInfo_size());	
		int index = player1->att.godRankIndex(); 
		*(player1->att.mutable_godRecordInfo(index % player1->att.godRecordInfo_size())) = info;
		player1->att.set_godRecordArg1(index % player1->att.godRecordArg1_size(), arg1);
		player1->att.set_godRankIndex(index + 1);

		info.set_roleID(roleID1);
		info.set_name(player1->att.att().baseAtt().name());
		info.set_professionType(player1->att.att().baseAtt().professionType());
		info.set_loveHeart(!flag);
		arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10) + player1->att.att().fightAtt().level();

		index = player2->att.godRankIndex(); 
		*(player2->att.mutable_godRecordInfo(index % player2->att.godRecordInfo_size())) = info;
		player2->att.set_godRecordArg1(index % player1->att.godRecordArg1_size(), arg1);
		player2->att.set_godRankIndex(index + 1);
	} else if (player1 != NULL && player2 == NULL) {
		player1->att.set_godDueDate(Time_TimeStamp());
		int64_t arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10);

		infoRecord.Clear();
		infoRecord.mutable_info1()->set_flag(true);
		infoRecord.mutable_info1()->mutable_info()->mutable_role()->set_roleID(roleID2);
		infoRecord.mutable_info1()->mutable_info()->mutable_role()->set_loveHeart(flag);
		infoRecord.mutable_info1()->mutable_info()->set_arg1(arg1);

		arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10) + player1->att.att().fightAtt().level();

		infoRecord.mutable_info2()->set_flag(false);
		infoRecord.mutable_info2()->mutable_info()->mutable_role()->set_roleID(roleID1);
		infoRecord.mutable_info2()->mutable_info()->mutable_role()->set_loveHeart(!flag);
		infoRecord.mutable_info2()->mutable_info()->mutable_role()->set_name(player1->att.att().baseAtt().name());
		infoRecord.mutable_info2()->mutable_info()->mutable_role()->set_professionType((PB_ProfessionInfo::Type)player1->att.att().baseAtt().professionType());
		infoRecord.mutable_info2()->mutable_info()->set_arg1(arg1);

		GCAgent_SendProtoToDCAgent(&infoRecord);
	} else if (player1 == NULL && player2 != NULL) {
		int64_t arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10) + player2->att.att().fightAtt().level();

		infoRecord.Clear();
		infoRecord.mutable_info1()->set_flag(false);
		infoRecord.mutable_info1()->mutable_info()->mutable_role()->set_roleID(roleID2);
		infoRecord.mutable_info1()->mutable_info()->mutable_role()->set_loveHeart(flag);
		infoRecord.mutable_info1()->mutable_info()->mutable_role()->set_name(player2->att.att().baseAtt().name());
		infoRecord.mutable_info1()->mutable_info()->mutable_role()->set_professionType((PB_ProfessionInfo::Type)player2->att.att().baseAtt().professionType());
		infoRecord.mutable_info1()->mutable_info()->set_arg1(arg1);

		arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10);

		infoRecord.mutable_info2()->set_flag(true);
		infoRecord.mutable_info2()->mutable_info()->mutable_role()->set_roleID(roleID1);
		infoRecord.mutable_info2()->mutable_info()->mutable_role()->set_loveHeart(!flag);
		infoRecord.mutable_info2()->mutable_info()->set_arg1(arg1);

		GCAgent_SendProtoToDCAgent(&infoRecord);
	} else if (player1 == NULL && player2 == NULL) {
		/*
		   info.mutable_role().set_roleID(roleID2);
		   info.mutable_role().set_loveHeart(flag);
		   int64_t arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10);
		   info.set_arg1(arg1);

		   infoRecord.Clear();
		   infoRecord.mutable_info1()->set_flag(false);
		   infoRecord.mutable_info1()->set_info(info);

		   info.mutable_role().set_roleID(roleID1);
		   info.mutable_role().set_loveHeart(!flag);
		   arg1 = ( ((int64_t)Time_TimeStamp()) << 32 ) + (abs(up) << 10);
		   info.set_arg1(arg1);

		   infoRecord.mutable_info2()->set_flag(false);
		   infoRecord.mutable_info2()->set_info(info);

		   GCAgent_SendProtoToDCAgent(&infoRecord);
		   */
	}

}

void PlayerEntity_SaveOnlineGodRecord(const DCProto_SaveGodRankInfoRecord* info) {
	if (info == NULL) {
		return;
	}

	if (info->info1().flag()) {
		PlayerEntity *player = PlayerEntity_PlayerByRoleID(info->info2().info().role().roleID());
		if (player != NULL) {
			int index = player->att.godRankIndex(); 
			FriendInfo friendInfo;
			friendInfo.FromPB(&info->info1().info().role());
			*(player->att.mutable_godRecordInfo(index % player->att.godRecordInfo_size())) = friendInfo;
			player->att.set_godRecordArg1(index % player->att.godRecordArg1_size(), info->info1().info().arg1());
			player->att.set_godRankIndex(index + 1);
		}
	}

	if (info->info2().flag()) {
		PlayerEntity *player = PlayerEntity_PlayerByRoleID(info->info1().info().role().roleID());
		if (player != NULL) {
			int index = player->att.godRankIndex(); 
			FriendInfo friendInfo;
			friendInfo.FromPB(&info->info2().info().role());
			*(player->att.mutable_godRecordInfo(index % player->att.godRecordInfo_size())) = friendInfo;
			player->att.set_godRecordArg1(index % player->att.godRecordArg1_size(), info->info2().info().arg1());
			player->att.set_godRankIndex(index + 1);
		}
	}
}

void PlayerEntity_GodRecordsRequest(struct PlayerEntity* entity, NetProto_GodRecords* info) {
	if (!PlayerEntity_IsValid(entity)) {
		return;
	}

	if (info == NULL) 
		return;

	for (int i = 0; i < entity->att.godRecordInfo_size() && i < entity->att.godRankIndex(); ++i) {
		int64_t v = entity->att.godRecordArg1(i);
		/*
		   info->set_flag(i, entity->att.godRecordInfo(i).loveHeart());		
		   info->set_num(i, v & 0x3FF);		
		   info->set_professionType(i, (PB_ProfessionInfo::Type)entity->att.godRecordInfo(i).professionType());		
		   info->set_level(i, (v & 0x0FFFFFFFF) >> 10);		
		   info->set_name(i, entity->att.godRecordInfo(i).name());		
		   info->set_time(i, (v >> 32));
		   info->set_roleID(i, entity->att.godRecordInfo(i).roleID());	
		   */
		info->add_flag(entity->att.godRecordInfo(i).loveHeart());		
		info->add_num(v & 0x3FF);		
		info->add_professionType((PB_ProfessionInfo::Type)entity->att.godRecordInfo(i).professionType());		
		info->add_level((v & 0x0FFFFFFFF) >> 10);		
		info->add_name(entity->att.godRecordInfo(i).name());		
		info->add_time((v >> 32));
		info->add_roleID(entity->att.godRecordInfo(i).roleID());
		DEBUG_LOG("MMMMMMMMMMMM%d",entity->att.godRecordInfo(i).roleID());
	}
}

void PlayerEntity_GodPanel(struct PlayerEntity* entity, NetProto_GodPanel* info) {
	if (!PlayerEntity_IsValid(entity)) {
		return;
	}

	if (info == NULL) 
		return;

	info->set_nextTime(entity->att.godDueDate());
	int value = entity->att.dayEvent(29) & 0x0FFFF;
	info->set_num(value);
}

void PlayerEntity_HistoryRecordAward(int64_t roleID, int rank, int& score) {
	PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	if (entity == NULL) {
		return;
	}

	int v = entity->att.minGodRank();
	if (v == -1) {
		entity->att.set_minGodRank(rank);
	} else {
		if (v >= rank) {
			entity->att.set_minGodRank(rank);
		}
	}

	if (rank >= 0 && rank < 10) {
		if (!entity->att.godRankRecords(rank)) {
			int money = 0;
			int godScore = 0;
			entity->att.set_godRankRecords(rank, true);
			Config_GodRankAward(rank, money, godScore);
			score = godScore;
			Item_ModifyGodScore(entity->component.item, godScore);
			Item_ModifyRMB(entity->component.item, money, false, -8, 0, 0);
		}
	}

	if (rank < v) {
		entity->att.set_minGodRank(rank);
		//	int num = sqrt(sqrt( 10 * sqrt(v-rank))) * 1000 / rank + 1;
		int num = 500 * (v - rank) / (v + rank);
		score += num;
		Item_ModifyGodScore(entity->component.item, num);
		Item_ModifyRMB(entity->component.item, num, false, -8, 0, 0);
	}

}

void PlayerEntity_ResetGodTime(struct PlayerEntity* entity) {
	if (entity != NULL) 
		entity->att.set_godDueDate(0);
}

int PlayerEntity_Treasure(struct PlayerEntity* entity, NetProto_Treasure* info) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	if (info == NULL) {
		return -2;
	}

	if (!(info->count() == 1 || info->count() == 10)) {
		return -3;
	}


	if (!(info->index() == 1 || info->index() == 2)) {
		return -4;
	}

	int32_t results[CONFIG_FIXEDARRAY];
	static NetProto_GetRes gr;
	gr.Clear();
	static NetProto_TreasureBox grco;
	grco.Clear();

	if (info->index() == 1) {
		bool costRMB = true;
		if (info->count() == 1) {
			int timer = entity->component.playerAtt->fixedEvent(53);
			if (timer > Time_TimeStamp()) {
				int count = entity->component.playerAtt->fixedEvent(110);
				if ((count & 0XFFFF) > 0 ) {
					PlayerEntity_SetFixedEvent(entity, 110, count - 1);
				}else {
					int rmb = Config_Treasure(1, 1);
					if (!Item_HasRMB(entity->component.item, rmb)) {
						return -5;
					}
					Item_ModifyRMB(entity->component.item, -rmb, false, 23, 0, 0);
				}
			}else {
				int value = Config_Treasure(-1, 1);
				PlayerEntity_SetFixedEvent(entity, 53, value);
				costRMB = false;
			}

			int flag = entity->component.playerAtt->fixedEvent(31);
			if ((flag & 0x020000) == 0) {
				PlayerEntity_SetFixedEvent(entity, 31, flag | 0x020000);
				Item_Lottery(entity->component.item, 5, results, CONFIG_FIXEDARRAY, false,&gr);
			}else {
				if (costRMB) {
					Item_Lottery(entity->component.item, 6, results, CONFIG_FIXEDARRAY, false,&gr);
				}else {
					Item_Lottery(entity->component.item, 4, results, CONFIG_FIXEDARRAY, false,&gr);
				}
			}
		}else if (info->count() == 10) {
			int rmb = Config_Treasure(1, 10);
			if (!Item_HasRMB(entity->component.item, rmb)) {
				return -6;
			}
			Item_ModifyRMB(entity->component.item, -rmb, false, 23, 0, 0);
			for (int i = 0; i < info->count(); ++i) {
				Item_Lottery(entity->component.item, 6, results, CONFIG_FIXEDARRAY, false,&gr);
			}
			int count = entity->component.playerAtt->fixedEvent(110);
			if ((count & 0XFFFF) < 0XFFFF ) {
				PlayerEntity_SetFixedEvent(entity, 110, count + 1);
			}
		}else {
			return -10;
		}
	} else if (info->index() == 2) {
		bool costRMB = true;
		if (info->count() == 1) {
			int timer = entity->component.playerAtt->fixedEvent(111);
			if (timer > Time_TimeStamp()) {
				int count = entity->component.playerAtt->fixedEvent(110);
				if ((count & 0X7FFF0000) > 0 ) {
					PlayerEntity_SetFixedEvent(entity, 110, count - 0x10000);
				}else {
					int rmb = Config_Treasure(2, 1);
					if (!Item_HasRMB(entity->component.item, rmb)) {
						return -5;
					}
					Item_ModifyRMB(entity->component.item, -rmb, false, 23, 0, 0);
				}
			}else {
				int value = Config_Treasure(-2, 1);
				PlayerEntity_SetFixedEvent(entity, 111, value);
				costRMB = false;
			}

			if (costRMB) {
				Item_Lottery(entity->component.item, 9, results, CONFIG_FIXEDARRAY, false,&gr);
			}else {
				Item_Lottery(entity->component.item, 8, results, CONFIG_FIXEDARRAY, false,&gr);
			}
		}else if (info->count() == 10) {
			int rmb = Config_Treasure(2, 10);
			if (!Item_HasRMB(entity->component.item, rmb)) {
				return -6;
			}
			Item_ModifyRMB(entity->component.item, -rmb, false, 23, 0, 0);
			for (int i = 0; i < info->count(); ++i) {
				Item_Lottery(entity->component.item, 9, results, CONFIG_FIXEDARRAY, false,&gr);
			}
			int count = entity->component.playerAtt->fixedEvent(110);
			if ((count & 0X7FFF0000) < 0X7FFF000 ) {
				PlayerEntity_SetFixedEvent(entity, 110, count + 0x10000);
			}
		}else {
			return -20;
		}
	}else {
		return -30;
	} 

	/*	
		for (int i = 0; i < info->count(); ++i) {
		if (Config_InActiveTime(28)) {
		if (entity->component.playerAtt->fixedEvent(100) > (int)Time_TimeStamp()) {
		PlayerEntity_SetFixedEvent(entity, 99, entity->component.playerAtt->fixedEvent(99) + 1);
		if (entity->component.playerAtt->fixedEvent(99) <= 250 )  {
		Item_Lottery(entity->component.item, 128, results, CONFIG_FIXEDARRAY, false ,&gr);
		}else if (entity->component.playerAtt->fixedEvent(99) <= 350 )  {
		Item_Lottery(entity->component.item, 129, results, CONFIG_FIXEDARRAY, false ,&gr);
		}else if (entity->component.playerAtt->fixedEvent(99) == 351 )  {
		Item_Lottery(entity->component.item, 130, results, CONFIG_FIXEDARRAY, false ,&gr);
		}else {
		Item_Lottery(entity->component.item, 128, results, CONFIG_FIXEDARRAY, false ,&gr);
		}
		}else {
		const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
		if (fixedMap == NULL) {
		return -71;
		}

		multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-28);
		if (begin == fixedMap->end()) {
		return -72;
		}
		PlayerEntity_SetFixedEvent(entity, 100, begin->second & 0xFFFFFFFF);
		PlayerEntity_SetFixedEvent(entity, 99, 1);
		Item_Lottery(entity->component.item, 6, results, CONFIG_FIXEDARRAY, false ,&gr);
		}

		}else {
		Item_Lottery(entity->component.item, 128, results, CONFIG_FIXEDARRAY, false ,&gr);
		}
		}
		*/

	for (int i = 0; i < gr.items_size(); ++i) {
		PB_ItemInfo* itemInfo = grco.add_items();
		*itemInfo = gr.items(i);
		DEBUG_LOG("AAAA:%lld", itemInfo->id());
	}
	GCAgent_SendProtoToClients(&entity->id, 1, &grco);

	return 0;
}

int PlayerEntity_Hire(struct PlayerEntity* entity, NetProto_Hire* info) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	if (info == NULL) {
		return -2;
	}

	int hireCount = entity->att.dayEvent(30) & 0xEF000000;
	if ((hireCount >> 24) >= 10) {
		static NetProto_Error error;
		error.set_content(Config_Words(67));
		GCAgent_SendProtoToClients(&entity->id, 1, &error);
		return -2;
	}

	if (info->index() == 0) {
		int64_t ids[MAX_HIREPERSONNUMBER] = {-1, -1, -1, -1, -1};
		int32_t power = Fight_Power(entity->component.fight);
		int res = PlayerPool_RandomHire(PlayerEntity_RoleID(entity), ids);
		if (res < 0) {
			DEBUG_LOG("hire  = %d", res);
		}

		static DCProto_LoadHireRoleDate proto;
		proto.Clear();
		for (int i = 0; i < MAX_HIREPERSONNUMBER; ++i) {
			if (*(ids + i) != -1) {
				proto.add_roleID(*(ids + i));		
			}
		}
		proto.set_id(PlayerEntity_ID(entity));
		proto.set_power(power);
		GCAgent_SendProtoToDCAgent(&proto);
		return 1;
	}else {
		int num = info->index() & 0x01;
		num += (info->index() & 0x02) >> 1;
		num += (info->index() & 0x04) >> 2;
		num += (info->index() & 0x08) >> 3;
		num += (info->index() & 0x10) >> 4;

		if (num > 2) {
			return -4;
		}

		int32_t multiRoom = Movement_MultiRoom(entity->component.movement);
		if (multiRoom == -1) {
			return -5;
		}
		RoomInfo *roomInfo = Movement_SearchRoomInfo(multiRoom);
		if (roomInfo == NULL) {
			return -6;
		}
		if (roomInfo->count() > 1) {
			return -7;
		}

		static Vector2i nextCoord;
		const MapInfo *mapInfo = MapInfoManager_MapInfo(roomInfo->map());
		if (mapInfo == NULL) {
			return -8;
		}

		if (!MapInfoManager_EnterCoord(mapInfo, 0, &nextCoord)) {
			return -9;
		}

		int32_t cost = 0;
		for (int i = 0; i < MAX_HIREPERSONNUMBER; ++i) {
			if (info->index() & (1 << i)) {
				if (entity->hirePower[i] != -1) {
					cost += entity->hirePower[i];
				}
			}
		}

		if (cost > Item_Package(entity->component.item)->money()) {
			return -10;
		}
		Item_ModifyMoney(entity->component.item, -cost);
		PlayerEntity_SetDayEvent(entity, 30, entity->att.dayEvent(30) + 0x01000000);
		Movement_BeginChangeScene(entity->component.movement, roomInfo->map(), &nextCoord, -1, roomInfo->noPower());
	}
	return 0;
}

void PlayerEntity_SetHire(struct PlayerEntity* entity, int index, int power) {
	if (!PlayerEntity_IsValid(entity))
		return;

	if (index < MAX_HIREPERSONNUMBER && index >= 0) {
		entity->hirePower[index] = power;
	}
}

int PlayerEntity_MoneyTree(struct PlayerEntity* entity, NetProto_MoneyTree *info) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	if (info == NULL) {
		return -2;
	}

	int totalCount = 0;
	int moneyTree = 0;
	int count = 0;
	int cost = 0;
	int level = entity->att.att().fightAtt().level();
	DEBUG_LOG("playerLevel == %d", level);
	int money = Config_MoneyTree(count, cost);
	DEBUG_LOG("baseMoney == %d", money);

	//（5W+等级*1000）*（1+今日已摇次数/20）
	if (info->index() == 0) {
		int freeBegin = entity->att.dayEvent(47);
		if (Time_TimeStamp() < freeBegin) {
			return -3;
		}

		int freeCount = (entity->att.dayEvent(35) & 0xFF00) >> 8;
		DEBUG_LOG("freeCount == %d", freeCount);
		if (freeCount >= 3) {
			return -4;
		}

		count = (entity->att.dayEvent(34) & 0x7FFF0000) >> 16;
		DEBUG_LOG("count == %d", count);
		totalCount = freeCount + count;
		DEBUG_LOG("totalCount == %d", totalCount);

		if ((totalCount + 1) % 4 == 0) {
			moneyTree = 2 * (money + level * 1000) * (1 + (float)totalCount / 20);
			DEBUG_LOG("moneyTree == %d", moneyTree);
		} else {
			moneyTree = (money + level * 1000) * (1 + (float)totalCount / 20);
			DEBUG_LOG("moneyTree == %d", moneyTree);
		}

		Item_ModifyMoney(entity->component.item, moneyTree);
		PlayerEntity_SetDayEvent(entity, 47, Time_TimeStamp() + 20 * 60);
		PlayerEntity_SetDayEvent(entity, 35, entity->att.dayEvent(35) + 0x100);
	}else if (info->index() == 1){
		int vipCount = 0;
		const VIPInfo *vip = VIPInfoManager_VIPInfo(entity->att.itemPackage().vip());
		if (vip != NULL)
			vipCount = vip->moneyTreeCount();

		count = (entity->att.dayEvent(34) & 0x7FFF0000) >> 16;
		DEBUG_LOG("count == %d", count);
		if (count >= vipCount) {
			return -5;
		}

		money = Config_MoneyTree(count, cost);
		DEBUG_LOG("baseMoney == %d", money);

		if (!Item_HasRMB(entity->component.item, cost)) {
			return -5;
		}
		Item_ModifyRMB(entity->component.item, -cost, false, 24, 0, 0);
		PlayerEntity_SetDayEvent(entity, 34, entity->att.dayEvent(34) + 0x10000);

		int freeCount = (entity->att.dayEvent(35) & 0xFF00) >> 8;
		DEBUG_LOG("freeCount == %d", freeCount);
		totalCount = freeCount + count;
		DEBUG_LOG("totalCount == %d", totalCount);

		if ((totalCount + 1) % 4 == 0) {
			moneyTree = 2 * (money + level * 1000) * (1 + (float)totalCount / 20);
			DEBUG_LOG("moneyTree == %d", moneyTree);
		} else {
			moneyTree = (money + level * 1000) * (1 + (float)totalCount / 20);
			DEBUG_LOG("moneyTree == %d", moneyTree);
		}
		Item_ModifyMoney(entity->component.item, moneyTree);
	}else {
		return -6;
	}


	DEBUG_LOG("moneyTree == %d", moneyTree);
	static NetProto_GetRes gr;
	gr.Clear();
	PB_ItemInfo *item = gr.add_items();
	item->set_type(PB_ItemInfo::MONEY);
	item->set_count(moneyTree);
	GCAgent_SendProtoToClients(&entity->id, 1, &gr);

	return 0;
}

int PlayerEntity_TopUpObtRMB(struct PlayerEntity* entity, NetProto_TopUpObtRMB* info) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	if (info == NULL) {
		return -2;
	}

	const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
	if (fixedMap == NULL) {
		return -3;
	}

	multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-6);
	if (begin == fixedMap->end()) {
		return -4;
	}

	const ItemPackage *package = Item_Package(entity->component.item);
	if (package == NULL) {
		return -5;
	}

	int bit = entity->component.playerAtt->fixedEvent(68);
	int count = Config_TopUpObtRMB(package->rmbActive(), bit);
	PlayerEntity_SetFixedEvent(entity, 68, bit);
	Item_ModifyRMB(entity->component.item, count, false, -9, 0, 0);

	static NetProto_GetRes proto;
	proto.Clear();
	PB_ItemInfo *unit = proto.add_items();
	unit->set_type(PB_ItemInfo::SUBRMB);
	unit->set_count(count);
	GCAgent_SendProtoToClients(&entity->id, 1, &proto);

	return 0;
}




int PlayerEntity_PetPsychicsLevelUp(struct PlayerEntity* entity, NetProto_PetPsychicsLevelUp* info) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	for (int i = 0; i< 6; ++i) {
		DEBUG_LOG("GGGGGGGGGGGGGGGGG%d == %d", i, entity->att.haloValue(i));
	}		
	if (info == NULL) {
		return -2;
	}

	int group = info->index();
	if (group < 0 || group > 5)
		return -2;

	for (int i = 0; i< 6; ++i) {
		DEBUG_LOG("GGGGGGGGGGGGGGGGG%d == %d", i, entity->att.haloCount(i));
	}		
	int haloCount = entity->att.haloCount(group);
	entity->att.set_haloCount(group, haloCount + 1);
	for (int i = 0; i< 6; ++i) {
		DEBUG_LOG("GGGGGGGGGGGGGGGGG%d == %d", i, entity->att.haloCount(i));
	}		

	int level = entity->att.haloLevel(group);
	int levelUp = 0;
	int randNum = Time_Random(0, 100);

	const std::map<int, map<int, PB_PetHaloInfo> >* petHaloInfo = NPCInfoManager_PetHaloInfo();
	if (petHaloInfo == NULL) {
		return -3;
	}

	map<int, map<int, PB_PetHaloInfo> >::const_iterator itGroup = petHaloInfo->find(group);
	if (itGroup == petHaloInfo->end()) {
		return -3;
	}

	int roleLevel = entity->att.att().fightAtt().level();
	if (level >= roleLevel) {
		map<int, PB_PetHaloInfo>::const_iterator it = itGroup->second.find(level);
		if (it->second.maxToughness() <= entity->att.haloValue(group)) {
			return -3;
		}
	}

	int add = 0;
	map<int, PB_PetHaloInfo>::const_iterator itLevel = itGroup->second.find(level + 1);
	if (itLevel == itGroup->second.end()) {
		itLevel = itGroup->second.find(level);
		add = itLevel->second.maxToughness() - entity->att.haloValue(group);
		if (add == 0) {
			return -4;
		}
		if (level >= roleLevel) {
			return -4;
		}

		levelUp = 1;
	}else {
		itLevel = itGroup->second.find(level);
		add = itLevel->second.maxToughness() - entity->att.haloValue(group);
		if (add == 0) {
			if (level >= roleLevel) {
				return -4;
			}
			levelUp = 2;
		}
	}

	itLevel = itGroup->second.find(level);
	if (itLevel == itGroup->second.end()) {
		return -5;
	}

	int goodID = itLevel->second.requiredGoodsID();
	int count = itLevel->second.GoodsCount();
	int money = itLevel->second.Gold();

	if (levelUp == 0) {
		if (randNum <= Config_RandomPetHalo()) {
			levelUp = 1;
		}

		if (!Item_HasItem(entity->component.item, ItemInfo::GOODS, goodID, count)) {
			return -6;
		}

		if (money > Item_Package(entity->component.item)->money()) {
			return -7;
		}

		Item_ModifyMoney(entity->component.item, -money);
		Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, (int64_t)goodID, count, true);

		if (levelUp) {
			if (level >= roleLevel) {
				return -8;
			}

			entity->att.set_haloLevel(group, level + 1);
			return 1;
		}else {
			int res = Config_RandomPetHaloInfo(randNum);
			if (res > add) 
				res = add;

			entity->att.set_haloValue(group, entity->att.haloValue(group) + res);
			return (1 << 16) + res;
		}
	}else {
		if (levelUp == 1) {
			if (!Item_HasItem(entity->component.item, ItemInfo::GOODS, goodID, count * 3)) {
				return -8;
			}

			if (money * 3 > Item_Package(entity->component.item)->money()) {
				return -9;
			}

			Item_ModifyMoney(entity->component.item, -money * 3);
			Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, (int64_t)goodID, count * 3, true);

			int res = Config_RandomPetHaloInfo(randNum);
			if (res < 2)
				res = 2;

			if (res > add) 
				res = add;

			entity->att.set_haloValue(group, entity->att.haloValue(group) + res);
			return (1 << 16) + res;
		}else if (levelUp == 2) {
			if (!Item_HasItem(entity->component.item, ItemInfo::GOODS, goodID, count)) {
				return -10;
			}

			if (money > Item_Package(entity->component.item)->money()) {
				return -11;
			}

			Item_ModifyMoney(entity->component.item, -money);
			Item_DelFromPackage(entity->component.item, ItemInfo::GOODS, (int64_t)goodID, count, true);
			entity->att.set_haloLevel(group, level + 1);
			return 1;
		}else {}

	}
	return 0;
}

/*
   void PlayerEntity_PetPower(struct PlayerEntity* entity, int index) {
   if (!PlayerEntity_IsValid(entity)) {
   return;
   }

   const std::map<int, map<int, PB_PetHaloInfo> >* petHaloInfo = NPCInfoManager_PetHaloInfo();
   if (petHaloInfo == NULL) {
   return;
   }

   int value[PB_FightAtt_PropertyType_PropertyType_ARRAYSIZE] = {0};
   for (int i = 0; i < entity->att.haloLevel_size() && i < entity->att.haloValue_size(); ++i) {
   map<int, map<int, PB_PetHaloInfo> >::const_iterator itGroup = petHaloInfo->find(i);
   if (itGroup == petHaloInfo->end()) {
   continue;
   }

   map<int, PB_PetHaloInfo>::const_iterator itUnit = itGroup->second.find(entity->att.haloLevel(i));
   if (itUnit == itGroup->second.end()) {
   continue;
   }
   value[i] = itUnit->second.propertyValue() * (1 + entity->att.haloValue(i) / 10000.0f);
   }

   const NPCAtt* pet = NULL;
   char ally[128];
   int quality = 0;
   int level = 0;
   PB_NPCAtt npcAtt;

   for (int i = 0; i < entity->att.pets_size(); ++i) {
   if (index == -2) {
   if (entity->att.pets(i).id() != -1) {
   quality = entity->att.pets(i).quality();
   level = entity->att.pets(i).level();

   pet =  NPCInfoManager_Pet(entity->att.pets(i).id(), quality, level);
   if (pet == NULL) {
   continue;
   }

   SNPRINTF1(ally,"%s,%s", entity->att.att().baseAtt().name(), pet->att().baseAtt().name());
   pet->ToPB(&npcAtt);
   PB_FightAtt fightAtt = npcAtt.att().fightAtt();

   for (int i = 0; i < (int)(sizeof(value) / sizeof(int)); i++) {
   fightAtt.set_properties(i, fightAtt.properties(i) + value[i]);
   }
   int32_t power = Fight_Power(&fightAtt);
   PlayerPool_UpdatePetRank(ally, power);
   }
   }else {
   if (index == entity->att.pets(i).id() && index != -1) {
   quality = entity->att.pets(i).quality();
   level = entity->att.pets(i).level();

   pet =  NPCInfoManager_Pet(entity->att.pets(i).id(), quality, level);
   if (pet == NULL) {
   continue;
   }

   SNPRINTF1(ally,"%s,%s", entity->att.att().baseAtt().name(), pet->att().baseAtt().name());
   pet->ToPB(&npcAtt);
   PB_FightAtt fightAtt = npcAtt.att().fightAtt();

   for (int i = 0; i < (int)(sizeof(value) / sizeof(int)); i++) {
   fightAtt.set_properties(i, fightAtt.properties(i) + value[i]);
   }
   int32_t power = Fight_Power(&fightAtt);
   PlayerPool_UpdatePetRank(ally, power);

}
}
}
}
*/

int PlayerEntity_Luck(struct PlayerEntity* entity, NetProto_Luck* info) {
	if (!PlayerEntity_IsValid(entity)) {
		return -1;
	}

	if (info == NULL) 
		return -2;

	if (!Config_InActiveTime(20)) {
		return -2;
	}

	int luck = entity->att.fixedEvent(88);
	int32_t results[CONFIG_FIXEDARRAY];
	static NetProto_GetRes gr;
	gr.Clear();

	if (info->free()) {
		int activeTime = entity->att.dayEvent(67);
		int count = (entity->att.dayEvent(66) & 0x7FFF0000) >> 16;;
		if (Time_TimeStamp() < activeTime) {
			return -3;
		}

		if (count >= Config_LuckFreeCount()) {
			return -4;
		}

		PlayerEntity_SetDayEvent(entity, 66, (entity->att.dayEvent(66) & 0xFFFF) | ((++count) << 16) );
		PlayerEntity_SetDayEvent(entity, 67, Time_TimeStamp() + Config_LuckCDTime());
	}else {
		if (!Item_HasRMB(entity->component.item, Config_LuckCost()))
			return -10;
		Item_ModifyRMB(entity->component.item, -Config_LuckCost(), false, 12, 0, 0);
	}

	PlayerEntity_SetFixedEvent(entity, 88, luck + 1);
	Item_Lottery(entity->component.item, 125, results, CONFIG_FIXEDARRAY, true, &gr);
	return 0;
}

const char * PlayerEntity_SelfInviteCode(int64_t roleID) {
	struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	const PlayerInfo* info = AccountPool_IDToAccount(PlayerEntity_ID(entity));
	if (info == NULL)
		return NULL;

	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, "bjylx(line:%d,roleID:%lld,platform:%d)", Config_Line(), (long long)roleID, Config_ChannelNum(info->platform().c_str()));
	string final = md5(buf);
	string code = final.substr(8, 16);
	entity->att.set_selfcode(code.c_str());
	return code.c_str();
}

void PlayerEntity_OtherInviteCode(int64_t roleID, const char * str, int64_t roleIDOther) {
	if (str == NULL)
		return;

	struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	if (entity != NULL) {
		entity->att.set_othercode(str);
	}

	if (entity->att.att().fightAtt().level() >= Config_InviteCodeLevel()) {
		PlayerEntity_AddInviteCode(roleIDOther);
	}
}

void PlayerEntity_SysOnlineInvateCode(struct PlayerEntity * entity, const char * str) {
	if (!PlayerEntity_IsValid(entity)) {
		return;
	}

	if (str == NULL)
		return;

	entity->att.set_selfcode(str);

} 

void PlayerEntity_AddInviteCode(int64_t roleID) {
	struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(roleID);
	if (entity != NULL) {
		int count = entity->att.fixedEvent(95) & 0xFFFF;
		if (count != 0xFFFF) {
			PlayerEntity_SetFixedEvent(entity, 95, (count + 1) | ((entity->att.fixedEvent(95) & 0xFFFF0000)));
			static NetProto_UpdateInvateCount proto;
			proto.Clear();

			proto.set_count(count + 1);
			GCAgent_SendProtoToClients(&entity->id, 1, &proto);
		}
	}else {
		static DCProto_AddCodeCount proto;
		proto.Clear();

		proto.set_roleID(roleID);
		GCAgent_SendProtoToDCAgent(&proto);
	}
}
