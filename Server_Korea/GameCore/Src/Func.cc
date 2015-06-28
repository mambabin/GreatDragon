#include "Func.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "FuncInfo.hpp"
#include "IDGenerator.hpp"
#include <sys/types.h>
#include <iostream>
#include <vector>

using namespace std;

struct Func{
	int32_t id;
	FuncAtt *att;
	struct Component *component;
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct Func *> pool;
}package;


void Func_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
}

struct Func * Func_Create(FuncAtt *att, struct Component *component) {
	if (att == NULL || component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct Func *func = SearchBlock(package.pool, id);
	func->id = id;
	func->att = att;
	func->component = component;

	return func;
}

bool Func_IsValid(struct Func *func) {
	return func != NULL && IDGenerator_IsValid(package.idGenerator, func->id);
}

const FuncAtt * Func_Att(struct Func *func) {
	if (!Func_IsValid(func))
		return NULL;

	return func->att;
}

void Func_Destroy(struct Func *func) {
	if (!Func_IsValid(func))
		return;

	IDGenerator_Release(package.idGenerator, func->id);
}

struct Component * Func_Component(struct Func *func) {
	if (!Func_IsValid(func))
		return NULL;

	return func->component;
}

void Func_AddFunc(struct Func *func, FuncInfo::Type type, int32_t arg1, int32_t arg2) {
	if (!Func_IsValid(func))
		return;

	for (int i = 0; i < func->att->funcs_size(); i++) {
		FuncInfo *info = func->att->mutable_funcs(i);
		if (info->type() == FuncInfo::NONE) {
			info->set_type(type);
			info->set_arg(0, arg1);
			info->set_arg(1, arg2);
			break;
		}
	}
}
