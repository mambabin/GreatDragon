#include "EquipmentInfoManager.hpp"
#include "EquipmentInfo.hpp"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllEquipments pool;
	AllEquipRecipes recipes;
	AllBaseWings baseWings;
	AllWings wings;
}package;

void EquipmentInfoManager_Init() {
	{
		string src = Config_DataPath() + string("/Equipments.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.pool.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		string src = Config_DataPath() + string("/EquipmentRecipe.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.recipes.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		string src = Config_DataPath() + string("/BaseWing.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.baseWings.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < package.baseWings.wings_size(); i++) {
			const BaseWing *info = &package.baseWings.wings(i);
			assert(i == info->level() * Config_MaxBaseWingDegree() + info->degree());
		}
	}
	{
		string src = Config_DataPath() + string("/Wing.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.wings.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
}

const EquipmentInfo * EquipmentInfoManager_EquipmentInfo(int64_t id) {
	// MUST BE THREAD SAFETY
	if (id < 0 || id >= package.pool.equipments_size())
		return NULL;

	if (package.pool.equipments(id).id() == -1)
		return NULL;
	return &package.pool.equipments(id);
}

const EquipRecipe * EquipmentInfoManager_EquipRecipe(int32_t id) {
	if (id < 0 || id >= package.recipes.recipes_size())
		return NULL;

	if (package.recipes.recipes(id).id() == -1)
		return NULL;
	return &package.recipes.recipes(id);
}

const BaseWing * EquipmentInfoManager_BaseWing(int level, int degree) {
	int id = level * Config_MaxBaseWingDegree() + degree;
	if (id < 0 || id >= package.baseWings.wings_size())
		return NULL;

	if (package.baseWings.wings(id).level() == -1)
		return NULL;
	return &package.baseWings.wings(id);
}

const Wing * EquipmentInfoManager_Wing(int32_t id) {
	// MUST BE THREAD SAFETY
	if (id < 0 || id >= package.wings.wings_size())
		return NULL;

	if (package.wings.wings(id).id() == -1)
		return NULL;
	return &package.wings.wings(id);
}
