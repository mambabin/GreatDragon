#ifndef _VIP_INFO_MANAGER_HPP_
#define _VIP_INFO_MANAGER_HPP_

#include "VIPInfo.pb.h"
#include <sys/types.h>

void VIPInfoManager_Init();

const VIPInfo * VIPInfoManager_VIPInfo(int32_t id);

int32_t VIPInfoManager_MatchVIP(int64_t totalRMB);

#endif
