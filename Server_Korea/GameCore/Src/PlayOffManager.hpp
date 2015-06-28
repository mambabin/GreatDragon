#ifndef _PLAYOFF_MANAGER_HPP_
#define _PLAYOFF_MANAGER_HPP_

#include "PlayOff.pb.h"
#include <sys/types.h>

void PlayOffManager_Init();

const PlayOff * PlayOffManager_PlayOff(int32_t id);

int PlayOffManager_TotalPass(int begin, int over);
int PlayOffManager_Count(int over);
bool PlayOffManager_BeginAndEnd(int32_t id, int day, int pass, int *begin, int *end);

int PlayOffManager_CurData(int32_t id, int *day, int *pass, int *turn, int *overTime);

int PlayOffManager_NextIndex(int playoff, int day, int pass, bool win);
int PlayOffManager_CurIndex(int playoff, int day, int pass);

#endif
