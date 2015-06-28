#include "NPCInfoManager.hpp"
#include "RoleAtt.hpp"
#include "NPCAtt.hpp"
#include "Fight.hpp"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllNPCs npcs;
	vector< vector< vector<NPCAtt> > > pets;
	map<int, map<int, PB_PetHaloInfo> > petHaloInfo;
}pool;

void NPCInfoManager_Init() {
	{
		string src = Config_DataPath() + string("/NPCs.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		PB_AllNPCs all;
		if (!all.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		assert(all.npcs_size() <= pool.npcs.npcs_size());
		pool.npcs.FromPB(&all);
	}
	{
		string src = Config_DataPath() + string("/Pets.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		PB_AllPets all;
		if (!all.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < all.pets_size(); i++) {
			const PB_NPCAtt *unit = &all.pets(i);
			if (unit->id() == -1)
				continue;

			if (unit->id() >= (int)pool.pets.size())
				pool.pets.resize(unit->id() + 1);

			vector< vector<NPCAtt> > *family = &pool.pets[unit->id()];
			if (unit->quality() >= (int)family->size())
				family->resize(unit->quality() + 1);

			vector<NPCAtt> *quality = &(*family)[unit->quality()];
			if (unit->level() >= (int)quality->size()) {
				NPCAtt temp;
				temp.set_id(-1);
				quality->resize(unit->level() + 1, temp);
			}
			NPCAtt petAtt;
			petAtt.FromPB(unit);
			(*quality)[unit->level()] = petAtt;
		}
	}
	
	{
		pool.petHaloInfo.clear();
		
		string src = Config_DataPath() + string("/PetHalo.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}
		PB_AllPetHaloInfo allPetHaloInfo;
		if (!allPetHaloInfo.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < allPetHaloInfo.info_size(); ++i) {
			int group = allPetHaloInfo.info(i).propertieType();
			int level = allPetHaloInfo.info(i).level();
			PB_PetHaloInfo pet = allPetHaloInfo.info(i);
			map<int, PB_PetHaloInfo> mapInfo;
			mapInfo.clear();

			map<int, map<int, PB_PetHaloInfo> >::iterator itGroup = pool.petHaloInfo.find(group);
			if (itGroup != pool.petHaloInfo.end()) {
				mapInfo = itGroup->second;
			}
			mapInfo[level] = pet;
			pool.petHaloInfo[group] = mapInfo;
			// DEBUG_LOGERROR("%d, %d", group, level);
		}
	}
}

const NPCAtt * NPCInfoManager_NPC(int32_t id) {
	if (id < 0 || id >= pool.npcs.npcs_size())
		return NULL;

	const NPCAtt *att = &pool.npcs.npcs(id);
	return att->id() == -1 ? NULL : att;
}

const NPCAtt * NPCInfoManager_Pet(int32_t id, int32_t quality, int32_t level) {
	if (id < 0 || id >= (int)pool.pets.size())
		return NULL;

	vector< vector<NPCAtt> > *family = &pool.pets[id];
	if (quality < 0 || quality >= (int)family->size())
		return NULL;

	vector<NPCAtt> *quality_ = &(*family)[quality];
	if (level < 0 || level >= (int)quality_->size())
		return NULL;

	const NPCAtt *info = &(*quality_)[level];
	return info->id() == -1 ? NULL : info;
}

void NPCInfoManager_SetupNPCMission(int32_t id, int32_t mission) {
	if (id < 0 || id >= pool.npcs.npcs_size())
		return;

	NPCAtt *att = pool.npcs.mutable_npcs(id);
	for (int i = 0; i < att->funcAtt().funcs_size(); i++) {
		FuncInfo *func = att->mutable_funcAtt()->mutable_funcs(i);
		if (func->type() == FuncInfo::NONE) {
			func->set_type(FuncInfo::MISSION);
			func->set_arg(0, mission);
			break;
		}
	}
}

const std::map<int, map<int, PB_PetHaloInfo> >* NPCInfoManager_PetHaloInfo() {
	return &pool.petHaloInfo;
}

const PB_PetHaloInfo* NPCInfoManager_PetHaloInfo(int32_t quality, int32_t level) {
	map<int, map<int, PB_PetHaloInfo> >::iterator itGroup = pool.petHaloInfo.find(quality);
	if (itGroup == pool.petHaloInfo.end()) {
		return NULL;
	}

	map<int, PB_PetHaloInfo>::iterator itUnit = itGroup->second.find(level);
	if (itUnit == itGroup->second.end()) {
		return NULL;
	}
	return &(itUnit->second);
}


