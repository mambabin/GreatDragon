#ifndef _GODSHIP_INFO_MANAGER_HPP_
#define _GODSHIP_INFO_MANAGER_HPP_

#include "GodShip.pb.h"
#include <sys/types.h>

void GodShipInfoManager_Init();

const GodShip * GodShipInfoManager_GodShipInfo(int32_t id);

#endif
