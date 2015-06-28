#ifndef _PLAYER_POOL_HPP_
#define _PLAYER_POOL_HPP_

#include "PlayerEntity.hpp"
#include <sys/types.h>
#include <vector>
#include <map>
#include <string>

void PlayerPool_Init();
void PlayerPool_Prepare();
void PlayerPool_Finalize();

void PlayerPool_SetIdles(int64_t cur);
int64_t PlayerPool_GenRoleID();

void PlayerPool_FilterDesignation(struct PlayerEntity *entity);

// 0: Succeed.
// -1: Failure.
int PlayerPool_Add(struct PlayerEntity *playerEntity);

void PlayerPool_Del(struct PlayerEntity *playerEntity);

// line:
// -1: all lines
// >= 0: some line
int PlayerPool_Count(int32_t mapID, int line);
int PlayerPool_TotalCount();

// players in some line
// if player == NULL, means line 0
int PlayerPool_Players(int32_t mapID, struct PlayerEntity *player, std::vector<struct PlayerEntity *> *players);
// players in all lines
int PlayerPool_Players(int32_t mapID, std::vector<struct PlayerEntity *> *players);
int PlayerPool_Players(std::vector<struct PlayerEntity *> *players);

int PlayerPool_FreeFaction(int32_t map, struct PlayerEntity *entity);

void PlayerPool_Push(const char *content);

int PlayerPool_ServerDayNum(ItemInfo::Type type, int32_t id);
void PlayerPool_AddServerDayNum(ItemInfo::Type type, int32_t id, int delta);
void PlayerPool_SaveRestrictionRecord(int64_t index, int64_t num, bool flag);
void PlayerPool_SetRestrictionRecords(const google::protobuf::RepeatedPtrField<RecordInfo> *Records);

void PlayerPool_InitRank(DCProto_InitRank* rank);
int64_t PlayerPool_QueryRank(const int64_t roleID, NetProto_Rank* rank);

void PlayerPool_WorldBossRank(DCProto_InitRank* rank);

void PlayerPool_SaveGodRankRecord(int64_t roleID1, int64_t& roleID2, bool flag, int& score);
void PlayerPool_AddGodRank(int64_t roleID, const RecordInfo* info);
int PlayerPool_RandomGod(int64_t roleID, NetProto_GodRandomPlayer* info);
void PlayerPool_SelectGodRole(int64_t roleID1, int64_t roleID2);
void PlayerPool_QueryGodRank(int64_t roleID, NetProto_GodPanel* info);
int64_t PlayerPool_GodRankEnermyId(int64_t roleId);

void PlayerPool_SendGodAward(bool flag);
void PlayerPool_SendWeekGodAward(bool flag);

void PlayerPool_SendPKAward(bool flag);
void PlayerPool_SendWeekPKAward(bool flag);

void PlayerPool_SynRoleLevelAndPower(int64_t roleID, int64_t value);
void PlayerPool_AddRoleInfoToRank(const PB_PlayerAtt* att);

void PlayerPool_SynGodInfoTime(int32_t preTime, bool flag = false);

void PlayerPool_UpdateFreeRoles(const google::protobuf::RepeatedPtrField<PB_PlayerAtt> *atts);
// for player
void PlayerPool_RandomFreeRoles(int32_t map, int count, std::vector<int64_t> *exists, google::protobuf::RepeatedPtrField<PB_PlayerAtt> *atts);
// for npc
void PlayerPool_RandomFreeRoles(int count, std::vector<int64_t> *exists, std::vector<PlayerAtt> *atts);

int PlayerPool_RandomHire(int64_t roleID, int64_t* ids);

int PlayerPool_Blessing(struct PlayerEntity* player, NetProto_BlessCome* info);
void PlayerPool_SendActiveAward(NetProto_Rank::Type type, bool flag);
void PlayerPool_ServerOpenTime(NetProto_ServerOpenTime* info);
int PlayerPool_GMRankStatistics(NetProto_GMRankStatistics* info);
void PlayerPool_GMRoleOnlineCount();

const std::map<std::string, int64_t>* PlayerPool_GetFactionFightInfo();
int PlayerPool_CurFactionFight();
void PlayerPool_UpdateFactionFight(const char* str, int64_t delta);
int64_t PlayerPool_GetFactionFight(const char* str);
void PlayerPool_ClearFactionFightInfo();
void PlayerPool_SaveFactionFightInfo(const char* name = NULL, int save = 0, int64_t hurt = 0);
bool PlayerPool_GetWinFactionFight(int save, std::string *name,  int64_t *hurt);
void PlayerPool_SetWinFactionRecord(const RecordInfo *record);
void PlayerPool_SetFactionRecords(const google::protobuf::RepeatedPtrField<RecordInfo> *Records);

//void PlayerPool_UpdatePetRank(const char* str, int power);

int PlayerPool_LoginLaterTime();

void PlayerPool_InitDevilRank(DCProto_QueryRoleFaction* info);
void PlayerPool_SendDevilAward(bool send = false);

const AwardInfo* PlayerPool_GodAwardInfoByRoleID(int64_t roleID);
void PlayerPool_FactionPower(DCProto_FactionPower* info);

void PlayerPool_RankConsumeAdjust(int64_t roleID, int rmb);
void PlayerPool_AddRekooRole(int64_t roleID);
bool PlayerPool_IsInRekooRole(int64_t roleID);

int PlayerPool_AddInvateCode(int64_t roleID, NetProto_InvateCode * info);
void PlayerPool_SysInvateCodeInfo(int64_t roleID, const char * str);
void PlayerPool_InitInviteCode(DCProto_LoadInviteCode * info);
void PlayerPool_AddInviteCode(const char * str);
void PlayerPool_GMSetMaxRoleCountInfo(NetProto_GMLoginInfo * info);
bool PlayerPool_RoleFull(int32_t id);
void PlayerPool_GrabRedEnvelope(int64_t roleID, int count);
void PlayerPool_GrabRedEnvelope(int64_t roleID);
void PlayerPool_CatGift(int64_t roleID, NetProto_CatGift * info);
void PlayerPool_GroupPurchase(int64_t roleID, NetProto_GroupPurchase * info);
int PlayerPool_Reservation(struct PlayerEntity * entity, NetProto_Reservation * info);
void PlayerPool_ReservationRefresh(int index, int64_t roleID = -1);
bool PlayerPool_ReservationEnterOrPower(int64_t roleID, int op);
int PlayerPool_ReservationMap();
int PlayerPool_ReservationList(NetProto_ReservationList * info);
void PlayerPool_SaveReservationToDB();
void PlayerPool_ReservationQueryRole(DCProto_QueryRole * info);
void PlayerPool_ResetPvpScore(bool flag);

#endif
