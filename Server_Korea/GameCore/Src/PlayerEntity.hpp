#ifndef _PLAYER_ENTITY_HPP_
#define _PLAYER_ENTITY_HPP_

#include "Component.hpp"
#include "PlayerInfo.pb.h"
#include "PlayerAtt.hpp"
#include "SkillInfo.pb.h"
#include "Math.hpp"
#include "DCProto.pb.h"
#include "NetProto.pb.h"
#include "PetHalo.pb.h"
#include <sys/types.h>
#include <map>

#define MAX_STRONG_COUNT 10

struct StrongTable{
	int money;
	int goods;
	float strongSuccess;
	float showSuccess;
	float strongFail;
	float att;
	int protect;
};

struct PlayerEntity;

void PlayerEntity_Init();

struct PlayerEntity * PlayerEntity_Create(int32_t id, const PB_PlayerAtt *att);
bool PlayerEntity_IsValid(struct PlayerEntity *entity);
void PlayerEntity_Destroy(struct PlayerEntity *entity);
void PlayerEntity_Finalize(struct PlayerEntity *entity);

struct PlayerEntity * PlayerEntity_Player(int32_t id);
int32_t PlayerEntity_ID(struct PlayerEntity *entity);

struct PlayerEntity * PlayerEntity_PlayerByRoleID(int64_t roleID);
int64_t PlayerEntity_RoleID(struct PlayerEntity *entity);

const PlayerAtt * PlayerEntity_Att(struct PlayerEntity *entity);

void PlayerEntity_ResetDayEvent(struct PlayerEntity *entity, int dayDelta, int weekDelta);
void PlayerEntity_SetDayEvent(struct PlayerEntity *entity, int event, int32_t value, bool send = false);
void PlayerEntity_SetFixedEvent(struct PlayerEntity *entity, int event, int32_t value, bool send = false);

int PlayerEntity_Check(struct PlayerEntity *entity, int index, int64_t time, const char *res);
int PlayerEntity_CheckCombatRecord(struct PlayerEntity *entity, const CombatRecord& combatRecord);
int PlayerEntity_CheckCombatRecordDamage(struct PlayerEntity *entity, const CombatRecord& combatRecord, bool isPvp);

void PlayerEntity_CompleteGuide(struct PlayerEntity *entity, int id);
void PlayerEntity_CompleteGuide(PB_PlayerAtt *att, int id);

int PlayerEntity_AddDesignation(struct PlayerEntity *entity, int32_t id);
void PlayerEntity_DelDesignation(struct PlayerEntity *entity, int32_t id, bool notice = true);
int PlayerEntity_ShowDesignation(struct PlayerEntity *entity, int32_t id);
void PlayerEntity_UnshowDesignation(struct PlayerEntity *entity, int32_t id, bool notice = true);
bool PlayerEntity_HasDesignation(struct PlayerEntity *entity, int32_t id);

void PlayerEntity_ToSceneData(const PlayerAtt *src, PB_PlayerAtt *dest);
void PlayerEntity_ToSceneData(const PB_PlayerAtt *src, PB_PlayerAtt *dest);

void PlayerEntity_CopyAsNPC(const PlayerAtt *src, NPCAtt *dest, float power = 0.8f, bool randomName = true, int faction = 1 << 29, int reviveTime = -1);

// create role
void PlayerEntity_FilterData(PlayerAtt *att);
// load role
void PlayerEntity_FilterData(PB_PlayerAtt *att);

int PlayerEntity_AccountStatus(struct PlayerEntity *entity);
void PlayerEntity_ReLogin(struct PlayerEntity *entity);

struct Component * PlayerEntity_Component(struct PlayerEntity *entity);

void PlayerEntity_Save(struct PlayerEntity *entity, DCProto_SaveRoleData *proto);
void PlayerEntity_Logout(struct PlayerEntity *entity, bool notice = true);
void PlayerEntity_Drop(struct PlayerEntity *entity);

