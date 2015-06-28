#ifndef _GOODS_INFO_MANAGER_HPP_
#define _GOODS_INFO_MANAGER_HPP_

#include "ItemInfo.hpp"
#include "ItemInfo.hpp"
#include <sys/types.h>

void GoodsInfoManager_Init();

const GoodsInfo * GoodsInfoManager_GoodsInfo(int32_t id);

bool GoodsInfoManager_IsGem(int32_t id);

int32_t GoodsInfoManager_RandomGem(int level);
int32_t GoodsInfoManager_Gem(int level, int type);
int GoodsInfoManager_MaxGemLevel();

const ExchangeTable * GoodsInfoManager_ExchangeTable(int index);

#endif
