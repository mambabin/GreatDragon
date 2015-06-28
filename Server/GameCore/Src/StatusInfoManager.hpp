#ifndef _STATUS_INFO_MANAGER_HPP_
#define _STATUS_INFO_MANAGER_HPP_

#include "StatusInfo.pb.h"
#include <sys/types.h>

void StatusInfoManager_Init();

const StatusInfo * StatusInfoManager_StatusInfo(int32_t id);

#endif