bool PlayerEntity_DoWorldChat(struct PlayerEntity *entity);
bool PlayerEntity_DoSecretChat(struct PlayerEntity *entity);

bool PlayerEntity_AddFriend(struct PlayerEntity *entity, int64_t roleID, const char *name, ProfessionInfo::Type professionType);
bool PlayerEntity_AddOutLineFriend(struct PlayerEntity *entity, int64_t roleID, const char *name, ProfessionInfo::Type professionType);
bool PlayerEntity_DelFriend(struct PlayerEntity *entity, int64_t roleID, bool flag);

int PlayerEntity_FriendsCount(struct PlayerEntity *entity);
int PlayerEntity_FansCount(struct PlayerEntity *entity);

int PlayerEntity_FindEmptyMail(PlayerAtt *att);
int PlayerEntity_FindEmptyMail(PB_PlayerAtt *att);
// Send mail to target, the equip will be reset.
// -1: error
// >= 0: index of mails
int PlayerEntity_AddMail(struct PlayerEntity *entity, PB_MailInfo *mail);
// Used by system, send equip or goods to me because the package is full.
int64_t PlayerEntity_AddMail(struct PlayerEntity *entity, ItemInfo::Type type, int64_t id, int count, const char *title, const char *content, int subrmb = 0, bool isRmb = false);
void PlayerEntity_DelMail(struct PlayerEntity *entity, int32_t id, bool delItem);
void PlayerEntity_DelFromMail(struct PlayerEntity *entity, int32_t select, int64_t id);
void PlayerEntity_ReadMail(struct PlayerEntity *entity, int32_t id);
int PlayerEntity_GetMailItem(struct PlayerEntity *entity, int32_t id);

const struct StrongTable * PlayerEntity_StrongTable(int level);
int PlayerEntity_MaxStrongLevel();

bool PlayerEntity_EquipNeedNotice(int32_t id);
bool PlayerEntity_GoodsNeedNotice(int32_t id);

// 0: success
// 1: nothing
// 2: failure
// -1: error
int PlayerEntity_Strong(struct PlayerEntity *entity, bool onBody, int32_t id, bool protect);
int PlayerEntity_ClearStrong(struct PlayerEntity *entity, bool onBody, int32_t id);

int PlayerEntity_Mount(struct PlayerEntity *entity, bool onBody, int32_t id, int32_t mountPos, int32_t gemPos);
int PlayerEntity_Unmount(struct PlayerEntity *entity, bool onBody, int32_t id, int32_t mountPos);

int PlayerEntity_EnhanceDelta(struct PlayerEntity *entity, bool onBody, int32_t id, bool ten, int32_t *delta, size_t size);
int PlayerEntity_Enhance(struct PlayerEntity *entity, bool onBody, int32_t id);

int PlayerEntity_Inherit(struct PlayerEntity *entity, bool parentBody, int32_t parentID, bool childBody, int32_t childID, bool useRMB, bool useStone);

void PlayerEntity_LoginToPlayerInfo(const NetProto_Login *login, PlayerInfo *info);

// -1: no right
// -2: now is champion, but has right of third
// 0: has right now
// 1: this pass is not over, but has right of next pass
// 2: this pass is not over, but lose right of next pass
int PlayerEntity_CanEnterPlayOff(struct PlayerEntity *entity, int playoff, int day, int pass);

void PlayerEntity_Update(struct PlayerEntity *entity);


//pet

