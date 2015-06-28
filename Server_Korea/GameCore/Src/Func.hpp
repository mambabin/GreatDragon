#ifndef _FUNC_HPP_
#define _FUNC_HPP_

#include "Component.hpp"
#include "FuncInfo.hpp"
#include "MissionInfo.hpp"
#include <sys/types.h>

struct Func;

void Func_Init();

struct Func * Func_Create(FuncAtt *att, struct Component *component);
bool Func_IsValid(struct Func *func);
void Func_Destroy(struct Func *func);

const FuncAtt * Func_Att(struct Func *func);

struct Component * Func_Component(struct Func *func);

void Func_AddFunc(struct Func *func, FuncInfo::Type type, int32_t arg1, int32_t arg2);

#endif
