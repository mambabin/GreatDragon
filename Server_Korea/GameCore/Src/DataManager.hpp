#ifndef _DATA_MANAGER_HPP_
#define _DATA_MANAGER_HPP_

#include "PlayerInfo.pb.h"
#include "PlayerAtt.hpp"
#include "EquipmentInfo.hpp"
#include "BaseInfo.pb.h"
#include "DCProto.pb.h"
#include <vector>
#include <sys/types.h>
#include <set>

struct RecordComp{
	bool operator()(const RecordInfo &lhs, const RecordInfo &rhs) {
		return lhs.arg1() < rhs.arg1();
	}
};

struct RecordComp_{
	bool operator()(const RecordInfo &lhs, const RecordInfo &rhs) {
		return lhs.role().roleID() < rhs.role().roleID();
	}
};

void DataManager_Init();

void DataManager_GenSnapshot();

void DataManager_RandomRoleID(int max, std::vector<int64_t> *ids);

bool DataManager_CollectRole(int64_t *cur, google::protobuf::RepeatedPtrField<RecordInfo> *singleRecord, google::protobuf::RepeatedPtrField<RecordInfo> *Restriction, RecordInfo* godInfoTime, RecordInfo* winFactionInfo, google::protobuf::RepeatedPtrField<RecordInfo>* factionInfo);
bool DataManager_HasAccount(const char *platform, const char *account);
bool DataManager_ActiveKey(const char *key);
// -1: error
// 0: success
// 1: exist
int DataManager_AddAccount(const PlayerInfo *info, const char *ip, const char *activateKey = NULL, char *addTime = NULL, char *deviceAddTime = NULL, bool beyond = false);
void DataManager_UpdateAccount(const PlayerInfo *info, const char *ip);
void DataManager_AddRole(const PB_PlayerAtt *data, const PlayerInfo *info);
int DataManager_RoleIDs(const char *platform, const char *account, int64_t *ids, size_t size);
bool DataManager_LoadRoleData(int64_t id, PB_PlayerAtt *data);
bool DataManager_LoadRoleData(const std::string &name, PB_PlayerAtt *data);
void DataManager_FilterRecharge(DCProto_FilterRecharge *proto);
bool DataManager_SaveRoleData(const PB_PlayerAtt *data, const PlayerInfo *info = NULL);
int64_t DataManager_DelRoleData(const char *platform, const char *account, int64_t id, const google::protobuf::RepeatedField<int64_t> *equipments);
void DataManager_SaveSingleRecord(int32_t map, const RecordInfo *record);
bool DataManager_HasName(const char *name);
int DataManager_AddMail(int64_t roleID, PB_MailInfo *mail);
int DataManager_AddMail(PB_PlayerAtt *att, PB_MailInfo *mail);
int DataManager_AddMail(std::set<int64_t>& roleIDs, const PB_MailInfo *mail);
// -1: key is not valid
// -2: has no key
// >= 0: gift id
int32_t DataManager_GetKeyGift(const char *key, int32_t *event, size_t size, bool done, int64_t roleID, int &groupIndex);

int32_t DataManager_RechargeValue(const DCProto_Recharge *recharge, int count = 0);
int DataManager_RechargeOver(const char *order, const PlayerInfo *info, int64_t roleID, int level);

void DataManager_CostRecord(const DCProto_CostRecord *cr);

// 0: success
// -1: account has logged in, process by gamecore
// -2: account is not valid
// -3: password is not match
int DataManager_Login(const char *platform, const char *account, const char *password, char *addTime = NULL, char *deviceAddTime = NULL);

void DataManager_SaveChat(const DCProto_SaveChat* info);

void DataManager_SaveRestrictionRecord(const char* str, int64_t num, int count);

void DateManager_GMSaveData(const DCProto_GMSaveData* gmSaveData);
bool DataMansger_GMLoadData(DCProto_GMLoadData* gmLoadData);
bool DataManager_GMLoadAtt(DCProto_GMPlayerQuery* gmQuery);

int DataManager_RegistDeviceServer(const char* str, bool flag, int time = 0);

int DataManager_FactionLoadData(DCProto_FactionLoadData * factionLoadData);
int DataManager_FactionSaveData(DCProto_FactionSaveData * factionSaveData);
int DataManager_FactionAddRecord(DCProto_FactionAddRecord * factionAddRecord);
int DataManager_FactionDelRecord(DCProto_FactionDelRecord * factionDelRecord);
int DataManager_FactionUpdateRecord(DCProto_FactionUpdateRecord * factionUpdateRecord);

int DataManager_LoadRankRecord(DCProto_InitRank* rank);
void DataManager_SysFactionMemInfo(DCProto_SysFactionMemInfo* info);

void DataManager_ModifyGodRank(const DCProto_ModifyGodRank* info);
int DataManager_SaveGodRankInfoRecord(DCProto_SaveGodRankInfoRecord* info);
int DataManager_GMChatRecords(DCProto_GMChatRecords* info);
int DataManager_GMRegistrCount(DCProto_GMRegistrCount* info);
int DataManager_GMLevelStatistics(DCProto_GMLevelStatistics* info);
int DataManager_GMRoleCount(DCProto_GMRoleCount* info);

int DataManager_QueryRoleFaction(DCProto_QueryRoleFaction* info);
int DataManager_FactionPower(DCProto_FactionPower* info);

int DataManager_LoadAllDataFromGMDataTable(DCProto_LoadAllDataFromGMDataTable * info);
void DataManager_SaveGMDataTable(DCProto_SaveGMDataTable* info);
void DataManager_GMAddExchange(DCProto_GMAddExchange * info);
void DataManager_GMAddRekooRole(DCProto_GMRekooRole * info);
int DataManager_LoadRekooRole(DCProto_LoadRekooRole * info);
int DataManager_LoadInviteCode(DCProto_LoadInviteCode * info);
void DataManager_QueryGMAccountFromSql(DCProto_QueryGMAccount * info);


#endif
