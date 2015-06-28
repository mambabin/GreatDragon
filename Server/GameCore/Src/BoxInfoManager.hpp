#ifndef _BOX_INFO_MANAGER_HPP_
#define _BOX_INFO_MANAGER_HPP_

#include "BoxInfo.pb.h"
#include <sys/types.h>

void BoxInfoManager_Init();

const BoxInfo * BoxInfoManager_BoxInfo(int32_t id);

#endif
