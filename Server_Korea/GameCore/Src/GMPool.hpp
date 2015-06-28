#ifndef _GM_POOL_HPP_
#define _GM_POOL_HPP_
#include <string>
#include <sys/types.h>

struct GMMessage {
	int32_t id;
	std::string account;
//	bool flag;
};

void GMPool_Init();

void GMPool_Add(int32_t id, std::string account);
void GMPool_Del(int32_t id);

bool GMPool_Has(int32_t id);

int GMPool_GMs(int32_t *gms, size_t size);

bool GMPool_HasShowChat(int32_t id);

void GMPool_SetChatFlag(int32_t id, bool flag);

std::string GMPool_IDByAccountID(int32_t id);

int32_t GMPool_AccountIDByID(std::string account);

#endif
