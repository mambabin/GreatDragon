#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

#include "NetProto.pb.h"
#include "EquipmentInfo.hpp"
#include "ConfigUtil.hpp"
#include "MapPool.hpp"
#include "VIPInfoManager.hpp"
#include "NetProto.pb.h"
#include "FightInfo.hpp"
#include "Award.pb.h"
#include "Debug.hpp"
#include "Time.hpp"
#include "Event.hpp"
#include <map>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/stat.h>

#define CONFIG_MAX_MSG_SIZE (1024 * 1024)
#define CONFIG_EXIT_CODE -2
#define CONFIG_FIXEDARRAY (1024 * 5)
#define CONFIG_MAXACCOUNT 32
#define CONFIG_INIT_USE_TIME (-60 * 1000)
#define MAXSTEPS 128
#define SOUL_RATE_LEVEL 4
#define MAX_NORMAL_TEAM_PLAYERS 3
#define MAX_MUL_TOWER_TEAM_PLAYERS 10
#define MIN_TEAM_PLAYERS 2
#define MAX_ROOM_PLAYERS 15
#define STRONG_GROUP_ATT 2
#define MAX_BLOOD_NODE_LINE 7
#define MAX_LOVEHEART 100
#define LOCAL_PLATFORM "ylx"
#define FRIENDLY_NPC_FACTION (1 << 30)
#define MAX_ROLE_PER_ACCOUNT 3
#define MAX_HIREPERSONNUMBER 5

#define SNPRINTF1(buf, format, ...) {snprintf(buf, sizeof(buf), format, ##__VA_ARGS__); buf[sizeof(buf) - 1] = 0;}

#define SNPRINTF2(buf, index, format, ...) {if (sizeof(buf) > (index - buf)) {snprintf(index, sizeof(buf) - (index - buf), format, ##__VA_ARGS__); buf[sizeof(buf) - 1] = 0;}}

struct SnapshotData {
	std::string host;
	int port;
	std::string user;
	std::string passwd;
	std::string db;
};

void Config_Init();
void Config_Reload(void *arg);
void Config_ReloadChannelData();
void Config_ReloadSnapshotData();
const std::vector<struct SnapshotData> * Config_Snapshots();

const char * Config_DataPath();
const char * Config_Words(int i);

static inline void Config_LockFile(FILE *file) {
	// flock does not work through nfs, use fcntl instead of it
	// flock(fileno(file), LOCK_EX);
	struct flock flock_;
	flock_.l_type = F_WRLCK;
	flock_.l_whence = SEEK_SET;
	flock_.l_start = 0;
	flock_.l_len = 0;
	fcntl(fileno(file), F_SETLKW, &flock_);
}
static inline void Config_UnlockFile(FILE *file) {
	// flock does not work through nfs, use fcntl instead of it
	// flock(fileno(file), LOCK_UN);
	struct flock flock_;
	flock_.l_type = F_UNLCK;
	fcntl(fileno(file), F_SETLKW, &flock_);
}

static inline const char * Config_EquipColorStr(EquipmentInfo::ColorType color) {
	if (color == EquipmentInfo::WHITE)
		return "[ffffff]";
	else if (color == EquipmentInfo::GREEN)
		return "[60dd62]";
	else if (color == EquipmentInfo::BLUE)
		return "[2d90ff]";
	else if (color == EquipmentInfo::YELLOW)
		return "[bb1fe2]";
	else if (color == EquipmentInfo::RED)
		return "[ff6000]";
	return "[ffffff]";
}

static inline const char * Config_RankStr(NetProto_Rank::Type type) {
	if (type == NetProto_Rank::POWER)
		return Config_Words(5);
	else if (type == NetProto_Rank::TOWER)
		return Config_Words(7);
	else if (type == NetProto_Rank::LEVEL)
		return Config_Words(4);
	else if (type == NetProto_Rank::GOD)
		return Config_Words(31);
	return "";
}

static inline int Config_Divisor() {
	return 100;
}

int Config_NetAgentPort();
int64_t Config_Version();
int Config_Line();
bool Config_HasPlatform(const char *platform);

const char * Config_DBHost();
int Config_DBPort();
const char * Config_DB();
const char * Config_DBUser();
const char * Config_DBPassword();

const char * Config_DBPublicHost();
int Config_DBPublicPort();
const char * Config_DBPublic();
const char * Config_DBPublicUser();
const char * Config_DBPublicPassword();

const char * Config_DBRechargeHost();
int Config_DBRechargePort();
const char * Config_DBRecharge();
const char * Config_DBRechargeUser();
const char * Config_DBRechargePassword();

const char * Config_RechargeTable();
const char * Config_AccountTable();
const char * Config_RoleTable();
const char * Config_SingleTable();
const char * Config_GiftTable();
const char * Config_FactionTable();
const char * Config_CostRecordTable();
const char * Config_ChatTable();
const char * Config_RekooRoleTable();
const char * Config_ActivateKeyTable();
bool Config_LogInfo();
bool Config_LogError();
bool Config_LogRecord();
bool Config_IsGM(const char *account, const char *passwd);
const char * Config_GMTable();
const char * Config_GMDataTable();
const char * Config_DeviceTable();
int Config_ServerStartTime();
static inline int Config_RMBToGem() {
	return 10;
}

