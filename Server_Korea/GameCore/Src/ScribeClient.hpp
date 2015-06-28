#ifndef _SCRIBE_CLIENT_HPP_
#define _SCRIBE_CLIENT_HPP_

#include "PlayerInfo.pb.h"
#include <sys/types.h>
#include <cstddef>

void ScribeClient_Init();
void ScribeClient_Prepare();

const char * ScribeClient_Format(const PlayerInfo *info, int32_t id, const char *action, const char *v1 = NULL, const char *v2 = NULL, const char *v3 = NULL, const char *v4 = NULL);
void ScribeClient_SendMsg(const char *msg);

#endif
