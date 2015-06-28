#ifndef _DROPTABLE_MANAGER_HPP_
#define _DROPTABLE_MANAGER_HPP_

#include "ItemInfo.hpp"
#include <sys/types.h>

void DropTableManager_Init();

const DropTable * DropTableManager_DropTable(int32_t id);

#endif