static inline const char * Config_GID() {
	return "20034";
}
static inline const char * Config_GameKey() {
	return "7d8009037fe86ed9d55ec4bdcde4694e";
}

static inline const char * Config_AppID(const char *platform) {
	if (strcmp(platform, "QQ") == 0)
		return "1104057244";
	else if (strcmp(platform, "WX") == 0)
		return "wx8599aca7e94a7670";
	else
		return "";
}

static inline const char * Config_AppKey(const char *platform) {
	if (strcmp(platform, "QQ") == 0)
		return "Dyj0Ae99MDkvDlqx";
	else if (strcmp(platform, "WX") == 0)
		return "967298ac505cbff9249989ac28fa17de";
	else
		return "";
}

int Config_OpenTime(char **str);

bool Config_InWhiteList(const char *account);

const char * Config_RandomName();

const std::vector<MailGift> * Config_MailGift(MailGift::Type type);

int Config_MaxPlayers();
int Config_MaxNPCs();
int Config_MaxComponents();
int Config_MaxSkills();
int Config_MaxStatuses();

int Config_EquipInheritPercent();
int Config_EquipRandomPercent2();
int Config_EquipRandomPercent3();

// NULL if close scribe
const char * Config_ScribeHost();
int Config_ScribePort();
static inline int Config_ScribeTimeout() {
	// ms
	return 1000;
}
const char * Config_ScribeCategory();

const char * Config_SnapshotDir();

const char * Config_Push();
const char * Config_PushDir();

static inline int Config_IDInterval() {
	return 1000 * 60 * 2;
}

static inline int Config_PoolStep() {
	return 1;
}

int Config_MaxMaps();

static inline int Config_DayRoomCount()
{
	return 5;
}

static inline int Config_InvalidPrice()
{
	return 99999999;
}

static inline int Config_MaxSizeOfMap() {
	// return 350 * 350;
	return 1;
}

static inline int Config_SaveInterval() {
	return 1000 * 60 * 1;
}

static inline int Config_SaveToDBInterval() {
	return 60 * 30;
}

static inline int Config_WorldChatInterval() {
	return 1000 * 10;
}

static inline int Config_SecretChatInterval() {
	return 1000 * 5;
}

static inline int Config_HeartbeatTime() {
	return 1000 * 60 * 3;
}

static inline int Config_HeartbeatInterval() {
	return 1000 * 20;
}

static inline int Config_SpeedUpInterval() {
	return 1000 * 15;
}

static inline int Config_SpeedUpCount() {
	return 5;
}

static inline int Config_DropToLogout() {
	return Config_IDInterval() - 1000 * 10;
}

static inline int Config_GemToMoney() {
	return 5000;
}

static inline int Config_MaxMana() {
	return 100;
}

static inline int Config_MaxEnergy() {
	return 100;
}

static inline int Config_PublicCD() {
	return 500;
}

static inline int Config_StatusStaticTime() {
	//return 800;
	return 300;
}

static inline int Config_StatusSkyTime() {
	return 1500;
}

static inline int Config_StatusStaticSkyTime() {
	return 1000;
}

static inline int Config_StatusFlyAwayTime() {
	//return 1500;
	return 600;
}

static inline int Config_StatusFlyAwayDist() {
	return 4;
}

static inline int Config_StatusGroundTime() {
	//return 1000;
	return 0;
}

static inline int Config_StatusStandupTime() {
	//return 1000;
	return 500;
}

static inline int Config_TransformTime() {
	return 1000 * 12;
}

static inline float Config_TransformEffect() {
	return 0.5f;
}

static inline int Config_MaxFreeRecoverCount() {
	return 1;
}

static inline int Config_RecoverGem() {
	return 5;
}

static inline int Config_SoulCount() {
	return 4;
}

static inline float Config_NormalExploreRate() {
	return 0.1f;
}

static inline float Config_HighExploreRate() {
	return 0.2f;
}

static inline int Config_NormalExploreMoney() {
	return 500;
}

static inline int Config_MaxNormalExplore() {
	return 20;
}

static inline int Config_MaxFreeNormalExplore() {
	return 5;
}

static inline int Config_HighExploreGem() {
	return 2;
}

static inline int Config_MaxHighExplore() {
	return 10;
}

static inline int Config_MaxHighExploreGem() {
	return 10;
}

static inline int Config_MaxPlayersOfLineInPeace() {
	return 20;
}

static inline int Config_MaxPlayersOfLineInWorldBoss() {
	return MAX_ROOM_PLAYERS;
}

static inline int64_t Config_MaxWaitRoomTime() {
	return 1000 * 60 * 15;
}

static inline int64_t Config_WaitRoomInterval() {
	return 1000 * 15;
}

static inline int64_t Config_WaitPVPInterval() {
	return 1000 * 15;
}

static inline int Config_MaxDurability(int vip) {
	int base = 200;
	if (vip >= 0) {
		const VIPInfo *info = VIPInfoManager_VIPInfo(vip);
		assert(info != NULL);
		base += info->durabilityDelta();
	}
	return base;
}

static inline int Config_DurabilityDelta() {
	return 10;
}

static inline int Config_MaxDurabilityDelta() {
	return 50;
}

static inline int Config_AutoRecoverDurability() {
	return 1;
}

static inline int Config_TowerBegin() {
	return 200;
}

static inline int Config_TowerEnd() {
	return 399;
}

