#ifndef _GMSET_HPP_
#define _GMSET_HPP_

#include "GmSet.pb.h"

void GmSet_Init();
void GmSet_Reload();

int GmSet_GetPercent(GmSet_Type setType, int arg1 = 0, int arg2 = 0);

#endif