int PlayerEntity_ObtainPet(struct PlayerEntity *entity, int32_t id);
bool PlayerEntity_SetPetFight(struct PlayerEntity *entity, int32_t index);
bool PlayerEntity_SetPetRest(struct PlayerEntity *entity, int32_t index, bool flag);
bool PlayerEntity_SetPetAttach(struct PlayerEntity *entity, int32_t index);
int PlayerEntity_SetPetLevelUp(struct PlayerEntity *entity, int32_t index);
// retuen value : the hight 16 bit is learn skill index, the low 16 bit is forget skill index
int PlayerEntity_PetLearnSkill(struct PlayerEntity *entity, int32_t index, int32_t skillID, bool flag = true);
bool PlayerEntity_PetInherit(struct PlayerEntity *entity, int32_t indexPre, int32_t indexAfter, bool flag);
bool PlayerEntity_PetForgetSkills(struct PlayerEntity *entity, int32_t index, int16_t bitSkills);
bool PlayerEntity_DelPet(struct PlayerEntity *entity, int32_t index);
int PlayerEntity_PetAdvance(struct PlayerEntity *entity, int32_t index);

bool PlayerEntity_FriendsLove(struct PlayerEntity *entity, int64_t roleID, bool flag);
void PlayerEntity_SynFriendsLoveFans(struct PlayerEntity *entity, int64_t roleID);
int PlayerEntity_LoginObtRMB(struct PlayerEntity *entity);

int PlayerEntity_ActiveGenRequest(struct PlayerEntity *entity);
bool PlayerEntity_ActiveDoubleGen(struct PlayerEntity *entity);
bool PlayerEntity_ActiveUpGradeGen(struct PlayerEntity *entity, int index);
bool PlayerEntity_ActiveGetGen(struct PlayerEntity *entity, bool flag);
bool PlayerEntity_ActiveFight(struct PlayerEntity *entity, bool flag = false);
int PlayerEntity_ActiveStrongeSolider(struct PlayerEntity *entity, bool flag = true);
void PlayerEntity_ActiveAddGoods(struct PlayerEntity *entity, int index);

void PlayerEntity_SysFaction(struct PlayerEntity *entity, const char* str);
void PlayerEntity_SysFactionMem(int64_t roleID, const char* str);

void PlayerEntity_SaveGodRankRecords(int64_t roleID1, int64_t roleID2, bool flag, int up);

void PlayerEntity_SaveOnlineGodRecord(const DCProto_SaveGodRankInfoRecord* info);
void PlayerEntity_GodRecordsRequest(struct PlayerEntity* entity, NetProto_GodRecords* info);

void PlayerEntity_GodPanel(struct PlayerEntity* entity, NetProto_GodPanel* info);
void PlayerEntity_HistoryRecordAward(int64_t roleID, int rank, int& godScore);

void PlayerEntity_ResetGodTime(struct PlayerEntity* entity);

int PlayerEntity_Treasure(struct PlayerEntity* entity, NetProto_Treasure* info);

int PlayerEntity_Hire(struct PlayerEntity* entity, NetProto_Hire* info);
void PlayerEntity_SetHire(struct PlayerEntity* entity, int index, int power);

int PlayerEntity_MoneyTree(struct PlayerEntity* entity, NetProto_MoneyTree *info);
int PlayerEntity_TopUpObtRMB(struct PlayerEntity* entity, NetProto_TopUpObtRMB* info);

int PlayerEntity_PetPsychicsLevelUp(struct PlayerEntity* entity, NetProto_PetPsychicsLevelUp* info);
const std::map<int, std::map<int, PB_PetHaloInfo> >* PlayerEntity_PetHaloInfo();

// index == -2  all pet 
//void PlayerEntity_PetPower(struct PlayerEntity* entity, int index);

int PlayerEntity_Luck(struct PlayerEntity* entity, NetProto_Luck* info);

const char * PlayerEntity_SelfInviteCode(int64_t roleID);
void PlayerEntity_OtherInviteCode(int64_t roleID, const char * str, int64_t roleID1);
void PlayerEntity_SysOnlineInvateCode(struct PlayerEntity * entity, const char * str);
void PlayerEntity_AddInviteCode(int64_t roleID);

int PlayerEntity_AddMsgSt(struct PlayerEntity *player, uint32_t group, uint32_t unit);

#endif