static inline int Config_SurviveMap() {
	return 199;
}
static inline int Config_SurviveCount() {
	return 3;
}

static inline int Config_GodMap() {
	return 198;
}

static inline int Config_ReservationMap() {
	return 192;
}

static inline int64_t Config_PlayerReviveTime() {
	return 30 * 1000;
}

static inline int Config_BossBegin() {
	return 400;
}
static inline int Config_BossEnd() {
	return 499;
}
static inline int Config_BossCount() {
//	return 3;
	return 5;
}

static inline int Config_PVPMap() {
	return 194;
}
static inline int Config_PVPCount() {
	return 100;
}
static inline float Config_PVPScoreBase(int count) {
	if (count <= 20)
		return 2.0f;
	else if (count <= 50)
		return 1.0f;
	else if (count <= 100)
		return 0.5f;
	else
		return 0.0f;
}
static inline int Config_MaxWaitPVPTime() {
	return 1000 * 15;
}

static inline int Config_PVPDurability() {
	return 10;
}

static inline int Config_PVPBaseHonor() {
	return 2;
}

static inline int Config_PVPBaseScore() {
	return 1;
}

static inline int Config_PracticeMap() {
	return 195;
}

static inline int Config_RankCount() {
	return 20;
}

static inline int Config_InitRankRoleCount() {
	return 1000;
}

static inline int32_t Config_LuckyBox() {
	return 2;
}

static inline int Config_RecoverDurabilityGem() {
	return 50;
}

static inline int Config_RecoverDurability() {
	return 100;
}

static inline int Config_RecoverDurabilityCount(int vip) {
	int ret = 0;
	if (vip >= 0) {
		const VIPInfo *info = VIPInfoManager_VIPInfo(vip);
		assert(info != NULL);
		ret += info->buyDurability();
	}
	return ret;
}

static inline int Config_SingleBegin() {
	return 10;
}

static inline int Config_SingleEnd() {
	return 120;
}

static inline int Config_RoomBegin() {
	return 600;
}

static inline int Config_RoomEnd() {
	return 799;
}

static inline int Config_SingleEnhanceBegin() {
	return 800;
}
static inline int Config_SingleEnhanceEnd() {
	return 999;
}
static inline int Config_SingleEnhanceCount() {
	return 3;
}
static inline int Config_ResetSingleEnhanceCost() {
	return 100;
}

static inline int Config_IgnorePKInterval() {
	return 1000 * 30;
}

static inline float Config_FrontSpeed() {
	return 0.5f;
}

static inline int Config_MaxNPCsPerPass() {
	return 6;
}

static inline int64_t Config_DesignationInterval() {
	return 1000 * 60;
}

static inline int64_t Config_WingInterval() {
	return 1000 * 60;
}

static inline int Config_MaxShowDesignation() {
	return 1;
}

static inline int32_t Config_RankDesignation(NetProto_Rank::Type type, int n) {
	if (type == NetProto_Rank::POWER)
		return 3 + n;
	else if (type == NetProto_Rank::TOWER)
		return 6 + n;
	else if (type == NetProto_Rank::LEVEL)
		return 0 + n;
	return -1;
}

static inline int Config_RidesPotentialExpend()
{
	return 10;
}

static inline int Config_RidesBaseFood()
{
	return 5;
}

static inline int Config_RidesHighFood()
{
	return 50;
}

static inline int Config_RidesHighRMB()
{
	return 10;
}

static inline int Config_FreeReviveCount() {
	return 1;
}

static inline int Config_ReviveBaseCount()
{
	return 2;
}

static inline int Config_ReviveFirst()
{
	return 10;
}

static inline int Config_ReviveCost() {
	return 20;
}
static inline int64_t Config_PetReviveGoods() {
	return 489;
}
static inline int Config_PetReviveCount() {
	return 3;
}

static inline int Config_Stamp() {
	return 1;
}

static inline int Config_MailCost() {
	return 10000;
}

static inline int Config_MinRankLevel() {
	return 20;
}

static inline int Config_MinRankPower() {
	return 2000;
}

static inline int Config_MinRankTower() {
	return 5;
}

static inline int Config_MinRankGod() {
	return 1;
}

static inline int Config_MinRankWorldBoss() {
	return 0;
}

static inline int Config_MaxDCRoleCache() {
	return 40;
}

static inline int Config_MaxDCEquipCache() {
	return 40;
}

static inline bool Config_CanNoticeStrong(const EquipmentInfo *info, const EquipAsset *asset) {
	return info != NULL
		&& (int)info->colorType() >= (int)EquipmentInfo::RED
		&& asset->strongLevel() >= 10;
}

static inline bool Config_CanNoticeBlood(int bloodLevel) {
	return bloodLevel >= 6;
}

static inline AwardInfo::Type Config_InvestAward(int id) {
	if (id == 1)
		return AwardInfo::INVEST_1;
	else if (id == 2)
		return AwardInfo::INVEST_2;
	else if (id == 3)
		return AwardInfo::INVEST_3;
	else
		return AwardInfo::INVEST_1;
}

static inline int Config_InvestCost(int id) {
	if (id == 1)
		return 1000;
	else if (id == 2)
		return 5000;
	else if (id == 3)
		return 10000;
	else
		return -1;
}

