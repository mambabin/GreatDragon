#ifndef _FASHION_INFO_MANAGER_HPP_
#define _FASHION_INFO_MANAGER_HPP_

#include "Fashion.pb.h"
#include <sys/types.h>

void FashionInfoManager_Init();

const FashionInfo * FashionInfoManager_FashionInfo(int32_t id);

#endif
