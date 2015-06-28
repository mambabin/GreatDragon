#include "AccountPool.hpp"
#include "PlayerEntity.hpp"
#include "Item.hpp"
#include "GCAgent.hpp"
#include "Debug.hpp"
#include "Config.hpp"
#include "PlayerInfo.pb.h"
#include "DCProto.pb.h"
#include <sys/types.h>
#include <string>
#include <map>

using namespace std;

struct Comp{
	bool operator()(const PlayerInfo &lhs, const PlayerInfo &rhs) {
		// 坑！！！
		// return lhs.account() < rhs.account() || lhs.platform() < rhs.platform();
		static char a[CONFIG_FIXEDARRAY];
		SNPRINTF1(a, "%s%s", lhs.platform().c_str(), lhs.account().c_str());
		static char b[CONFIG_FIXEDARRAY];
		SNPRINTF1(b, "%s%s", rhs.platform().c_str(), rhs.account().c_str());
		return strcmp(a, b) < 0;
	}
};

static struct{
	map<int32_t, PlayerInfo> idToAccount;
	map<PlayerInfo, int32_t, Comp> accountToID;
	map<PlayerInfo, DCProto_LoadRoleData, Comp> accountToRoleList;
	map<PlayerInfo, int, Comp> accountStatus;
}cache;

void AccountPool_Init() {
	cache.idToAccount.clear();
	cache.accountToID.clear();
	cache.accountToRoleList.clear();
	cache.accountStatus.clear();
}

const PlayerInfo* AccountPool_IDToAccount(int32_t id) {
	map<int32_t, PlayerInfo>::iterator it = cache.idToAccount.find(id);
	if (it == cache.idToAccount.end())
		return NULL;
	return &it->second;
}

int32_t AccountPool_AccountToID(const PlayerInfo &account) {
	map<PlayerInfo, int32_t>::iterator it = cache.accountToID.find(account);
	if (it == cache.accountToID.end())
		return -1;
	return it->second;
}

DCProto_LoadRoleData * AccountPool_AccountToRoleList(const PlayerInfo &account) {
	map<PlayerInfo, DCProto_LoadRoleData>::iterator it = cache.accountToRoleList.find(account);
	if (it == cache.accountToRoleList.end())
		return NULL;
	return &it->second;
}

void AccountPool_AddAccountAndID(const PlayerInfo &account, int32_t id) {
	cache.idToAccount[id] = account;
	cache.accountToID[account] = id;
	cache.accountStatus[account] = ACCOUNT_ONLINE;
}

void AccountPool_AddRoleList(const PlayerInfo &account, const DCProto_LoadRoleData *proto) {
	if (proto == NULL)
		return;
	cache.accountToRoleList[account] = *proto;
}

int AccountPool_AccountStatus(const PlayerInfo &account) {
	map<PlayerInfo, int>::iterator it = cache.accountStatus.find(account);
	if (it == cache.accountStatus.end())
		return ACCOUNT_OFFLINE;
	return it->second;
}

void AccountPool_ReLogin(int32_t id) {
	struct PlayerEntity *player = PlayerEntity_Player(id);
	if (player != NULL)
		PlayerEntity_ReLogin(player);

	const PlayerInfo *account = AccountPool_IDToAccount(id);
	if (account != NULL)
		cache.accountStatus[*account] = ACCOUNT_ONLINE;
}

void AccountPool_Logout(int32_t id) {
	GCAgent_SendOrderToNetAgent(ORDER_CLOSE_CLIENT, id, -1);

	struct PlayerEntity *player = PlayerEntity_Player(id);
	if (player != NULL)
		PlayerEntity_Logout(player);

	if (cache.idToAccount.find(id) == cache.idToAccount.end())
		return;

	PlayerInfo &account = cache.idToAccount[id];
	DEBUG_LOG("Account logout: %d %s %s", id, account.platform().c_str(), account.account().c_str());
	cache.accountToRoleList.erase(account);
	cache.accountToID.erase(account);
	cache.accountStatus.erase(account);
	cache.idToAccount.erase(id);
}

void AccountPool_Drop(int32_t id) {
	GCAgent_SendOrderToNetAgent(ORDER_CLOSE_CLIENT, id, -1);

	struct PlayerEntity *player = PlayerEntity_Player(id);
	if (player != NULL)
		PlayerEntity_Drop(player);

	const PlayerInfo *account = AccountPool_IDToAccount(id);
	if (account == NULL)
		return;
	if (AccountPool_AccountStatus(*account) != ACCOUNT_ONLINE)
		return;

	DEBUG_LOG("Account drop: %d %s %s", id, account->platform().c_str(), account->account().c_str());
	cache.accountStatus[*account] = ACCOUNT_DROP;
}
