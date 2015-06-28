#include "EquipmentInfoManager.hpp"
#include "EquipmentInfo.hpp"
#include "Debug.hpp"
#include "Random.hpp"
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
	AllEquipmentRandomColor allEquipRandomColor;
	AllEquipmentRandomPos allEquipRandomPos;
	AllEquipmentRandomLevel allEquipRandomLevel;
	AllEquipmentRandomEffect allEquipRandomEffect;
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
	{
		string src = Config_DataPath() + string("/EquipmentRandomColor.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.allEquipRandomColor.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		string src = Config_DataPath() + string("/EquipmentRandomPos.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.allEquipRandomPos.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		string src = Config_DataPath() + string("/EquipmentRandomLevel.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.allEquipRandomLevel.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	{
		string src = Config_DataPath() + string("/EquipmentRandomEffect.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.allEquipRandomEffect.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
	/* out put equip random config to check
	DEBUG_LOGERROR("EquipRandomColor");
	for(int i = 0; i < package.allEquipRandomColor.equipmentRandomColor_size(); i++)
	{
		EquipmentRandomColor* pEquipRandomColor = package.allEquipRandomColor.mutable_equipmentRandomColor(i);
		DEBUG_LOGERROR("%d	%d	%d	%d",
				pEquipRandomColor->color(),
				pEquipRandomColor->chance1(),
				pEquipRandomColor->chance2(),
				pEquipRandomColor->chance3());
	}
	DEBUG_LOGERROR("EquipRandomPos");
	for(int i = 0; i < package.allEquipRandomPos.equipmentRandomPos_size(); i++)
	{
		EquipmentRandomPos* pEquipRandomPos = package.allEquipRandomPos.mutable_equipmentRandomPos(i);
		DEBUG_LOGERROR("%d	%d	%d	%d	%d	%d	%d",
				pEquipRandomPos->pos(),
				pEquipRandomPos->atk(),
				pEquipRandomPos->def(),
				pEquipRandomPos->hp(),
				pEquipRandomPos->crit(),
				pEquipRandomPos->accuracy(),
				pEquipRandomPos->dodge()
				);
	}
	DEBUG_LOGERROR("EquipRandomLevel");
	for(int i = 0; i < package.allEquipRandomLevel.equipmentRandomLevel_size(); i++)
	{
		EquipmentRandomLevel* pEquipRandomLevel = package.allEquipRandomLevel.mutable_equipmentRandomLevel(i);
		DEBUG_LOGERROR("%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d",
				pEquipRandomLevel->level(),
				pEquipRandomLevel->atkMin(),
				pEquipRandomLevel->atkMax(),
				pEquipRandomLevel->defMin(),
				pEquipRandomLevel->defMax(),
				pEquipRandomLevel->hpMin(),
				pEquipRandomLevel->hpMax(),
				pEquipRandomLevel->critMin(),
				pEquipRandomLevel->critMax(),
				pEquipRandomLevel->accuracyMin(),
				pEquipRandomLevel->accuracyMax(),
				pEquipRandomLevel->dodgeMin(),
				pEquipRandomLevel->dodgeMax()
				);
	}
	DEBUG_LOGERROR("EquipRandomEffect");
	for(int i = 0; i < package.allEquipRandomEffect.equipmentRandomEffect_size(); i++)
	{
		EquipmentRandomEffect* pEquipRandomEffect = package.allEquipRandomEffect.mutable_equipmentRandomEffect(i);
		DEBUG_LOGERROR("%d	%d	%d",
				pEquipRandomEffect->id(),
				pEquipRandomEffect->effectId(),
				pEquipRandomEffect->chance()
				);
	}
	*/
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

int EquipmentInfoManager_GetRandomValue(int level, EquipmentInfo_Type type)
{
	int index = package.allEquipRandomLevel.equipmentRandomLevel_size() - 1;
	for(int i = 0; i < (package.allEquipRandomLevel.equipmentRandomLevel_size() - 1); i++)
	{
		EquipmentRandomLevel* pEquipRandomLevel1 = package.allEquipRandomLevel.mutable_equipmentRandomLevel(i);
		EquipmentRandomLevel* pEquipRandomLevel2 = package.allEquipRandomLevel.mutable_equipmentRandomLevel(i+1);
		if(level > pEquipRandomLevel1->level() && level <= pEquipRandomLevel2->level())
		{
			index = i;
			break;
		}
	}

	EquipmentRandomLevel* pEquipRandomLevel = package.allEquipRandomLevel.mutable_equipmentRandomLevel(index);
	int min = 0;
	int max = 0;
	switch(type)
	{
		case 0:
			{
				min = pEquipRandomLevel->atkMin();
				max = pEquipRandomLevel->atkMax();
				break;
			}
		case 1:
			{
				min = pEquipRandomLevel->defMin();
				max = pEquipRandomLevel->defMax();
				break;
			}
		case 2:
			{
				min = pEquipRandomLevel->hpMin();
				max = pEquipRandomLevel->hpMax();
				break;
			}
		case 3:
			{
				min = pEquipRandomLevel->critMin();
				max = pEquipRandomLevel->critMax();
				break;
			}
		case 4:
			{
				min = pEquipRandomLevel->accuracyMin();
				max = pEquipRandomLevel->accuracyMax();
				break;
			}
		default:
			{
				min = pEquipRandomLevel->dodgeMin();
				max = pEquipRandomLevel->dodgeMax();
				break;
			}
	}

	if(min >= max)
	{
		return min;
	}
	else
	{
		return min + random() % (max - min + 1);
	}
}
void EquipmentInfoManager_GetRadomData(int level,
		EquipmentInfo_ColorType color,
		EquipmentInfo_Type pos,
		int* randomType,
		int* randomData)
{
	EquipmentRandomColor* pEquipRandomColor = package.allEquipRandomColor.mutable_equipmentRandomColor(color);
	vector<int> colorChance;
	colorChance.push_back(pEquipRandomColor->chance1());
	colorChance.push_back(pEquipRandomColor->chance2());
	colorChance.push_back(pEquipRandomColor->chance3());
	int randomDataCount = Random_GetRandomIndex(colorChance) + 1;

	int randomType1 = -1;
	int randomType2 = -1;
	int randomType3 = -1;

	EquipmentRandomPos* pEquipRandomPos = package.allEquipRandomPos.mutable_equipmentRandomPos(pos);
	vector<int> posChance1;
	posChance1.push_back(pEquipRandomPos->atk());
	posChance1.push_back(pEquipRandomPos->def());
	posChance1.push_back(pEquipRandomPos->hp());
	posChance1.push_back(pEquipRandomPos->crit());
	posChance1.push_back(pEquipRandomPos->accuracy());
	posChance1.push_back(pEquipRandomPos->dodge());
	randomType1 = Random_GetRandomIndex(posChance1);
	*randomType = randomType1;
	*randomData = EquipmentInfoManager_GetRandomValue(level, (EquipmentInfo_Type)randomType1);

	if(randomDataCount == 2)
	{
		vector<int> posChance2;
		posChance2.push_back(pEquipRandomPos->atk());
		posChance2.push_back(pEquipRandomPos->def());
		posChance2.push_back(pEquipRandomPos->hp());
		posChance2.push_back(pEquipRandomPos->crit());
		posChance2.push_back(pEquipRandomPos->accuracy());
		posChance2.push_back(pEquipRandomPos->dodge());
		posChance2[randomType1] = (int)(posChance2[randomType1] / 100.0f * Config_EquipRandomPercent2());
		randomType2 = Random_GetRandomIndex(posChance2);
		*(randomType + 1) = randomType2;
		*(randomData + 1) = EquipmentInfoManager_GetRandomValue(level, (EquipmentInfo_Type)randomType2);
	}
	else
	{
		vector<int> posChance3;
		posChance3.push_back(pEquipRandomPos->atk());
		posChance3.push_back(pEquipRandomPos->def());
		posChance3.push_back(pEquipRandomPos->hp());
		posChance3.push_back(pEquipRandomPos->crit());
		posChance3.push_back(pEquipRandomPos->accuracy());
		posChance3.push_back(pEquipRandomPos->dodge());
		if(randomType1 == randomType2)
		{
			posChance3[randomType1] = (int)(posChance3[randomType1] / 100.0f * Config_EquipRandomPercent2() / 100.0f * Config_EquipRandomPercent3());
		}
		else
		{
			posChance3[randomType1] = (int)(posChance3[randomType1] / 100.0f * Config_EquipRandomPercent2());
			posChance3[randomType2] = (int)(posChance3[randomType2] / 100.0f * Config_EquipRandomPercent2());
		}
		randomType3 = Random_GetRandomIndex(posChance3);
		*(randomType + 2) = randomType3;
		*(randomData + 2) = EquipmentInfoManager_GetRandomValue(level, (EquipmentInfo_Type)randomType3);
	}
}

int EquipmentInfoManager_GetRadomEffect(int id)
{
	if(id == -1)
		return -1;

	vector<int> effectIds;
	vector<int> effectChances;
	for(int i = 0; i < package.allEquipRandomEffect.equipmentRandomEffect_size(); i++)
	{
		EquipmentRandomEffect* pEquipRandomEffect = package.allEquipRandomEffect.mutable_equipmentRandomEffect(i);
		if(pEquipRandomEffect->id() == id)
		{
			effectIds.push_back(pEquipRandomEffect->effectId());
			effectChances.push_back(pEquipRandomEffect->chance());
		}
	}
	if(effectIds.size() == 0)
		return -1;

	return effectIds[Random_GetRandomIndex(effectChances)];
}
