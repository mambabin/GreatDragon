#ifndef _WEB_HPP_
#define _WEB_HPP_

#include "DCProto.pb.h"
#include <sys/types.h>

void Web_Init();

void Web_CheckUser(const DCProto_Login *login);
// -1: error
// 0: success
// 1: need to search db
int Web_CheckRecharge(const DCProto_Recharge *proto);
void Web_CheckCost(const DCProto_Cost *proto);

void Web_Update();

#endif
