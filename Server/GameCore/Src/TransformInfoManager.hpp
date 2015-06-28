#ifndef _TRANSFORM_INFO_MANAGER_HPP_
#define _TRANSFORM_INFO_MANAGER_HPP_

#include "TransformInfo.pb.h"
#include <sys/types.h>

void TransformInfoManager_Init();

const TransformInfo * TransformInfoManager_TransformInfo(int32_t id, int quality, int level);

#endif
