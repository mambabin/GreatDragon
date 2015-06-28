#ifndef _DESIGNATION_INFO_MANAGER_HPP_
#define _DESIGNATION_INFO_MANAGER_HPP_

#include "DesignationInfo.pb.h"
#include <sys/types.h>

void DesignationInfoManager_Init();

const DesignationInfo * DesignationInfoManager_DesignationInfo(int32_t id);

#endif
