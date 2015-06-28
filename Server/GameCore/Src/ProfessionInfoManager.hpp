#ifndef _PROFESSION_INFO_MANAGER_HPP_
#define _PROFESSION_INFO_MANAGER_HPP_

#include "ProfessionInfo.hpp"
#include "PlayerAtt.hpp"

void ProfessionInfoManager_Init();
void ProfessionInfoManager_Prepare();

const PlayerAtt * ProfessionInfoManager_ProfessionInfo(ProfessionInfo::Type type, bool male);

#endif
