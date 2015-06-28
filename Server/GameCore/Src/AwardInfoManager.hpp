#ifndef _AWARD_INFO_MANAGER_HPP_
#define _AWARD_INFO_MANAGER_HPP_

#include "Award.pb.h"
#include <sys/types.h>
#include <vector>

void AwardInfoManager_Init();

const AwardInfo * AwardInfoManager_AwardInfo(AwardInfo::Type type, int32_t index);
const AwardInfo * AwardInfoManager_AwardInfo(AwardInfo::Type type, int32_t level, int32_t* index);
const AwardInfo * AwardInfoManager_AwardInfo(AwardInfo::Type type, int32_t arg, int32_t index);
const std::vector<const AwardInfo *> *  AwardInfoManager_AllAwardInfo(AwardInfo::Type type, int32_t argBegin, int32_t argDelta);

#endif