static inline int Config_FirstRecharge() {
	return 2;
}
static inline bool Config_CanFirstRecharge(int rmb) {
	return rmb == 180
		|| rmb == 480
		|| rmb == 1580
		|| rmb == 2880;
}

static inline int Config_RandomGodTargetCost() {
	return 2000;
}

static inline int Config_MaxInspireCount() {
	return 10;
}

static inline float Config_GodAward(int count) {
	if (count <= 10)
		return 3;
	else if (count <= 20)
		return 1;
	else if (count <= 30)
		return 0.5f;
	else
		return 0;
}

static inline int Config_ResetGodCost() {
	return 10;
}
static inline int Config_ClearGodCDCost() {
	return 10;
}

static inline int Config_MaxGodCount() {
	return 10;
}

static inline int Config_HellMap() {
	return 197;
}
static inline int Config_MaxWaitHellTime() {
	return 1000 * 2;
}
static inline int Config_MaxHellNPCs() {
	return 4;
}
static inline int Config_HellReviveTime() {
	return 1000 * 8;
}
static inline int Config_MaxHellNPCDie() {
	return 2;
}

static inline int Config_HellHonorBase() {
	return 10;
}

static inline int Config_HellScoreBase() {
	return 5;
}

static inline int Config_HellBegin() {
	return 20;
}

static inline int Config_HellTime() {
	return 1000 * 3600;
}

static inline bool Config_IsHellOpen(time_t cur) {
	int end = Config_HellBegin() + Config_HellTime() / (1000 * 3600);
	if (end >= 24) {
		end -= 24;
	}
	return Time_WithinHour(cur, Config_HellBegin(), Config_HellBegin() + Config_HellTime() / (1000 * 3600));
}

static inline int Config_HellMaxKill() {
	return 50;
}

static inline int Config_GodScore(int32_t power) {
	return power / 100;
}

static inline int Config_StampRMBCost(int64_t value) {
	if (value <= 0)
		return 0;
	if (value % 200 == 0)
		value--;
	return 1 + (int)(value / 200);
}

static inline int Config_GenGemCount() {
	return 3;
}
static inline int Config_GenGemCost(int gemLevel) {
	int v = 1;
	for (int i = 1; i < gemLevel - 1; i++)
		v *= 2;
	return 2000 * v;
}

static inline void Config_HoleStone(int index, int64_t *stone, int *count) {
	if (index >= 0 && index <= 1) {
		*stone = -1;
		*count = 0;
	} else if (index == 2) {
		*stone = 500;
		*count = 1;
	} else if (index == 3) {
		*stone = 500;
		*count = 2;
	} else if (index == 4) {
		*stone = 501;
		*count = 1;
	} else {
		*stone = -1;
		*count = 0;
	}
}

static inline void Config_UnmountStone(int level, int64_t *stone, int *count) {
	*stone = 502;
	if (level >= 1 && level <= 4) {
		*count = 0;
	} else if (level >= 5 && level <= 6) {
		*count = 1;
	} else if (level == 7) {
		*count = 2;
	} else if (level == 8) {
		*count = 5;
	} else if (level == 9) {
		*count = 10;
	} else if (level == 10) {
		*count = 50;
	} else {
		*stone = -1;
		*count = 0;
	}
}

static inline int Config_UnmountCost(int gemLevel) {
	int v = 1;
	for (int i = 1; i < gemLevel; i++)
		v *= 2;
	return 10000 * v;
}

static inline int Config_RandomGemType() {
	return Time_Random((int)FightAtt::ATK, (int)FightAtt::DODGE + 1);
}

template<typename T>
static void Config_AdjustGemHole(T *asset) {
	int count = 0;
	if (asset->strongLevel() >= 15)
		count = 5;
	else if (asset->strongLevel() >= 10)
		count = 4;
	else if (asset->strongLevel() >= 5)
		count = 3;
	else if (asset->strongLevel() >= 3)
		count = 2;
	else if (asset->strongLevel() >= 0)
		count = 1;
	for (int i = 0; i < count; i++) {
		if (asset->gemModel(i) == -2)
			asset->set_gemModel(i, -1);
	}
}

static inline int64_t Config_EnhanceStone() {
	return 503;
}

static inline int Config_EnhanceDelta(PB_FightAtt::PropertyType type) {
	if (type == PB_FightAtt::MAXHP) {
		return Time_Random(-80, 101);
	} else if (type == PB_FightAtt::DODGE) {
		return Time_Random(-10, 11);
	} else {
		return Time_Random(-8, 11);
	}
}

static inline int64_t Config_StrongStone() {
	return 499;
}

static inline int64_t Config_StrongProtect() {
	return 504;
}

static inline void Config_InheritStone(int64_t *stone, const EquipmentInfo *equip, const EquipAsset *asset, int *count) {
	*stone = 505;
	float v = asset->strongLevel();
	for (int i = 0; i < EquipAsset::ENHANCEDELTA_SIZE; i++) {
		if (asset->enhanceDelta(i) != 0) {
			if (equip->enhanceType(i) == (PB_FightAtt::PropertyType)FightAtt::MAXHP)
				v += asset->enhanceDelta(i) / 1000;
			else
				v += asset->enhanceDelta(i) / 100;
		}
	}
	*count = (int)v;
	if (*count <= 0)
		*count = 1;
}

static inline int Config_MaxBaseWingDegree() {
	return 21;
}

