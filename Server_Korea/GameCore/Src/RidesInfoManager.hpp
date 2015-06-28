#ifndef _RIDES__INFO_MANAGER_HPP_
#define _RIDES__INFO_MANAGER_HPP_

#include "RidesInfo.hpp"
#include <sys/types.h>

void RidesInfoManager_Init();

const RidesInfo * RidesInfoManager_RidesInfo(int32_t id, int32_t star);

#endif
