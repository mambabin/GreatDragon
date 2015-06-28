#ifndef _BUSINESS_INFO_MANAGER_HPP_
#define _BUSINESS_INFO_MANAGER_HPP_

#include "BusinessInfo.pb.h"
#include <sys/types.h>

void BusinessInfoManager_Init();
void BusinessInfoManager_Reload();

const BusinessInfo * BusinessInfoManager_BusinessInfo(int32_t id);

#endif