static inline int Config_ForeverWingPrice(int base) {
	return base * 10;
}

static inline int Config_WingDays() {
	return 7;
}

static inline int Config_WorldBossMap() {
	return 196;
}

static inline int64_t Config_WorldBossTime() {
	return 1000 * 3600;
}

static inline int Config_WorldBossBegin(int begin[]) {
	int n = 0;
	begin[n++] = 12;
	begin[n++] = 19;
	n += Event_ExtraWorldBoss(Time_TimeStamp(), &begin[n]);
	return n;
}

static inline bool Config_IsWorldBossOpen(time_t cur) {
	int begin[24];
	int n = Config_WorldBossBegin(begin);
	for (int i = 0; i < n; i++) {
		int end = begin[i] + Config_WorldBossTime() / (1000 * 3600);
		if (end >= 24) {
			end -= 24;
		}
		if (Time_WithinHour(cur, begin[i], begin[i] + Config_WorldBossTime() / (1000 * 3600)))
			return true;
	}
	return false;
}

static inline int Config_MaxRestrictionNum() {
	return 300;
}

static inline int Config_MakeHoleNeedItemNum(int index) {
	int num[4] = {1, 1, 1, 2};
	if (index > 3 || index < 0)
		return -1;
	return num[index];
}

static inline int64_t Config_MakeHoleNeedItemID(int index) {
	int id[5] = {500, 500, 500, 500, 501};
	if (index > 4 || index < 0)
		return -1;
	return id[index];
}

static inline int Config_UnInlayNeedItemNum(int lvl) {
	int num[3] = {1, 2, 5};
	if (lvl > 2 || lvl < 0)
		return -1;
	return num[lvl];
}

static inline int64_t Config_UnInlayNeedItemID() {
	return 502;
}

static inline int Config_ForgetProbably(int num) {
	switch(num) {
		case 0:
		case 1:
		case 2:
			return 0;
		case 3:
			return 10;
		case 4:
			return 20;
		case 5:
			return 30;
		case 6:
			return 40;
		case 7:
			return 50;
		case 8:
			return 60;
		case 9:
			return 70;
		case 10:
			return 80;
		case 11:
			return 90;
		case 12:
			return 100;
		default:
			return -1;
	}
}

static inline int Config_PetLevelUpProbably(int rand) {
	if (rand >= 0 && rand < 10) {
		return 3;
	} else if (rand >= 10 && rand ) {
		return 2;
	} else {
		return 1;
	}
} 

static inline int Config_PetInheritExpTransform(int level) {
	if (level <= 0) {
		return 0;
	}
	return (int)(level * 0.8f);
}

static inline int Config_MoneyPetLevelUp(int level) {
	return level * 500 * (1 + level / 5);
}

static inline int Config_PetInheritSkillsTransform(int skillNum) {
	if (skillNum < 0) {
		return 0;
	}
	return skillNum * skillNum * 100;
}

static inline int Config_TowerRecord(int maxTower) {
	return maxTower / 5 * 5;
}

static inline int Config_SendFriendsLove() {
	return 100;
}

static inline int Config_HarvestFriendsLove() {
	return 30;
}

static inline int Config_WorldBossReviveCost() {
	return 20;
}

static inline int Config_SkillPower(int level) {
	return level * 20 + (level / 11) * (level / 11) * 200;
}

void Config_SetMulExp(int mul);
int Config_MulExp();

static inline int Config_TotalComeCount(int level) {
	static int v[] = {1, 2, 3, 5, 7, 10, 15, 20, 30};
	static int size = (int)(sizeof(v) / sizeof(v[0]));
	if (level < 0 || level >= size)
		return INT_MAX;
	return v[level];
}

static inline bool Config_BuyDurability(int index, int *rmb, int *v) {
	if (index == 0) {
		*rmb = 0;
		*v = 30;
	} else if (index == 1) {
		*rmb = 50;
		*v = 50;
	} else if (index == 2) {
		*rmb = 100;
		*v = 70;
	} else if (index == 3) {
		*rmb = 300;
		*v = 150;
	} else {
		return false;
	}
	return true;
}

static inline int Config_LoginObtRMB() {
	return 15;
}

static inline int Config_ActiveObtItem() {
	return 1;
}

const std::multimap<int, int64_t>* Config_fixedEventMap();
bool Config_InActiveTime(int id);
bool Config_SameActiveInterval(int id, int preTime, int endTime);
void Config_ActiveTick(void *arg);

static inline int Config_GetRMBBegin1() {
	return 11;
}
static inline int Config_GetRMBBegin2() {
	return 19;
}
static inline int Config_GetRMBTime() {
	return 1000 * 3600 * 3;
}

static inline int Config_GetDurabilityBegin1() {
	return 11;
}
static inline int Config_GetDurabilityBegin2() {
	return 19;
}
static inline int Config_GetDurabilityTime() {
	return 1000 * 3600 * 3;
}

static inline int Config_PlayOffMap() {
	return 193;
}
static inline int Config_PlayOffTime() {
	return 1000 * 60 * 3;
}
static inline int Config_PlayOffInterval() {
	return 1000 * 60 * 2;
}
static inline int Config_PlayOffWait() {
	return 1000 * 60 * 1;
}
static inline int Config_PlayOffTick() {
	return 1000 * 10;
}

