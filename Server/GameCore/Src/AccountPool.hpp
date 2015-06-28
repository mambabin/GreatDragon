#ifndef _ACCOUNT_POOL_HPP_
#define _ACCOUNT_POOL_HPP_

#include "Config.hpp"
#include "DCProto.pb.h"
#include "PlayerInfo.pb.h"
#include <sys/types.h>
#include <string>

#define ACCOUNT_OFFLINE 0
#define ACCOUNT_ONLINE 1
#define ACCOUNT_DROP 2

void AccountPool_Init();

const PlayerInfo* AccountPool_IDToAccount(int32_t id);
// Pair of PlayerInfo.platform and PlayerInfo.account is key.
int32_t AccountPool_AccountToID(const PlayerInfo &account);
DCProto_LoadRoleData * AccountPool_AccountToRoleList(const PlayerInfo &account);

void AccountPool_AddAccountAndID(const PlayerInfo &account, int32_t id);
void AccountPool_AddRoleList(const PlayerInfo &account, const DCProto_LoadRoleData *proto);

int AccountPool_AccountStatus(const PlayerInfo &account);

void AccountPool_ReLogin(int32_t id);

void AccountPool_Logout(int32_t id);
void AccountPool_Drop(int32_t id);

#endif