static inline int Config_TransformCount(int vip) {
	int ret = 2;
	if (vip > 0) {
		const VIPInfo *info = VIPInfoManager_VIPInfo(vip);
		//assert(info != NULL);
		if(info != NULL)
			ret += info->transformCount();
	}
	return ret;
}

static inline int Config_CreateFaction() {
	return 1000;
}

static inline void Config_FactionDoante(int type, int* expend, int* dayEvent, int* factionExp, int* exp, int* item) {
	if (type == 1) {
		*expend = 100000;
		*factionExp = 10;
		*exp = 10;
		*item = 10;
		*dayEvent = 10;
	} else if (type == 2) {
		*expend = 20;
		*factionExp = 30;
		*exp = 30;
		*item = 30;
		*dayEvent = 10;
	} else if (type == 3) {
		*expend = 200;
		*factionExp = 300;
		*exp = 300;
		*item = 300;
		*dayEvent = 10;
	}	
}

static inline int Config_AwardRange(int64_t rank) {
	if (rank == 0)
		return 0;
	else if (rank == 1)
		return 1;
	else if (rank == 2)
		return 2;
	else if (rank < 10)
		return 3;
	else if (rank < 20)
		return 4;
	else if (rank < 50)
		return 5;
	else if (rank < 100)
		return 6;
	else if (rank < 200)
		return 7;
	else if (rank < 500)
		return 8;
	else if (rank < 1000)
		return 9;
	else if (rank < 2000)
		return 10;
	else if (rank < 4000)
		return 11;
	else
		return 12;
}

static inline int Config_AwardRange(int64_t rank, int64_t rankSize) {
	if (rank == 0)
		return 0;
	else if (rank == 1)
		return 1;
	else if (rank == 2)
		return 2;
	else if (rank >= 3 && rank <= 5)
		return 3;
	else if (rank >= 6 && rank <= 10)
		return 4;
	else if (rank >= 10 && rank <= (rankSize / 10))
		return 5;
	else if (rank > (rankSize / 10) && rank <= (rankSize / 5))
		return 6;
	else if (rank > (rankSize / 5) && rank <= (rankSize / 2))
		return 7;
	else if (rank > (rankSize / 2) && rank <= rankSize)
		return 8;
	else
		return 9;
}

static inline void Config_GodRankAward(int rank, int& money, int& godScore) {
	if (rank == 0) {
		money = 1000;
		godScore = 1000;
	} else if (rank == 1) {
		money = 800;
		godScore = 800;
	} else if (rank == 2) {
		money = 500;
		godScore = 500;
	} else {
		money = 50;
		godScore = 50;
	}
}

static inline int Config_GodRankSize() {
	return 10;
}

static inline bool Config_IsApplePhone(const char *phoneType) {
	static const char *tokens[] = {"iPhone", "iPad", "iPod"};
	for (size_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
		if(strstr(phoneType, tokens[i]) != NULL)
			return true;
	}
	return false;
}

static inline int Config_EventMap1_Easy() {
	return 501;
}
static inline int Config_EventMap1_Normal() {
	return 502;
}
static inline int Config_EventMap1_Hard() {
	return 503;
}

//static inline int Config_MaxEvent1Count() {
//	return 3;
//}

static inline int Config_EventMap_ProtectCrystal_Easy() {
	return 500;
}
static inline int Config_EventMap_ProtectCrystal_Normal() {
	return 507;
}
static inline int Config_EventMap_ProtectCrystal_Hard() {
	return 508;
}
//static inline int Config_MaxCount_EventMap_ProtectCrystal() {
//	return 3;
//}

static inline int Config_EventMap_GoldPig_Easy() {
	return 505;
}
static inline int Config_EventMap_GoldPig_Normal() {
	return 509;
}
static inline int Config_EventMap_GoldPig_Hard() {
	return 510;
}
//static inline int Config_MaxCount_EventMap_GoldPig() {
//	return 3;
//}

static inline int Config_EventMap_RobRes_Easy() {
	return 516;
}
static inline int Config_EventMap_RobRes_Normal() {
	return 517;
}
static inline int Config_EventMap_RobRes_Hard() {
	return 518;
}
//static inline int Config_MaxCount_EventMap_RobRes() {
//	return 3;
//}

static inline int Config_EventMap_PetStone_Easy() {
	return 519;
}
static inline int Config_EventMap_PetStone_Normal() {
	return 520;
}
static inline int Config_EventMap_PetStone_Hard() {
	return 521;
}
//static inline int Config_MaxCount_EventMap_PetStone() {
//	return 3;
//}

static inline int Config_EventMap_ObstacleFan_Easy() {
	return 513;
}
static inline int Config_EventMap_ObstacleFan_Normal() {
	return 514;
}
static inline int Config_EventMap_ObstacleFan_Hard() {
	return 515;
}
//static inline int Config_MaxCount_EventMap_ObstacleFan() {
//	return 3;
//}

static inline int Config_EventMap_PetBattle_Easy() {
	return 506;
}
static inline int Config_EventMap_PetBattle_Normal() {
	return 511;
}
static inline int Config_EventMap_PetBattle_Hard() {
	return 512;
}
//static inline int Config_MaxCount_EventMap_PetBattle() {
//	return 3;
//}

static inline int64_t Config_SpecialInterval() {
	return 1000 * 20;
}

static inline float Config_AITransformHP() {
	return 0.5f;
}
static inline int Config_AITransformInterval() {
	return 1000 * 30;
}
static inline int Config_Treasure(int index, int count) {
	if (index == 1) {
		return 288 * count;
	}else if (index == 2) {
		return 288 * count;
	}else if (index == -1) {
		return Time_TimeStamp() + 24 * 3600;
	}else if (index == -2) {
		return Time_TimeStamp() + 24 * 3600;
	}
	return -1;
}

static inline void Config_FactionBossAward(int *exp, int *item, int *contrib) {
	*exp = 10000;
	*item = 10000;
	*contrib = 200;
}
static inline int Config_FactionBossMap() {
	return 550;
}
static inline int Config_FactionBossTime() {
	return 1000 * 3600;
}
static inline int Config_FactionBossBegin() {
	return 18;
}
static inline bool Config_IsFactionBossOpen(time_t cur) {
	int end = Config_FactionBossBegin() + Config_FactionBossTime() / (1000 * 3600);
	if (end >= 24) {
		end -= 24;
	}
	return Time_WithinHour(cur, Config_FactionBossBegin(), Config_FactionBossBegin() + Config_FactionBossTime() / (1000 * 3600));
}
static inline int Config_MoneyTree(int count, int& cost) {
	if (count >= 0) {
		cost = 5 * count + 10;
	}
	return 50000;
}

int Config_MulTowerGroup(int room);
// -1: not
// 0: begin < map && map < end
// 1: begin
// 2: end
int Config_MulTowerType(int room);
static inline int Config_MaxMulTowerCount() {
	return 2;
}

static inline int Config_MaxTeamPlayers(int map) {
	if (Config_MulTowerType(map) >= 0)
		return MAX_MUL_TOWER_TEAM_PLAYERS;
	else
		return MAX_NORMAL_TEAM_PLAYERS;
}
static inline int Config_FlyPlay(int index, int& id) {
	if (index == 0) {
		id = 506;
		return 50;
	}else if(index == 1){
		id = 507;
		return 1000;
	}
	return -1;
}
static inline int Config_ClearQuickFightCDCost() {
	return 0;
}
static inline int Config_QuickFightCD() {
	return 60;
}

// 1: B
// 2: A
// 3: S
static inline int Config_RoomFinalLevel(int score) {
	if (score < 1400)
		return 1;
	else if (score <= 2000)
		return 2;	
	else
		return 3;
}

static inline int Config_Blessing() {
	return 20;
}
static inline int Config_TopUpObtRMB(int rmb, int& bit) {
	static int modle[8] = {1000, 200, 2000, 300, 5000, 1000, 10000, 1500};
	int count = 0;
	for (int i = 0; i < 8; i = i + 2) {
		if (rmb >= modle[i]) {
			if ((bit & (1 << (16 + i / 2))) == 0) {
				count += modle[i + 1];
				bit |= 1 << (16 + i / 2);
			}
		}
	}
	return count;
}

static inline int Config_QueryRechargeInterval() {
	return 2;
}
static inline int Config_QueryRechargeCount() {
	return 10;
}

const char * Config_ChannelName(int num);
int Config_ChannelNum(const char *channel);
int Config_ChannelRechargeNum(const char *channel);
int Config_ChannelRMBBase(const char *channel);
const char * Config_ChannelBIName(const char *channel);
const char * Config_ChannelPlatform(const char *channel);
const char * Config_ChannelURL(const char *channel);

static inline int Config_MaxPetDistance() {
	return 30;
}

static inline int Config_FactionWarMap() {
	return 551;
}
static inline int Config_FactionWarBegin() {
	return 20;
}
static inline int Config_FactionWarTime() {
	return 1000 * 3600;
}
static inline int Config_FactionWarDay(int day[]) {
	int n = 0;
	day[n++] = 1;
	day[n++] = 3;
	day[n++] = 5;
	day[n++] = 0;

	day[n++] = 2;
	day[n++] = 4;
	day[n++] = 6;
	return n;
}
static inline bool Config_IsFactionWarOpen(time_t cur) {
	tm t;
	Time_LocalTime(cur, &t);
	int day[7];
	int n = Config_FactionWarDay(day);
	bool in = false;
	for (int i = 0; i < n; i++) {
		if (day[i] == t.tm_wday) {
			in = true;
			break;
		}
	}
	if (!in)
		return false;
	int begin = Config_FactionWarBegin();
	int end = begin + Config_FactionWarTime() / (1000 * 3600);
	if (end >= 24) {
		end -= 24;
	}
	return Time_WithinHour(cur, begin, end);
}
static inline int Config_MaxCountInFactionInFactionWar() {
	return 5;
}
static inline int Config_FactionWarAward(int kill, int time) {
	return kill * 5 + time / 60 * 5;
}
static inline int Config_RandomPetHalo() {
	return 10;
}
static inline int Config_RandomPetHaloInfo(int value) {
	if (value < 1)
		return 10;
	if (value < 4)
		return 4;
	if (value < 15)
		return 2;
	return 1;
}

int Config_MaxOnlinePlayers();
static inline bool Config_DoubleRecharge(int64_t delta) {
	return delta == 60 || delta == 180 || delta == 300 || delta == 980 || delta == 1980 || delta == 3280 || delta == 6480; 
}
static inline int Config_DoubleRechargeLevel(int64_t delta) {
	if(delta == 60) {
		return 3;
	} else if(delta == 180) {
		return 4;
	} else if(delta == 300) {
		return 5;
	} else if(delta == 980) {
		return 6;
	} else if(delta == 1980) {
		return 7;
	} else if(delta == 3280) {
		return 8;
	} else if(delta == 6480) {
		return 9;
	} else {
		return 9;
	}
}

static inline int Config_MonthCardCost() {
	return 300;
}
static inline int Config_MonthCardAward() {
	return 120;
}
static inline int Config_MonthCardFirstAward() {
	return 300;
}

static inline int Config_LuckCost() {
	return 20;
}

static inline int Config_LuckFreeCount() {
	return 20;
}

static inline int Config_LuckCDTime() {
	return 10 * 60;
}
static inline int Config_SendRekooRoleRMB() {
	return 0;
}
static inline int Config_InviteCodeLevel() {
	return 30;
}
static inline int Config_GrabRedEnvelope() {
	return 481;
}

static inline bool Config_CanGenGem(int level) {
	static float rate[] = {1.0f, 1.0f, 0.3f, 0.2f, 0.1f, 0.1f, 0.05f, 0.02f, 0.01f};
	if (level < 2 || level > (int)(sizeof(rate) / sizeof(rate[0])) + 1)
		return false;
	return Time_CanPass(rate[level - 2]);
}

static inline int Config_CatGift(int count) {
	if (count == 1)
		return 5;
	if (count == 10)
		return 45;

	return 0;
}

static inline int Config_LevelUp(int level, int exp, int cur) {
	//神格升级需要的经验=等级*（1+等级/10）*EXP
	float levelup = (float)level;
	float expadd = 0.0f;
	for (levelup = level; ; levelup++) {
		expadd += levelup * (1 + levelup / 10) * exp;
		if (cur < expadd)
			break;
	}
	return levelup;
}

static inline int Config_GodShipCost(int i) {
	if (i == 0)
		return 10000;
	else if (i == 1)
		return 20000;
	else if (i == 2)
		return 40000;
	else if (i == 3)
		return 80000;
	else if (i == 4)
		return 160000;
	else
		return 0;
}

static inline int Config_GodShipRandom(int i) {
	if (i == 0)
		return 40;
	else if (i == 1)
		return 20;
	else if (i == 2)
		return 10;
	else if (i == 3)
		return 5;
	else if (i == 4)
		return 0;
	else
		return 0;
}
static inline bool Config_Reservation(int64_t roleID, int rmb) {
	if (roleID >= 0 || roleID == -1 || roleID == -2 || roleID == -3) {
		if (rmb == 100 || rmb == 1000 || rmb == 10000) {
			return true;
		}
		return false;
	}else {
		if (roleID == -4) {
			return true;
		}
	}
	return false;
}

static inline int Config_ReservationCost(int rmb, bool flag) {
	if (flag) {
		return rmb + 100;
	}else {
		return rmb;
	}
}

static inline void Config_Reservation(int time, int* year, int* month, int* day, int* hour, int* min, int*sec) {
	time_t t = (time_t)time;
	tm cur;
	Time_LocalTime(t, &cur);

	if (year != NULL) {
		*year = cur.tm_year + 1900;
	}
	if (month != NULL) {
		*month = cur.tm_mon + 1;
	}
	if (day != NULL) {
		*day = cur.tm_mday;
	}
	if (hour != NULL) {
		*hour = cur.tm_hour; 
	}
	if (min != NULL) {
		*min = cur.tm_min;
	}
	if (sec != NULL) {
		*sec = cur.tm_sec;
	}
}

static inline int Config_ReservationTime(int hour, int min) {
	time_t t = (time_t)Time_TimeStamp();
	tm cur;
	Time_LocalTime(t, &cur);
	cur.tm_hour = hour;
	cur.tm_min = min;
	cur.tm_sec = 0;
	return mktime(&cur);
}

static inline int32_t Config_PKLevel(int64_t PKScore) {
	if (PKScore >= 0 && PKScore <= 1100)
		return 1;
	else if (PKScore >= 1101 && PKScore <= 1300)
		return 2;
	else if (PKScore >= 1301 && PKScore <= 1700)
		return 3;
	else if (PKScore >= 1701 && PKScore <= 2000)
		return 4;
	else if (PKScore >= 2001 && PKScore <= 2300)
		return 5;
	else if (PKScore >= 2301 && PKScore <= 2600)
		return 6;
	else if (PKScore >= 2601 && PKScore <= 3000)
		return 7;
	else if (PKScore >= 3001 && PKScore <= 3500)
		return 8;
	else if (PKScore >= 3501 && PKScore <= 4000)
		return 9;
	else if (PKScore >= 4001 && PKScore <= 99999)
		return 10;
	return -1;
}

static inline int64_t Config_PKScore(bool win, int32_t diffLevel) {
	if (win) {
		if (diffLevel < 0) {
			return 50 * (-diffLevel);
		} else if (diffLevel == 0) {
			return 30;
		} else {
			return 20 * diffLevel;
		}
	} else {
		if (diffLevel < 0) {
			return (-15) * (-diffLevel);
		} else if (diffLevel == 0) {
			return -25;
		} else {
			return (-40) * diffLevel;
		}
	}
}

std::string Config_RandomName(bool male);

#endif
