#include "Fight.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "IDGenerator.hpp"
#include "Movement.hpp"
#include "SkillEntity.hpp"
#include "SkillInfoManager.hpp"
#include "PlayerEntity.hpp"
#include "Status.hpp"
#include "StatusInfoManager.hpp"
#include "NPCEntity.hpp"
#include "Mission.hpp"
#include "GCAgent.hpp"
#include "Time.hpp"
#include "MathUtil.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "Debug.hpp"
#include "EquipmentInfoManager.hpp"
#include "Item.hpp"
#include "GoodsInfoManager.hpp"
#include "ItemInfo.hpp"
#include "NetProto.pb.h"
#include "StatusInfo.pb.h"
#include "SkillInfo.pb.h"
#include "FightInfo.hpp"
#include "MovementInfo.hpp"
#include "DropTableManager.hpp"
#include "BloodInfoManager.hpp"
#include "BoxInfoManager.hpp"
#include "NPCInfoManager.hpp"
#include "NPCPool.hpp"
#include "ProfessionInfoManager.hpp"
#include "VIPInfoManager.hpp"
#include "AwardInfoManager.hpp"
#include "Event.hpp"
#include "ScribeClient.hpp"
#include "ConfigUtil.hpp"
#include "AccountPool.hpp"
#include "PlayOffManager.hpp"
#include "FactionPool.hpp"
#include "TransformInfoManager.hpp"
#include "StatusEntity.hpp"
#include "DCProto.pb.h"
#include <sys/types.h>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <cmath>
#include <string>

using namespace std;

struct Fight{
	int32_t id;
	FightAtt *att;
	struct Component *component;
	struct Fight *tFight;
	Vector3f tPos;
	const SkillInfo *curSkill;
	const SkillInfo *prevSkill;
	map<int64_t, int32_t> enemies;
	map<int32_t, int64_t> skillsCD;
	int64_t publicCD;
	vector<struct SkillEntity *> skills;
	int64_t deadTime;
	bool cantAttack;
	int64_t pkTarget;
	bool isTransforming;
	int64_t prevTransformTime;
	int32_t transformTimeDelta;
	map<int32_t, int32_t> skillCDDelta;
	map<int32_t, int32_t> skillAreaDelta;
	int32_t skillIncMana;
	map<int32_t, int32_t> skillEnergyDelta;
	map<int32_t, int32_t> skillManaDelta;
	bool clearRoom;
	bool openRoomFreeBox;
	bool openRoomGemBox;
	int recoverCount;
	int64_t enterRoomTime;
	bool waitPVP;
	int64_t waitPVPTime;
	bool waitHell;
	bool cantBeAttacked;
	bool isSpecial;
	bool ignorePK;
	int64_t ignorePKTime;
	int reviveCount;
	int petReviveCount;
	int32_t godTargetPower;
	int32_t godTargetScore;
	int killCount;
	int dieCount;
	int64_t unspecialTime;
	int specialProtect;
	struct StatusEntity *transformStatus;
	bool inspire;
	bool reviveWithPet;
	vector<int> npcID;
	// playofftest
	/*
	bool change;
	int level;
	int atts[6];
	FightPropertyDelta delta[6];
	*/
};

struct PVPComp{
	bool operator()(struct Fight *lhs, struct Fight *rhs) {
		return lhs->att->level() <= rhs->att->level();
	}
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct Fight *> pool;
	vector<int64_t> totalExp;
	int moneySoulRate[SOUL_RATE_LEVEL];
	int gemSoulRate[SOUL_RATE_LEVEL];
	int soulDelta[SOUL_RATE_LEVEL];
	set<struct Fight *, PVPComp> pvp;
	set<struct Fight *, PVPComp> hell;
}package;

void Fight_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();

	{
		string name = Config_DataPath() + string("/TotalExp.txt");
		FILE *file = fopen(name.c_str(), "r");
		if (file == NULL) {
			DEBUG_LOGERROR("Failed to open file: %s", name.c_str());
			exit(EXIT_FAILURE);
		}

		package.totalExp.clear();
		package.totalExp.push_back(-1);
		char line[128];
		while (fgets(line, sizeof(line), file) != NULL) {
			package.totalExp.push_back(atoll(line));
		}

		fclose(file);
	}

	{
		string name = Config_DataPath() + string("/SoulTable.txt");
		FILE *file = fopen(name.c_str(), "r");
		if (file == NULL) {
			DEBUG_LOGERROR("Failed to open file: %s", name.c_str());
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < SOUL_RATE_LEVEL; i++) {
			char key[16];
			SNPRINTF1(key, "%d", i);
			char line[CONFIG_FIXEDARRAY];
			if (!ConfigUtil_ReadStr(file, key, line, CONFIG_FIXEDARRAY)) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}

			char *tokens[CONFIG_FIXEDARRAY];
			int count = ConfigUtil_ExtraToken(line, tokens, CONFIG_FIXEDARRAY, ",");
			if (count == -1) {
				DEBUG_LOGERROR("Failed to read file: %s", name.c_str());
				exit(EXIT_FAILURE);
			}

			package.soulDelta[i] = (int)atoi(tokens[0]);
			package.moneySoulRate[i] = (int)atoi(tokens[1]);
			package.gemSoulRate[i] = (int)atoi(tokens[2]);
		}
		fclose(file);
	}

	package.pvp.clear();
	package.hell.clear();
}

struct Fight * Fight_Create(FightAtt *att, struct Component *component) {
	if (att == NULL || component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct Fight *fight = SearchBlock(package.pool, id);
	fight->id = id;
	fight->att = att;
	fight->component = component;
	fight->tFight = NULL;
	fight->curSkill = NULL;
	fight->prevSkill = NULL;
	fight->enemies.clear();
	fight->publicCD = CONFIG_INIT_USE_TIME;
	fight->skillsCD.clear();
	fight->skills.clear();
	fight->cantAttack = false;
	fight->pkTarget = -1;
	fight->isTransforming = false;
	fight->prevTransformTime = CONFIG_INIT_USE_TIME;
	fight->transformTimeDelta = 0;
	fight->skillCDDelta.clear();
	fight->skillAreaDelta.clear();
	fight->skillIncMana = 0;
	fight->skillEnergyDelta.clear();
	fight->skillManaDelta.clear();
	fight->clearRoom = true;
	fight->openRoomFreeBox = true;
	fight->openRoomGemBox = true;
	fight->recoverCount = 0;
	fight->enterRoomTime = 0;
	fight->waitPVP = false;
	fight->waitPVPTime = 0;
	fight->waitHell = false;
	fight->cantBeAttacked = false;
	fight->isSpecial = false;
	fight->ignorePK = false;
	fight->ignorePKTime = 0;
	fight->reviveCount = 0;
	fight->petReviveCount = 0;
	fight->godTargetPower = 0;
	fight->godTargetScore = 0;
	fight->killCount = 0;
	fight->dieCount = 0;
	fight->unspecialTime = 0;
	fight->specialProtect = 0;
	fight->transformStatus = NULL;
	fight->inspire = false;
	fight->reviveWithPet = false;
	fight->npcID.clear();
	// playofftest
	// fight->change = false;

	return fight;
}

bool Fight_IsValid(struct Fight *fight) {
	return fight != NULL && IDGenerator_IsValid(package.idGenerator, fight->id);
}

static void AddBloodNodeEffect(struct Fight *fight, int index) {
	if (fight->component->player == NULL)
		return;

	int prof = (int)fight->component->baseAtt->professionType() - 1;
	const BloodNodeInfo *node = BloodInfoManager_BloodNodeInfo(index);
	if (node == NULL) 
		return;
	BloodNodeInfo::Type type = node->type(prof);
	int arg1 = node->arg1(prof);
	int arg2 = node->arg2(prof);
	switch(type) {
		case BloodNodeInfo::ATT:
			{
				Fight_ModifyProperty(fight, (FightAtt::PropertyType)arg1, arg2);
			}
			break;
		case BloodNodeInfo::SKILL_CD:
			{
				map<int32_t, int32_t>::iterator it = fight->skillCDDelta.find(arg1);
				if (it == fight->skillCDDelta.end()) {
					fight->skillCDDelta[arg1] = arg2;
				} else {
					it->second += arg2;
				}
			}
			break;
		case BloodNodeInfo::AREA:
			{
				map<int32_t, int32_t>::iterator it = fight->skillAreaDelta.find(arg1);
				if (it == fight->skillAreaDelta.end()) {
					fight->skillAreaDelta[arg1] = arg2;
				} else {
					it->second += arg2;
				}
			}
			break;
		case BloodNodeInfo::INC_MANA:
			{
				fight->skillIncMana += arg1;
			}
			break;
		case BloodNodeInfo::INC_ENERGY:
			{
				map<int32_t, int32_t>::iterator it = fight->skillEnergyDelta.find(arg1);
				if (it == fight->skillEnergyDelta.end()) {
					fight->skillEnergyDelta[arg1] = arg2;
				} else {
					it->second += arg2;
				}
			}
			break;
		case BloodNodeInfo::DEC_MANA:
			{
				map<int32_t, int32_t>::iterator it = fight->skillManaDelta.find(arg1);
				if (it == fight->skillManaDelta.end()) {
					fight->skillManaDelta[arg1] = arg2;
				} else {
					it->second += arg2;
				}
			}
			break;
		case BloodNodeInfo::TRANSFORM_TIME:
			{
				fight->transformTimeDelta += arg1;
			}
			break;
		default:
			assert(0);
	}
}

static int64_t FinalSkillCD(struct Fight *fight, const SkillInfo *info) {
	if (fight->component->player == NULL)
		return info->cd();

	if (fight->isTransforming) {
		map<int32_t, int32_t>::iterator it = fight->skillCDDelta.find(info->id());
		return max(0, info->cd() - (it == fight->skillCDDelta.end() ? 0 : it->second));
	} else {
		return info->cd();
	}
}

static int32_t FinalSkillDecMana(struct Fight *fight, const SkillInfo *info) {
	if (fight->component->player == NULL)
		return info->mp();

	if (!fight->isTransforming)
		return info->mp();

	map<int32_t, int32_t>::iterator it = fight->skillManaDelta.find(info->id());
	return max(0, info->mp() - (it == fight->skillManaDelta.end() ? 0 : it->second));
}

static void AddBloodEffect(struct Fight *fight, int level, FightAtt::PropertyType type, int count) {
	if (count <= 0)
		return;

	const BloodInfo *info = BloodInfoManager_BloodInfo(level);
	if (type == FightAtt::ATK) {
		Fight_ModifyProperty(fight, type, count * info->toAtk());
	} else if (type == FightAtt::DEF) {
		Fight_ModifyProperty(fight, type, count * info->toDef());
	} else if (type == FightAtt::DODGE) {
		Fight_ModifyProperty(fight, type, count * info->toDodge());
	} else if (type == FightAtt::ACCURACY) {
		Fight_ModifyProperty(fight, type, count * info->toAccuracy());
	} else {
		assert(0);
	}
}

static void ModifyLevel(struct Fight *fight, int delta, bool prepare) {
	if (delta == 0)
		return;

	int curLevel = fight->att->level();
	const BaseAtt *baseAtt = fight->component->baseAtt;
	if (!prepare) {
		fight->att->set_level(fight->att->level() + delta);
		for (int i = 1; i <= delta; ++i) {
			int index = 0;
			const AwardInfo* info = AwardInfoManager_AwardInfo(AwardInfo::TEN_GIFT, curLevel + i, &index);
			if (info == NULL) {
				continue;
			}

			if (!(fight->component->playerAtt->fixedEvent(84) & (1 << index))) {
				PlayerEntity_SetFixedEvent(fight->component->player, 84, (fight->component->playerAtt->dayEvent(84) | (1 << index)));
				PlayerEntity_AddMail(fight->component->player, ItemInfo::GOODS, info->award(), 1, info->name().c_str(), info->content().c_str());
			}
		}
		
		if (curLevel + delta >= Config_InviteCodeLevel() && curLevel < Config_InviteCodeLevel()) {
			const PlayerAtt *att = PlayerEntity_Att(fight->component->player);
			if (att != NULL) {
				string str = "";
				if (str.compare(att->othercode()) != 0) {
					PlayerPool_AddInviteCode(att->othercode());
				}
			}
		}
	}

	switch(baseAtt->professionType()) {
		case ProfessionInfo::KNIGHT:
			{
				Fight_ModifyProperty(fight, FightAtt::ATK, delta * 10);
				Fight_ModifyProperty(fight, FightAtt::DEF, delta * 6);
				Fight_ModifyProperty(fight, FightAtt::MAXHP, delta * 110);
				Fight_ModifyProperty(fight, FightAtt::ACCURACY, delta * 2);
				Fight_ModifyProperty(fight, FightAtt::CRIT, delta * 3);
				Fight_ModifyProperty(fight, FightAtt::DODGE, delta * 3);
			}
			break;
		case ProfessionInfo::RANGER:
			{
				Fight_ModifyProperty(fight, FightAtt::ATK, delta * 9);
				Fight_ModifyProperty(fight, FightAtt::DEF, delta * 5);
				Fight_ModifyProperty(fight, FightAtt::MAXHP, delta * 100);
				Fight_ModifyProperty(fight, FightAtt::ACCURACY, delta * 4);
				Fight_ModifyProperty(fight, FightAtt::CRIT, delta * 3);
				Fight_ModifyProperty(fight, FightAtt::DODGE, delta * 4);
			}
			break;
		case ProfessionInfo::MAGICIAN:
			{
				Fight_ModifyProperty(fight, FightAtt::ATK, delta * 10);
				Fight_ModifyProperty(fight, FightAtt::DEF, delta * 5);
				Fight_ModifyProperty(fight, FightAtt::MAXHP, delta * 100);
				Fight_ModifyProperty(fight, FightAtt::ACCURACY, delta * 4);
				Fight_ModifyProperty(fight, FightAtt::CRIT, delta * 3);
				Fight_ModifyProperty(fight, FightAtt::DODGE, delta * 3);
			}
			break;
		default:
			assert(0);
			break;
	}

	fight->att->set_hp(Fight_FinalProperty(fight, FightAtt::MAXHP));
	fight->att->set_mana(Config_MaxMana());

//	if (!prepare) {
//		for (int i = 0; i < fight->att->skills_size(); i++) {
//			while (true) {
//				if (Fight_SkillLevelUp(fight, i, 1) != 0)
//					break;
//			}
//		}
//	}

	static NetProto_LevelUp levelUp;
	levelUp.Clear();
	int32_t id = PlayerEntity_ID(fight->component->player);
	levelUp.set_id(id);
	levelUp.set_level(fight->att->level());
	GCAgent_SendProtoToClients(&id, 1, &levelUp);

	{
		int64_t roleID = PlayerEntity_RoleID(fight->component->player);
		int64_t value = (((int64_t)(fight->att->level())) << 32) + Fight_Power(fight);
		PlayerPool_SynRoleLevelAndPower(roleID, value);

		const PlayerAtt *att = PlayerEntity_Att(fight->component->player);
		FactionPool_SynFactionLevel(att);
	}
}

static void AdjustOriginSkill(struct Fight *fight) {
	const PlayerAtt *origin = ProfessionInfoManager_ProfessionInfo(fight->component->baseAtt->professionType(), fight->component->baseAtt->male());
	int originSkill = origin->att().fightAtt().skills(0).id();
	fight->att->mutable_skills(0)->set_id(originSkill);
}

void Fight_Prepare(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;

	if (fight->component->player != NULL) {
		AdjustOriginSkill(fight);

		const PlayerAtt *info = ProfessionInfoManager_ProfessionInfo(fight->component->baseAtt->professionType(), fight->component->baseAtt->male());
		assert(info != NULL);
		for (int i = 0; i < fight->att->properties_size(); i++)
			fight->att->set_properties(i, info->att().fightAtt().properties(i));

		ModifyLevel(fight, fight->att->level() - 1, true);

		// for (int i = 0; i <= fight->att->bloodLevel(); i++) {
		for (int i = 0; i < BloodInfoManager_MaxBloodLevel(); i++) {
			AddBloodEffect(fight, i, FightAtt::ATK, fight->att->bloodDelta(i).toAtk());
			AddBloodEffect(fight, i, FightAtt::DEF, fight->att->bloodDelta(i).toDef());
			AddBloodEffect(fight, i, FightAtt::DODGE, fight->att->bloodDelta(i).toDodge());
			AddBloodEffect(fight, i, FightAtt::ACCURACY, fight->att->bloodDelta(i).toAccuracy());
		}

		for (int i = 0; i < fight->att->bloodNode(); i++)
			AddBloodNodeEffect(fight, i);

		Fight_EnhanceByPet(fight, true);
		if (Time_TimeStamp() < fight->component->playerAtt->fixedEvent(36)) {
			int level = fight->att->level();
			int count = (fight->component->playerAtt->fixedEvent(35) & 0x0FFFF);
			int detail = level * count * (1 + count /100);
			Fight_ModifyPropertyDelta(fight, FightAtt::ATK, detail, 0);
			Fight_ModifyPropertyDelta(fight, FightAtt::DEF, detail, 0);
		}
	} else if (fight->component->npc != NULL) {
		if (fight->component->npcAtt->specialPercent() >= 1.0f)
			Fight_SetSpecial(fight, true);
	}
}

void Fight_ClearSkills(struct Fight *fight) {
	for (size_t i = 0; i < fight->skills.size(); i++) {
		if (fight->skills[i] == NULL)
			continue;

		SkillEntity_Destroy(fight->skills[i]);
	}
}

void Fight_Destroy(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;
	IDGenerator_Release(package.idGenerator, fight->id);
}

void Fight_Finalize(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;

	// playofftest
	/*
	if (fight->change) {
		fight->att->set_level(fight->level);
		for (int i = 0; i < fight->att->properties_size(); i++)
			fight->att->set_properties(i, fight->atts[i]);
		for (int i = 0; i < fight->att->propertiesDelta_size(); i++)
			*fight->att->mutable_propertiesDelta(i) = fight->delta[i];
		fight->att->set_hp(Fight_FinalProperty(fight, FightAtt::MAXHP));
		fight->change = false;
	}
	*/

	Fight_ClearSkills(fight);
	Fight_EndWaitPVP(fight);
	Fight_EndWaitHell(fight);
	Fight_Transform(fight, false);

	int32_t map = Movement_Att(fight->component->movement)->mapID();
	if (fight->component->player != NULL) {
		bool over = false;
		const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
		if (info != NULL) {
			if (info->mapType() == MapInfo::PRACTICE || info->mapType() == MapInfo::PVP) {
				Fight_ClearRoom(fight, false, 0);
				over = true;
			}
		}
		if (!over) {
			map = Movement_NextMap(fight->component->movement);
			info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
			if (info != NULL) {
				if (info->mapType() == MapInfo::PRACTICE || info->mapType() == MapInfo::PVP) {
					Fight_ClearRoom(fight, false, 0);
				}
			}
		}
	}

	struct NPCEntity *pet = NPCPool_Pet(map, fight->component);
	if (NPCEntity_IsValid(pet)) {
		MapPool_ReleaseNPCs(map, NPCEntity_ID(pet), true);
	}
}

const FightAtt * Fight_Att(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return NULL;

	return fight->att;
}

struct Component * Fight_Component(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return NULL;

	return fight->component;
}

static inline bool IsNear(const Vector3f *v1, const Vector3f *v2, int dist) {
	if (dist <= 0)
		dist = 1;
	return Vector3f_Distance(v1, v2) < dist;
}

/*
   static inline bool IsNear(const Vector2i *v1, const Vector2i *v2, int dist) {
   return abs(v1->x() - v2->x()) <= dist && abs(v1->y() - v2->y()) <= dist;
   }
   */

static inline int64_t SkillCD(struct Fight *fight, int32_t id) {
	if (fight->skillsCD.find(id) == fight->skillsCD.end())
		fight->skillsCD[id] = CONFIG_INIT_USE_TIME;
	return fight->skillsCD[id];
}

static const SkillInfo * LastSkill(const SkillInfo *skill) {
	while (skill->next() != -1)
		skill = SkillInfoManager_SkillInfo(skill->next(), skill->level());
	return skill;
}

bool Fight_IsCDOver(struct Fight *fight, const SkillInfo *skill, int64_t cur) {
	if (cur > Time_ElapsedTime())
		return false;

	// int64_t cur = Time_ElapsedTime();

	bool family = false;
	if (fight->prevSkill != NULL) {
		if (fight->prevSkill->next() == skill->id()) {
			if (cur - SkillCD(fight, fight->prevSkill->id()) < FinalSkillCD(fight, fight->prevSkill))
				return false;
			if (cur - SkillCD(fight, skill->id()) < FinalSkillCD(fight, skill))
				return false;
			return true;
		}
		else {
			const SkillInfo *last = LastSkill(fight->prevSkill);

			for (const SkillInfo *root = skill; ; root = SkillInfoManager_SkillInfo(root->next(), root->level())) {
				if (root->id() == fight->prevSkill->id()) {
					if (last->id() == fight->prevSkill->id()) {
						if (cur - SkillCD(fight, fight->prevSkill->id()) < FinalSkillCD(fight, last))
							return false;
					}
					else {
						if (cur - SkillCD(fight, fight->prevSkill->id()) < FinalSkillCD(fight, fight->prevSkill) + fight->prevSkill->interval() + last->interval())
							return false;
					}
					family = true;
				}

				if (root->next() == -1)
					break;
			}
		}
	}

	if (cur - SkillCD(fight, skill->id()) < FinalSkillCD(fight, skill))
		return false;

	if (fight->component->npc != NULL)
		if (fight->prevSkill != NULL)
			if (cur - fight->publicCD < fight->prevSkill->fireActionTime())
				return false;

	if (skill->next() != -1 && !family) {
		const SkillInfo *prev = skill;
		int64_t prevCD = SkillCD(fight, skill->id());
		for (const SkillInfo *curSkill = SkillInfoManager_SkillInfo(prev->id(), prev->level()); curSkill != NULL; curSkill = SkillInfoManager_SkillInfo(curSkill->next(), curSkill->level())) {
			if (SkillCD(fight, curSkill->id()) > prevCD) {
				prev = curSkill;
				prevCD = SkillCD(fight, curSkill->id());
			}
		}
		if (prev->next() == -1) {
			if (cur - prevCD < FinalSkillCD(fight, prev))
				return false;
		}
		else {
			const SkillInfo *last = LastSkill(prev);
			if (cur - prevCD < FinalSkillCD(fight, prev) + prev->interval() + last->interval())
				return false;
		}
	}

	return cur - fight->publicCD >= Config_PublicCD();
}

void Fight_Idle(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;
	fight->att->set_status(FightAtt::IDLE);
}

void Fight_Cancel(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;
	if (fight->att->status() == FightAtt::DEAD)
		return;

	fight->tFight = NULL;
	fight->att->set_status(FightAtt::IDLE);
	// fight->prevSkill = fight->curSkill = NULL;
	fight->curSkill = NULL;
	Movement_Stop(fight->component->movement);
}

void Fight_EnhanceByPet(struct Fight *fight, bool add) {
	/*
	   if (!Fight_IsValid(fight))
	   return;
	   if (fight->att->enhancingPet() == -1)
	   return;

	   const PetAsset *pet = &fight->component->playerAtt->pets(fight->att->enhancingPet());
	   const NPCAtt *att = NPCInfoManager_Pet(pet->id());
	   assert(att != NULL);

	   for (int i = 0; i < att->att().fightAtt().properties_size(); i++) {
	   Fight_ModifyProperty(fight, (FightAtt::PropertyType)i, (add ? 1 : -1) * att->att().fightAtt().properties(i) * pet->level());
	   }
	   */
}

int32_t Fight_Power(const PB_FightAtt *att) {
	if (att == NULL)
		return 0;

	int32_t atk = Fight_FinalProperty(att, PB_FightAtt::ATK);
	int32_t def = Fight_FinalProperty(att, PB_FightAtt::DEF);
	int32_t maxHP = Fight_FinalProperty(att, PB_FightAtt::MAXHP);
	int32_t crit = Fight_FinalProperty(att, PB_FightAtt::CRIT);
	int32_t accuracy = Fight_FinalProperty(att, PB_FightAtt::ACCURACY);
	int32_t dodge = Fight_FinalProperty(att, PB_FightAtt::DODGE);

	int32_t skill = 0;
	for (int i = 0; i < att->skills_size(); i++) {
		const PB_Skill *info = &att->skills(i);
		if (info->id() != -1 && info->level() > 0)
			skill += Config_SkillPower(info->level());
	}

	return atk + def + maxHP / 10 + crit + accuracy + dodge + skill;
}

int32_t Fight_Power(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return 0;

	if (fight->isTransforming)
		Fight_ModifyPropertyDelta(fight, FightAtt::ATK, 0, -Config_TransformEffect(), false);
	int32_t atk = Fight_FinalProperty(fight, FightAtt::ATK);
	if (fight->isTransforming)
		Fight_ModifyPropertyDelta(fight, FightAtt::ATK, 0, Config_TransformEffect(), false);
	int32_t def = Fight_FinalProperty(fight, FightAtt::DEF);
	int32_t maxHP = Fight_FinalProperty(fight, FightAtt::MAXHP);
	int32_t crit = Fight_FinalProperty(fight, FightAtt::CRIT);
	int32_t accuracy = Fight_FinalProperty(fight, FightAtt::ACCURACY);
	int32_t dodge = Fight_FinalProperty(fight, FightAtt::DODGE);

	int32_t skill = 0;
	for (int i = 0; i < fight->att->skills_size(); i++) {
		const Skill *info = &fight->att->skills(i);
		if (info->id() != -1 && info->level() > 0)
			skill += Config_SkillPower(info->level());
	}

	return atk + def + maxHP / 10 + crit + accuracy + dodge + skill;
}

static void InspireEffect(struct Fight *fight) {
	if (fight->component->npc != NULL) {
		if (Fight_IsSpecial(fight))
			return;
		if (fight->att->hp() > 0 && fight->unspecialTime == 0 && fight->att->hp() <= Fight_FinalProperty(fight, FightAtt::MAXHP) * fight->component->npcAtt->specialPercent()) {
			Fight_SetSpecial(fight, true);
		}
	} else if (fight->component->player != NULL) {
		// playofftest
		// return;

		int32_t map = fight->component->roleAtt->movementAtt().mapID();
		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
		if (mapInfo == NULL)
			return;
		int index = -1;
		if (map == MapPool_WorldBoss())
			index = 19;
		else if (mapInfo->mapType() == MapInfo::PVP)
			index = 17;
		else if (mapInfo->mapType() == MapInfo::HELL)
			index = 18;
		if (index != -1) {
			int special = (fight->component->playerAtt->fixedEvent(index) & 0xff0000) >> 16;
			if (fight->att->hp() > 0 && fight->att->hp() <= Fight_FinalProperty(fight, FightAtt::MAXHP) * (special * 0.05f)) {
				fight->inspire = true;
				Fight_SetSpecial(fight, true);
			} else {
				fight->inspire = false;
				Fight_SetSpecial(fight, false);
			}
		} else {
			fight->inspire = false;
			Fight_SetSpecial(fight, false);
		}
	}
}

void Fight_ModifyHP(struct Fight *fight, struct Fight *maker, int32_t delta) {
	if (!Fight_IsValid(fight))
		return;
	if (fight->att->status() == FightAtt::DEAD)
		return;

	if (delta <= 0) {
		if (Fight_IsValid(maker)) {
			Fight_AddEnemy(fight, maker);
			Fight_AddEnemy(maker, fight);

			if (fight->component->npc != NULL) {
				struct Component *target = NULL;
				if (maker->component->player != NULL) {
					target = maker->component;
				} else if (maker->component->npc != NULL) {
					struct Component *master = NPCEntity_Master(maker->component->npc);
					if (master != NULL && master->player != NULL)
						target = master;
				}
				if (target != NULL) {
				/*
					{
						int cur = fight->component->roleAtt->movementAtt().mapID();
						if (cur == MapPool_WorldBoss()) {
							delta = -50000000;
						}
					}
				*/
					int64_t roleID = target->baseAtt->roleID();
					if (fight->enemies.find(roleID) != fight->enemies.end())
						fight->enemies[roleID] += -delta;
					else
						fight->enemies[roleID] = -delta;

					int cur = fight->component->roleAtt->movementAtt().mapID();
					if (cur == MapPool_WorldBoss()) {
						target->fight->att->set_worldBossHurt(-delta + target->fight->att->worldBossHurt());
						target->fight->att->set_worldBossNum(MapPool_WorldBossNum());
					} else if (cur == MapPool_FactionWar()) {
						int32_t npcID = NPCEntity_ID(fight->component->npc);
						const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(cur));
						const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
						if (mapUnit != NULL && npcID == mapUnit->boss())
							PlayerPool_UpdateFactionFight(target->playerAtt->faction(), -delta);
					}
				}
			}
		}
	}

	fight->att->set_hp(fight->att->hp() + delta);
	// DEBUG_LOG("ModifyHP, delta: %d, cur: %d", delta, fight->att->hp());
	if (fight->att->hp() <= 0) {
		fight->att->set_hp(0);
		Fight_Die(fight, maker);
	} else if (fight->att->hp() > Fight_FinalProperty(fight, FightAtt::MAXHP)) {
		fight->att->set_hp(Fight_FinalProperty(fight, FightAtt::MAXHP));
	}

	InspireEffect(fight);
}

void Fight_ModifyMana(struct Fight *fight, struct Fight *maker, int32_t delta) {
	if (!Fight_IsValid(fight))
		return;
	if (fight->att->status() == FightAtt::DEAD)
		return;

	if (delta < 0) {
		if (Fight_IsValid(maker)) {
			Fight_AddEnemy(fight, maker);
			Fight_AddEnemy(maker, fight);
		}
	}

	fight->att->set_mana(fight->att->mana() + delta);
	if (fight->att->mana() < 0) {
		fight->att->set_mana(0);
	} else if (fight->att->mana() > Config_MaxMana()) {
		fight->att->set_mana(Config_MaxMana());
	}
}

void Fight_ModifyEnergy(struct Fight *fight, struct Fight *maker, int32_t delta) {
	/*
	   if (!Fight_IsValid(fight))
	   return;
	   if (fight->att->status() == FightAtt::DEAD)
	   return;

	   if (delta < 0) {
	   if (Fight_IsValid(maker)) {
	   Fight_AddEnemy(fight, maker);
	   Fight_AddEnemy(maker, fight);
	   }
	   }

	   fight->att->set_energy(fight->att->energy() + delta);
	   if (fight->att->energy() < 0) {
	   fight->att->set_energy(0);
	   } else if (fight->att->energy() > Config_MaxEnergy()) {
	   fight->att->set_energy(Config_MaxEnergy());
	   }
	   */
}

int Fight_SetPKTarget(struct Fight *fight, int64_t target) {
	if (!Fight_IsValid(fight))
		return -1;
	struct PlayerEntity *targetPlayer = PlayerEntity_PlayerByRoleID(target);
	if (targetPlayer == NULL)
		return -1;
	struct Component *component = PlayerEntity_Component(targetPlayer);

	const MapInfo *cur = MapInfoManager_MapInfo(MapPool_MapInfo(Movement_Att(fight->component->movement)->mapID()));
	if (cur == NULL || cur->mapType() != MapInfo::PEACE)
		return -2;

	cur = MapInfoManager_MapInfo(MapPool_MapInfo(Movement_Att(component->movement)->mapID()));
	if (cur == NULL || cur->mapType() != MapInfo::PEACE)
		return -2;

	if (component->fight->ignorePK)
		return -3;

	fight->pkTarget = target;
	return 0;
}

int64_t Fight_PKTarget(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return -1;

	return fight->pkTarget;
}

bool Fight_DoPK(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return false;

	struct PlayerEntity *target = PlayerEntity_PlayerByRoleID(fight->pkTarget);
	if (target == NULL)
		return false;
	struct Component *component = PlayerEntity_Component(target);

	const MapInfo *cur = MapInfoManager_MapInfo(MapPool_MapInfo(Movement_Att(fight->component->movement)->mapID()));
	if (cur == NULL || cur->mapType() != MapInfo::PEACE)
		return false;

	cur = MapInfoManager_MapInfo(MapPool_MapInfo(Movement_Att(component->movement)->mapID()));
	if (cur == NULL || cur->mapType() != MapInfo::PEACE)
		return false;

	int32_t pvp = Config_PracticeMap();
	assert(pvp != -1);
	const MapInfo *info = MapInfoManager_MapInfo(pvp);
	assert(info != NULL);

	static Vector2i nextCoord1, nextCoord2;
	if (!MapInfoManager_EnterCoord(info, 0, &nextCoord1) || !MapInfoManager_EnterCoord(info, 1, &nextCoord2)) {
		DEBUG_LOGERROR("Failed to get enter coord");
		assert(0);
	}

	int32_t room = MapPool_Gen(pvp, true);
	if (room == -1) {
		DEBUG_LOGERROR("Failed to gen room");
		return false;
	}

	Movement_BeginChangeScene(fight->component->movement, room, &nextCoord1);
	Movement_BeginChangeScene(component->movement, room, &nextCoord2);

	/*
	   int faction = 0;
	   fight->att->set_selfFaction(1 << faction++);
	   fight->att->set_friendlyFaction(fight->att->selfFaction());
	   component->fight->att->set_selfFaction(1 << faction++);
	   component->fight->att->set_friendlyFaction(component->fight->att->selfFaction());
	   */

	fight->pkTarget = -1;
	return true;
}

bool Fight_IsEnemy(struct Fight *fight, struct Fight *defender) {
	if (!Fight_IsValid(fight) || !Fight_IsValid(defender) || fight == defender)
		return false;

	if (defender->component->player != NULL) {
		int64_t roleID = defender->component->baseAtt->roleID();
		if (fight->enemies.find(roleID) != fight->enemies.end())
			return true;
	}

	return ((fight->att->selfFaction() | fight->att->friendlyFaction()) & defender->att->selfFaction()) == 0;
}

void Fight_AddEnemy(struct Fight *fight, struct Fight *enemy) {
	if (!Fight_IsValid(fight) || !Fight_IsValid(enemy) || fight == enemy)
		return;

	if (Fight_IsEnemy(fight, enemy))
		return;

	if (enemy->component->player != NULL) {
		int64_t roleID = enemy->component->baseAtt->roleID();
		if (fight->enemies.find(roleID) == fight->enemies.end())
			fight->enemies[roleID] = 0;
	}
}

void Fight_DelEnemy(struct Fight *fight, struct Fight *enemy) {
	if (!Fight_IsValid(fight) || !Fight_IsValid(enemy) || fight == enemy)
		return;

	if (enemy->component->player != NULL) {
		int64_t roleID = enemy->component->baseAtt->roleID();
		fight->enemies.erase(roleID);
	}
}

void Fight_ClearEnemy(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;

	for (map<int64_t, int32_t>::iterator it = fight->enemies.begin(); it != fight->enemies.end(); it++) {
		struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(it->first);
		if (entity != NULL) {
			struct Component *component = PlayerEntity_Component(entity);
			if (component != NULL)
				Fight_DelEnemy(component->fight, fight);
		}
	}
	fight->enemies.clear();
}

int Fight_Skills(struct Fight *fight, struct SkillEntity **skills, size_t size) {
	if (!Fight_IsValid(fight) || skills == NULL)
		return -1;

	if (size < fight->skills.size())
		return -1;

	int count = 0;
	for (size_t i = 0; i < fight->skills.size(); i++) {
		if (fight->skills[i] == NULL)
			continue;

		skills[count++] = fight->skills[i];
	}

	return count;
}

int Fight_BindSkill(struct Fight *fight, struct SkillEntity *skill, int32_t id) {
	if (!Fight_IsValid(fight) || skill == NULL)
		return -1;

	if (id == -1) {
		assert(fight->component->npc != NULL);
		for (size_t i = 0; i < fight->skills.size(); i++) {
			if (fight->skills[i] == NULL) {
				fight->skills[i] = skill;
				return 0;
			}
		}
		fight->skills.push_back(skill);
	}
	else {
		assert(fight->component->player != NULL);
		if (id >= (int32_t)fight->skills.size())
			fight->skills.resize(id + 1, NULL);
		if (fight->skills[id] != NULL)
			return -2;
		fight->skills[id] = skill;
	}

	return 0;
}

void Fight_DelSkill(struct Fight *fight, struct SkillEntity *skill) {
	if (!Fight_IsValid(fight) || skill == NULL)
		return;

	for (size_t i = 0; i < fight->skills.size(); i++) {
		if (fight->skills[i] == skill) {
			fight->skills[i] = NULL;
			break;
		}
	}

	/*
	   if (fight->prevSkill != NULL) {
	   if (SkillEntity_Info(skill)->id() == fight->prevSkill->id()) {
	   fight->prevSkill = NULL;
	   }
	   }
	   */
}

int32_t Fight_SkillClientID(struct Fight *fight, struct SkillEntity *skill) {
	if (!Fight_IsValid(fight) || skill == NULL)
		return -1;

	for (size_t i = 0; i < fight->skills.size(); i++) {
		if (fight->skills[i] == skill) {
			return (int32_t)i;
		}
	}

	return -1;
}

bool Fight_HasSkillLaunchType(struct Fight *fight, SkillInfo::LaunchType launchType) {
	if (!Fight_IsValid(fight))
		return false;

	for (size_t i = 0; i < fight->skills.size(); i++) {
		if (fight->skills[i] == NULL)
			continue;

		const SkillInfo *info = SkillEntity_Info(fight->skills[i]);
		if (info->launchType() == launchType)
			return true;
	}

	return false;
}

bool Fight_HasSkillLaunchTypes(struct Fight *fight, SkillInfo::LaunchType launchTypes[], size_t size) {
	if (!Fight_IsValid(fight))
		return false;

	for (size_t i = 0; i < fight->skills.size(); i++) {
		if (fight->skills[i] == NULL)
			continue;

		const SkillInfo *info = SkillEntity_Info(fight->skills[i]);
		for (size_t j = 0; j < size; j++) {
			if (info->launchType() == launchTypes[j])
				return true;
		}
	}

	return false;
}

static void SendDoAttack(struct Fight *fight, int32_t *ids, size_t count, const int32_t *skills, size_t size) {
	static NetProto_DoAttack doAttack;
	doAttack.Clear();
	if (fight->component->player != NULL) {
		doAttack.set_aType(NetProto_DoAttack::PLAYER);
		doAttack.set_aID(PlayerEntity_ID(fight->component->player));
	} else if (fight->component->npc != NULL) {
		doAttack.set_aType(NetProto_DoAttack::NPC);
		doAttack.set_aID(NPCEntity_ID(fight->component->npc));
	} else {
		assert(0);
	}

	doAttack.mutable_aSkill()->set_id(fight->curSkill->id());
	doAttack.mutable_aSkill()->set_level(0);
	doAttack.clear_aSkillID();
	if (skills == NULL || size <= 0) {
		assert(fight->component->npc != NULL);
		for (size_t i = 0; i < count; i++) {
			doAttack.add_aSkillID(ids[i]);
		}
	} else {
		assert(fight->component->player != NULL);
		for (size_t i = 0; i < size; i++) {
			doAttack.add_aSkillID(skills[i]);
		}
	}

	if (fight->tFight != NULL) {
		if (fight->tFight->component->player != NULL) {
			doAttack.set_dType(NetProto_DoAttack::PLAYER);
			doAttack.set_dID(PlayerEntity_ID(fight->tFight->component->player));
		} else if (fight->tFight->component->npc != NULL) {
			doAttack.set_dType(NetProto_DoAttack::NPC);
			doAttack.set_dID(NPCEntity_ID(fight->tFight->component->npc));
		} else {
			assert(0);
		}
	} else {
		doAttack.set_dID(-1);
		fight->tPos.ToPB(doAttack.mutable_tPos());
	}

	Movement_Att(fight->component->movement)->position().ToPB(doAttack.mutable_aPos());
	if (fight->component->player != NULL)
		GCAgent_SendProtoToAllClientsExceptOneInSameScene(fight->component->player, &doAttack);
	else if (fight->component->npc != NULL)
		GCAgent_SendProtoToAllClientsInSameScene(Movement_Att(fight->component->movement)->mapID(), fight->component->player, &doAttack);
	else
		assert(0);
}

void Fight_DestroyClientSkills(struct Fight *fight, const int32_t *skills, size_t size) {
	if (!Fight_IsValid(fight) || skills == NULL || size <= 0)
		return;
	if (fight->component->player == NULL)
		return;

	static NetProto_DestroySkill destroySkill;
	destroySkill.Clear();
	int playerID = PlayerEntity_ID(fight->component->player);
	destroySkill.set_id(playerID);
	destroySkill.set_immediately(true);
	for (size_t i = 0; i < size; i++)
		destroySkill.add_skill(skills[i]);
	GCAgent_SendProtoToClients(&playerID, 1, &destroySkill);
}

int Fight_DoAttack(struct Fight *fight, const int32_t *skills, size_t size, int64_t cur) {
	if (!Fight_IsValid(fight))
		return -1;

	if (fight->att->status() == FightAtt::DEAD)
		return -2;

	if (fight->cantAttack) {
		Fight_Cancel(fight);
		return -3;
	}

	if (fight->tFight != NULL) {
		if (!Movement_SameScene(fight->component->movement, fight->tFight->component->movement)) {
			Fight_Cancel(fight);
			return -4;
		}
	}

	/*
	   if (fight->curSkill->hp() >= fight->att->hp() || (fight->curSkill->mp() >= 0 ? fight->curSkill->mp() : 0) > fight->att->mana()) {
	   Fight_Cancel(fight);
	   return -5;
	   }
	   */

	fight->att->set_hp(fight->att->hp() - fight->curSkill->hp());
	if (fight->curSkill->mp() > 0)
		fight->att->set_mana(fight->att->mana() - FinalSkillDecMana(fight, fight->curSkill));

	Movement_Stop(fight->component->movement);
	fight->att->set_status(FightAtt::IDLE);
	fight->publicCD = cur;
	fight->skillsCD[fight->curSkill->id()] = fight->publicCD;
	fight->prevSkill = fight->curSkill;
	if (fight->component->npc != NULL)
		fight->att->set_status(FightAtt::ATTACK);

	int32_t ids[CONFIG_FIXEDARRAY];
	int count = SkillEntity_Create(fight->curSkill, fight, fight->tFight, fight->tFight == NULL ? &fight->tPos : NULL, ids, CONFIG_FIXEDARRAY, skills, size);
	if (count < 0) {
		// DEBUG_LOGERROR("Failed to create skill entity");
		return -6;
	}

	SendDoAttack(fight, ids, count, skills, size);
	return 0;
}

bool Fight_HasSkill(struct Fight *fight, const SkillInfo *skill) {
	if (!Fight_IsValid(fight))
		return false;
	for (int i = 0;  i< fight->att->skills_size(); i++) {
		const Skill *cur = &fight->att->skills(i);
		if (cur->id() == -1) {
			continue;
		} else if (cur->id() == skill->id() && cur->level() == skill->level()) {
			return true;
		} else {
			const SkillInfo *info = SkillInfoManager_SkillInfo(cur->id(), cur->level());
			for (info = SkillInfoManager_SkillInfo(info->next(), info->level()); info != NULL; info = SkillInfoManager_SkillInfo(info->next(), info->level())) {
				if (info->id() == skill->id() && info->level() == skill->level())
					return true;
			}
		}
	}
	return false;
}

const SkillInfo * Fight_NextSkill(struct Fight *fight, const SkillInfo *root) {
	if (root == NULL || fight->prevSkill == NULL)
		return root;

	if (fight->prevSkill->next() == -1)
		return root;

	if (fight->prevSkill->next() != -1) {
		const SkillInfo *skill = SkillInfoManager_SkillInfo(fight->prevSkill->next(), fight->prevSkill->level());
		if (skill->next() != -1) {
			for (skill = SkillInfoManager_SkillInfo(skill->next(), skill->level()); ; skill = SkillInfoManager_SkillInfo(skill->next(), skill->level())) {
				if (skill->id() == root->id())
					return NULL;

				if (skill->next() == -1)
					break;
			}
		}
	}

	bool family = false;
	const SkillInfo *skill = root;
	for (;;) {
		if (skill == fight->prevSkill) {
			family = true;
			break;
		}

		if (skill->next() == -1)
			break;

		/*
		   int i = 0;
		   for (; i < fight->att->skills_size(); i++) {
		   if (fight->att->skills(i).id() == -1)
		   continue;
		   if (fight->att->skills(i).id() == skill->next())
		   break;
		   }
		   if (i >= fight->att->skills_size()) {
		   DEBUG_LOGERROR("Failed to find next skill");
		   return root;
		   }
		   */

		skill = SkillInfoManager_SkillInfo(skill->next(), skill->level());
	}

	if (!family)
		return root;

	if (Time_ElapsedTime() - SkillCD(fight, fight->prevSkill->id()) >= (int64_t)(skill->interval() + FinalSkillCD(fight, skill)))
		return root;

	if (skill->next() == -1)
		return root;

	/*
	   int i = 0;
	   for (; i < fight->att->skills_size(); i++) {
	   if (fight->att->skills(i).id() == -1)
	   continue;
	   if (fight->att->skills(i).id() == skill->next())
	   break;
	   }
	   if (i >= fight->att->skills_size()) {
	   DEBUG_LOGERROR("Failed to find next skill");
	   return root;
	   }
	   */

	return SkillInfoManager_SkillInfo(skill->next(), skill->level());
}

bool Fight_CantAttack(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return true;

	return fight->cantAttack;
}

void Fight_SetCantAttack(struct Fight *fight, bool cantAttack) {
	if (!Fight_IsValid(fight))
		return;

	fight->cantAttack = cantAttack;
}

bool Fight_CantBeAttacked(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return true;

	return fight->cantBeAttacked;
}

void Fight_SetCantBeAttacked(struct Fight *fight, bool cantBeAttacked) {
	if (!Fight_IsValid(fight))
		return;

	fight->cantBeAttacked = cantBeAttacked;
}

bool Fight_IsSpecial(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return true;

	return fight->isSpecial;
}

void Fight_SetSpecial(struct Fight *fight, bool special) {
	if (!Fight_IsValid(fight))
		return;

	if (fight->component->npc != NULL) {
		if (fight->isSpecial && !special) {
			fight->unspecialTime = Time_ElapsedTime();
		} else if (!fight->isSpecial && special) {
			fight->specialProtect = 0;
		} else {
			return;
		}

		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(fight->component->roleAtt->movementAtt().mapID()));
		if (mapInfo != NULL) {
			if (mapInfo->mapType() == MapInfo::ROOM || mapInfo->mapType() == MapInfo::ONLY_ROOM) {
				static NetProto_SetSpecial proto;
				proto.set_npc(NPCEntity_ID(fight->component->npc));
				proto.set_enable(special);
				GCAgent_SendProtoToAllClientsInSameScene(fight->component->roleAtt->movementAtt().mapID(), &proto);
			}
		}

		fight->isSpecial = special;
	} else if (fight->component->player != NULL) {
		if (special) {
			fight->isSpecial = true;
		} else {
			if (!fight->inspire && !fight->isTransforming)
				fight->isSpecial = false;
		}
	}
}

void Fight_AddSpecialProtect(struct Fight *fight, int v) {
	if (!Fight_IsValid(fight))
		return;
	if (!fight->isSpecial)
		return;

	fight->specialProtect += v;
	if (fight->specialProtect > (int)(Fight_FinalProperty(fight, FightAtt::MAXHP) * 0.1f)) {
		fight->specialProtect = 0;
		Fight_SetSpecial(fight, false);
	}
}

int32_t Fight_FinalProperty(const PB_FightAtt *att, PB_FightAtt::PropertyType type) {
	if (att == NULL || !PB_FightAtt::PropertyType_IsValid(type))
		return 0;
	return (int32_t)(att->properties(type) * (1.0f + att->propertiesDelta(type).percent())) + att->propertiesDelta(type).delta();
}
int32_t Fight_FinalProperty(const FightAtt *att, FightAtt::PropertyType type) {
	if (att == NULL || !FightAtt::PropertyType_IsValid(type))
		return 0;
	return (int32_t)(att->properties(type) * (1.0f + att->propertiesDelta(type).percent())) + att->propertiesDelta(type).delta();
}

int32_t Fight_FinalProperty(struct Fight *fight, FightAtt::PropertyType att) {
	if (!Fight_IsValid(fight) || !FightAtt::PropertyType_IsValid(att))
		return 0;
	return (int32_t)(fight->att->properties(att) * (1.0f + fight->att->propertiesDelta(att).percent())) + fight->att->propertiesDelta(att).delta();
}

static inline void AdjustHP(struct Fight *fight) {
	int32_t maxHP = Fight_FinalProperty(fight, FightAtt::MAXHP);
	if (fight->att->hp() != maxHP)
		fight->att->set_hp(maxHP);
}

void Fight_ModifyPropertyDelta(struct Fight *fight, FightAtt::PropertyType att, int32_t delta, float percent, bool adjust) {
	if (!Fight_IsValid(fight) || !FightAtt::PropertyType_IsValid(att) || (delta == 0 && percent == 0.0f))
		return;

	switch(att) {
		default:
			{
				fight->att->mutable_propertiesDelta(att)->set_delta(fight->att->propertiesDelta(att).delta() + delta);
				if (fight->att->propertiesDelta(att).delta() < 0)
					fight->att->mutable_propertiesDelta(att)->set_delta(0);
				fight->att->mutable_propertiesDelta(att)->set_percent(fight->att->propertiesDelta(att).percent() + percent);
				if (fight->att->propertiesDelta(att).percent() < 0)
					fight->att->mutable_propertiesDelta(att)->set_percent(0);
			}
			break;
	}

	if (adjust) {
		if (att == FightAtt::MAXHP)
			AdjustHP(fight);
	}
}

void Fight_ModifyProperty(struct Fight *fight, FightAtt::PropertyType att, int point, bool adjust) {
	if (!Fight_IsValid(fight) || !FightAtt::PropertyType_IsValid(att) || point == 0)
		return;

	switch(att) {
		default:
			{
				fight->att->set_properties(att, fight->att->properties(att) + point);
				if (fight->att->properties(att) < 0)
					fight->att->set_properties(att, 0);
			}
			break;
	}

	if (adjust) {
		if (att == FightAtt::MAXHP)
			AdjustHP(fight);
	}
}

static inline int64_t TotalExp(int32_t level) {
	if (level <= 0 || level >= (int32_t)package.totalExp.size())
		return -1;
	return package.totalExp[level];
}

void Fight_ModifyExp(struct Fight *fight, int64_t delta) {
	if (!Fight_IsValid(fight) || delta <= 0)
		return;

	int64_t totalExp = TotalExp(fight->att->level() + 1);
	if (totalExp == -1)
		return;

	fight->att->set_exp(fight->att->exp() + delta);

	if (delta != 0) {
		static NetProto_ModifyExp modifyExp;
		modifyExp.Clear();
		modifyExp.set_exp(fight->att->exp());
		int32_t id = PlayerEntity_ID(fight->component->player);
		GCAgent_SendProtoToClients(&id, 1, &modifyExp);
	}

	int levelDelta = 0;
	while (fight->att->exp() >= totalExp) {
		levelDelta++;
		totalExp = TotalExp(fight->att->level() + 1 + levelDelta);
		if (totalExp == -1)
			break;
	}

	if (levelDelta > 0) {
		ModifyLevel(fight, levelDelta, false);
	}
}

void Fight_SetFaction(struct Fight *fight, int32_t self, int32_t friendly) {
	if (!Fight_IsValid(fight))
		return;

	fight->att->set_selfFaction(self);
	fight->att->set_friendlyFaction(friendly);
}

void Fight_SetGodTarget(struct Fight *fight, int64_t target, int32_t power, int32_t score) {
	/*	if (!Fight_IsValid(fight))
		return;

		fight->att->set_godTarget(target);
		fight->godTargetPower = power;
		fight->godTargetScore = score;
		*/
}

int32_t Fight_GodTargetScore(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return 0;
	return fight->godTargetScore;
}

bool Fight_ResetCount(struct Fight *fight, NetProto_ResetCount::Type type, int arg) {
	if (!Fight_IsValid(fight))
		return false;

	switch(type) {
		case NetProto_ResetCount::SINGLE_ENHANCE:
			{
				const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
				int cost = 0;
				int index_, bit_, count_;
				{
					int events[] = {48, 49, 50, 51, 52, 53, 54, 55, 56};
					int index = (arg - Config_SingleEnhanceBegin()) / 10;
					if (index >= 0 && index < (int)(sizeof(events) / sizeof(events[0]))) {
						index = events[index];
						int bit = ((arg - Config_SingleEnhanceBegin()) % 10) * 3;
						int count = (fight->component->playerAtt->dayEvent(index) & (7 << bit)) >> bit;
						if (count >= vip->resetSingleEnhance())
							return false;
						cost = Config_ResetSingleEnhanceCost() * (count + 1);
						//						if (fight->component->playerAtt->itemPackage().rmb() < cost)
						if (!Item_HasRMB(fight->component->item, cost))
							return false;
						index_ = index;
						bit_ = bit;
						count_ = count;
					} else {
						return false;
					}
				}
				{
					int events[] = {32, 33, 36, 37, 38, 39, 40, 41};
					int index = (arg - Config_SingleEnhanceBegin()) / 15;
					if (index >= 0 && index < (int)(sizeof(events) / sizeof(events[0]))) {
						index = events[index];
						int bit = ((arg - Config_SingleEnhanceBegin()) % 15) * 2;
						int count = (fight->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
						if (count <= 0) {
							return false;
						} else {
							PlayerEntity_SetDayEvent(fight->component->player, index, (fight->component->playerAtt->dayEvent(index) & (~(3 << bit))) | ((count - 1) << bit));
							PlayerEntity_SetDayEvent(fight->component->player, index_, (fight->component->playerAtt->dayEvent(index_) & (~(7 << bit_))) | ((count_ + 1) << bit_));
						}
					} else {
						return false;
					}
				}
				Item_ModifyRMB(fight->component->item, -cost, false, 22, type, 0);
			}
			break;
		case NetProto_ResetCount::BOSS:
			{
				const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
				int cost = 0;
				int index_, bit_, count_;
				{
					int events[] = {57, 58, 59, 60, 61};
					int index = (arg - Config_BossBegin()) / 10;
					if (index >= 0 && index < (int)(sizeof(events) / sizeof(events[0]))) {
						index = events[index];
						int bit = ((arg - Config_BossBegin()) % 10) * 3;
						int count = (fight->component->playerAtt->dayEvent(index) & (7 << bit)) >> bit;
						if (count >= vip->resetBoss())
							return false;
						cost = Config_ResetSingleEnhanceCost() * (count + 1);
						//if (fight->component->playerAtt->itemPackage().rmb() < cost)
						if (!Item_HasRMB(fight->component->item, cost))
							return false;
						index_ = index;
						bit_ = bit;
						count_ = count;
					} else {
						return false;
					}
				}
				{
					int events[] = {32, 33, 36, 37, 38, 39, 40, 41};
					int index = (arg - Config_BossBegin()) / 15;
					if (index >= 0 && index < (int)(sizeof(events) / sizeof(events[0]))) {
						index = events[index];
						int bit = ((arg - Config_BossBegin()) % 15) * 2;
						int count = (fight->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
						if (count <= 0) {
							return false;
						} else {
							PlayerEntity_SetDayEvent(fight->component->player, index, (fight->component->playerAtt->dayEvent(index) & (~(3 << bit))) | ((count - 1) << bit));
							PlayerEntity_SetDayEvent(fight->component->player, index_, (fight->component->playerAtt->dayEvent(index_) & (~(7 << bit_))) | ((count_ + 1) << bit_));
						}
					} else {
						return false;
					}
				}
				Item_ModifyRMB(fight->component->item, -cost, false, 22, type, 0);
			}
			break;
		case NetProto_ResetCount::GOD:
			{
				const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
				int event = fight->component->playerAtt->dayEvent(35);
				int count = (event & 0xff0000) >> 16;;
				if (count >= vip->resetGod())
					return false;

				int cost = Config_ResetGodCost() * (count + 1);
				//if (fight->component->playerAtt->itemPackage().rmb() < cost)
				if (!Item_HasRMB(fight->component->item, cost))
					return false;
				Item_ModifyRMB(fight->component->item, -cost, false, 22, type, 0);

				PlayerEntity_SetDayEvent(fight->component->player, 29, fight->component->playerAtt->dayEvent(29) & (~0xffff));

				PlayerEntity_SetDayEvent(fight->component->player, 35, (event & (~0xff0000)) | ((count + 1) << 16));
			}
			break;
		case NetProto_ResetCount::GOD_CD:
			{
				const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
				if (!vip->clearGodCD())
					return false;

				int cost = Config_ClearGodCDCost();
				//if (fight->component->playerAtt->itemPackage().rmb() < cost)
				if (!Item_HasRMB(fight->component->item, cost))
					return false;
				Item_ModifyRMB(fight->component->item, -cost, false, 22, type, 0);

				PlayerEntity_ResetGodTime(fight->component->player);
			}
			break;
		case NetProto_ResetCount::QUICK_FIGHT_CD:
			{
				const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
				if (!vip->clearQuickFightCD())
					return false;

				int cost = Config_ClearQuickFightCDCost();
				if (cost > fight->component->playerAtt->itemPackage().money())
					return false;

				PlayerEntity_SetFixedEvent(fight->component->player, 56, 0);
				Item_ModifyMoney(fight->component->item, -cost);
			}
			break;
		default:
			return false;
	}
	return true;
}

void Fight_AddTower(struct Fight *fight) {
	int32_t mapID = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
	if (mapInfo != NULL) {
		if (mapInfo->id() >= Config_TowerBegin() && mapInfo->id() <= Config_TowerEnd()) {
			fight->att->set_curTower(mapInfo->id() - Config_TowerBegin() + 2);
			if (fight->att->curTower() - 1 > fight->att->maxTower())
				fight->att->set_maxTower(fight->att->curTower() - 1);
		}
	}
}
/*
   void Fight_DropTower(struct Fight *fight) {
   int32_t mapID = Movement_Att(fight->component->movement)->mapID();
   const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
   if (mapInfo != NULL) {
   if (mapInfo->id() >= Config_TowerBegin() && mapInfo->id() <= Config_TowerEnd()) {
   if (fight->att->curTower() > 0)
   fight->att->set_curTower((fight->att->curTower() - 1) / 5 * 5 + 1);
   }
   }
   }
   */
void Fight_AddSurvive(struct Fight *fight, int group) {
	int32_t mapID = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
	if (mapInfo != NULL) {
		if (mapInfo->id() == Config_SurviveMap()) {
			if (fight->att->maxSurvive() < group)
				fight->att->set_maxSurvive(group);
		}
	}
}

int64_t Fight_SkillLevelUp(struct Fight *fight, int32_t index, int32_t delta) {

	DEBUG_LOG("Fight_SkillLevelUp start");

	if (!Fight_IsValid(fight) || delta <= 0)
		return -1;

	if (index < 0 || index >= fight->att->skills_size())
		return -1;

	Skill *skill = fight->att->mutable_skills(index);
	if (skill->id() == -1)
		return -1;

	int nextLevel = skill->level() + delta;
	int maxLevel = SkillInfoManager_MaxLevel(skill->id());

	assert(maxLevel != -1);
	if (nextLevel > maxLevel) {	
		DEBUG_LOG(" loginfo: has max skill level (%d > %d), cur skillID  is %d", nextLevel, maxLevel, skill->id());
		return -1;
	}

	static map<int64_t, int> goodMapInfo;
	goodMapInfo.clear();
	for (int i = 1; i <= delta; i++) {
		const SkillInfo *info = SkillInfoManager_SkillInfo(skill->id(), skill->level() + i);
		if (info->requireLevel() > fight->att->level()) {
			DEBUG_LOG("loginfo: requirelevel is bigger than cur_level (%d > %d), skill id is %d", info->requireLevel(), fight->att->level(), skill->id());
			return -1;
		}

		if (info->goods() >= 0) {
			goodMapInfo[info->goods()] += info->count();
		}
	}

	for (map<int64_t, int>::iterator iter = goodMapInfo.begin(); iter != goodMapInfo.end(); ++iter) {
		if (!Item_HasItem(fight->component->item, ItemInfo::GOODS, iter->first, iter->second)) {
			DEBUG_LOG("loginfo: item num is not enough (id = %d, num = %d, skillID = %d)", iter->first, iter->second, skill->id());
			return -1;
		}
	}

	if (skill->level() <= 0) {
		int32_t emptyALT = Item_EmptyALT(fight->component->item);
		if (emptyALT != -1) {
			static ItemInfo item;
			item.set_type(ItemInfo::SKILL);
			item.set_id(index);
			Item_AddToALT(fight->component->item, &item, emptyALT);
		}
	}

	skill->set_level(nextLevel);

	for (map<int64_t, int>::iterator iter = goodMapInfo.begin(); iter != goodMapInfo.end(); ++iter) {
		Item_DelFromPackage(fight->component->item, ItemInfo::GOODS, iter->first, iter->second);	
	}

	DEBUG_LOG("Fight_SkillLevelUp end");

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *str = buf;

		SNPRINTF2(buf, str, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(fight->component->player));
		str += strlen(str);
		SNPRINTF2(buf, str, "\",\"roleName\":\"%s", fight->component->baseAtt->name());
		str += strlen(str);
		SNPRINTF2(buf, str, "\",\"skillID\":\"%d", fight->att->mutable_skills(index)->id());
		str += strlen(str);
		SNPRINTF2(buf, str, "\",\"skillLevel\":\"%d\"}", fight->att->mutable_skills(index)->level());
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "SkillLevel", buf));
	}

	return 0;
}

int Fight_ClearSkill(struct Fight *fight, int index) {
	if (!Fight_IsValid(fight))
		return -1;
	if (index < 0 || index >= fight->att->skills_size())
		return -1;

	Skill *skill = fight->att->mutable_skills(index);
	if (skill->id() == -1 || skill->level() <= 0)
		return -1;

	skill->set_level(0);
	Item_DelAllFromALT(fight->component->item, ItemInfo::SKILL, index);

	return 0;
}

int Fight_Attack(struct Fight *fight, const SkillInfo *skill, struct Fight *tFight, Vector3f *tPos, int64_t cur) {
	if (!Fight_IsValid(fight) || skill == NULL || fight == tFight) {
		return -1;
	}

	if (fight->att->status() == FightAtt::DEAD)
		return -2;

	if (fight->cantAttack)
		return -4;

	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(Movement_Att(fight->component->movement)->mapID()));
	if (mapInfo == NULL || mapInfo->mapType() == MapInfo::PEACE)
		return -14;

	if (skill->level() <= 0)
		return -17;

	if (fight->component->npc != NULL) {
		skill = Fight_NextSkill(fight, skill);
		if (skill == NULL)
			return -13;
		if (!Fight_IsCDOver(fight, skill, cur)) {
			DEBUG_LOGERROR("serverTime: %lld, clientTime: %lld", Time_ElapsedTime(), cur);
			return -3;
		}
	} else if (fight->component->player != NULL) {
		if (!Fight_IsCDOver(fight, skill, cur)) {
			// DEBUG_LOGERROR("serverTime: %lld, clientTime: %lld", Time_ElapsedTime(), cur);
			return -3;
		}
	} else {
		assert(0);
	}

	/*
	   if (skill->hp() >= fight->att->hp())
	   return -14;
	   if (skill->mp() > 0 && skill->mp() > fight->att->mana())
	   return -15;
	   */

	bool isNear = false;

	if (tFight != NULL) {
		if (tFight->att->status() == FightAtt::DEAD)
			return -6;

		if (!Movement_SameScene(fight->component->movement, tFight->component->movement))
			return -15;

		const MovementAtt *tAtt = Movement_Att(tFight->component->movement);
		// const Vector2i *tCoord = &tAtt->logicCoord();
		// if (MapInfoManager_Unwalkable(mapInfo, tCoord))
		// 	return -7;

		if (Status_HasType(tFight->component->status, StatusInfo::GOD))
			return -8;

		if (fight->component->player != NULL) {
			isNear = true;
		}
		else if (fight->component->npc != NULL) {
			const Vector3f *tPos = &tAtt->position();
			// const MovementAtt *att = Movement_Att(fight->component->movement);
			// float extra = (att->radius() + tAtt->radius()) * MAP_BLOCKSIZE;
			if (!IsNear(&Movement_Att(fight->component->movement)->position(), tPos, skill->dist()/* + extra*/)) {
				int followDelta = 500;
				if (fight->component->npc != NULL)
					followDelta = AI_Att(fight->component->ai)->followDelta();
				if (Movement_Follow(fight->component->movement, tFight->component->movement, skill->dist()/* + extra*/, followDelta) == -1) {
					return -9;
				}
			}
			else {
				isNear = true;
			}
		}
		else {
			assert(0);
		}
	}
	else if (tPos != NULL) {
		const MovementAtt *att = Movement_Att(fight->component->movement);
		const Vector3f *myPos = &Movement_Att(fight->component->movement)->position();
		// SkillEntity_FilterTargetPos(skill, myPos, tPos);

		static Vector2i tCoord;
		MapInfoManager_LogicCoordByPosition(tPos, &tCoord);
		// if (MapInfoManager_Unwalkable(mapInfo, &tCoord))
		// 	return -10;

		float extra = att->radius() * MAP_BLOCKSIZE;
		if (!IsNear(myPos, tPos, skill->dist() + extra)) {
			/*
			   static Vector2i coord;
			   MapInfoManager_LogicCoordByPosition(tPos, &coord);

			   if (Movement_MoveStraightly(fight->component->movement, &coord) == -1) {
			   if (Movement_MoveByAStar(fight->component->movement, &coord) == -1) {
			   return -1;
			   }
			   }
			   */

			// return -11;
			isNear = true;
		}
		else {
			isNear = true;
		}
	}
	else {
		return -12;
	}

	fight->att->set_status(FightAtt::MOVE);
	fight->curSkill = skill;
	fight->tFight = tFight;
	if (tPos != NULL) {
		fight->tPos.set_x(tPos->x());
		fight->tPos.set_y(0.0f);
		fight->tPos.set_z(tPos->z());
	}

	if (isNear) {
		Movement_Stop(fight->component->movement);
		return 1;
	}

	return 0;
}

static void DieInPVP(struct Fight *fight, struct Fight *killer, int mapID) {
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
	if (mapInfo->id() == Config_FactionWarMap()) {
		if (fight->component->npc != NULL) {
			int npcID = NPCEntity_ID(fight->component->npc);
			const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
			if (mapUnit != NULL && npcID == mapUnit->boss()) {
				MapPool_ClearFactionWar();
			}
		}
	} else if (mapInfo->mapType() == MapInfo::PRACTICE || mapInfo->mapType() == MapInfo::PVP) {
		if (fight->component->player != NULL) {
			if (mapID == PlayerPool_ReservationMap()) {
				PlayerPool_ReservationRefresh(-4, PlayerEntity_RoleID(fight->component->player));
			}
			Fight_ClearRoom(fight, false, 0);
		} else if (fight->component->npc != NULL && NPCEntity_Master(fight->component->npc) == NULL) {
			if (killer->component->player != NULL) {
				Fight_ClearRoom(killer, true, 0);
			} else if (killer->component->npc != NULL) {
				struct Component *master = NPCEntity_Master(killer->component->npc);
				if (Component_IsValid(master))
					Fight_ClearRoom(master->fight, true, 0);
			}
		}
		if(fight->component->npc != NULL && NPCEntity_Master(fight->component->npc) == NULL)
			MapPool_ReleaseNPCs(mapID, NPCEntity_ID(fight->component->npc), true);
	} else if (mapInfo->mapType() == MapInfo::HELL) {
		fight->dieCount++;
		if (Fight_IsValid(killer) && killer->component->player != NULL) {
			if (killer->component->npc != NULL) {
				struct Component *master = NPCEntity_Master(killer->component->npc);
				if (Component_IsValid(master))
					killer = master->fight;
			}
			if (Config_IsHellOpen(Time_TimeStamp())) {
				killer->killCount++;
			} else {
				if (killer->component->player != NULL) {
					int count = (killer->component->playerAtt->dayEvent(29) & 0x7fff0000) >> 16;
					if (count < Config_HellMaxKill()) {
						killer->killCount++;
						PlayerEntity_SetDayEvent(killer->component->player, 29, ((count + 1) << 16) | (killer->component->playerAtt->dayEvent(29) & 0xffff));
					}
				}
			}
			Mission_CompleteTarget(killer->component->mission, MissionTarget::HELL_KILL, 0, 0);
		}
		if (fight->component->npc != NULL && NPCEntity_Master(fight->component->npc) == NULL) {
			if (fight->dieCount >= Config_MaxHellNPCDie()) {
				MapPool_ReleaseNPCs(mapID, NPCEntity_ID(fight->component->npc), true);
			} else {
				static vector<struct PlayerEntity *> players;
				players.clear();
				int playerCount = PlayerPool_Players(fight->component->roleAtt->movementAtt().mapID(), &players);
				int minPower = INT_MAX;
				for (int i = 0; i < playerCount; i++) {
					struct Component *component = PlayerEntity_Component(players[i]);
					if (component != NULL) {
						int power = Fight_Power(component->fight);
						if (power < minPower) {
							minPower = power;
						}
					}
				}
				if (minPower != INT_MAX) {
					if (Fight_Power(fight) > minPower) {
						MapPool_ReleaseNPCs(mapID, NPCEntity_ID(fight->component->npc), true);
					}
				}
			}
		}
	}
}

static void ClearWorldBoss(struct Fight *fight, struct Fight *killer) {
	multimap<int32_t, int64_t> rank;
	for (map<int64_t, int32_t>::iterator it = fight->enemies.begin(); it != fight->enemies.end(); it++)
		rank.insert(make_pair(it->second, it->first));
	int index = (int)rank.size() - 1;
	for (multimap<int32_t, int64_t>::iterator it = rank.begin(); it != rank.end(); it++) {
		int range = Config_AwardRange(index);
		const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::WORLD_BOSS, range);
		if (award == NULL) {
			DEBUG_LOGERROR("Failed to get worldboss award, range: %d", range);
			continue;
		}
		struct PlayerEntity *entity = PlayerEntity_PlayerByRoleID(it->second);
		if (entity != NULL) {
			PlayerEntity_AddMail(entity, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());
		} else {
			static DCProto_SendMail dsm;
			dsm.Clear();
			dsm.set_id(-1);
			dsm.mutable_sm()->set_receiver(it->first);
			dsm.mutable_sm()->mutable_mail()->set_title(award->name());
			dsm.mutable_sm()->mutable_mail()->set_sender(Config_Words(1));
			dsm.mutable_sm()->mutable_mail()->set_content(award->content());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_type(PB_ItemInfo::GOODS);
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_id(award->award());
			dsm.mutable_sm()->mutable_mail()->mutable_item()->set_count(1);
			GCAgent_SendProtoToDCAgent(&dsm);
		}
		index--;
	}

	static DCProto_InitRank proto;
	proto.Clear();
	proto.set_type(NetProto_Rank::WORLD_BOSS);
	proto.set_flag(true);
	proto.mutable_finalKiller()->mutable_role()->set_roleID(-1);
	proto.mutable_finalKiller()->set_arg1(MapPool_WorldBossNum());

	if (Fight_IsValid(killer)) {
		const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::WORLD_BOSS_LAST, 0);
		assert(award != NULL);
		PlayerEntity_AddMail(killer->component->player, ItemInfo::GOODS, award->award(), 1, award->name().c_str(), award->content().c_str());

		static NetProto_Message message;
		message.Clear();
		char buf[CONFIG_FIXEDARRAY];
		SNPRINTF1(buf, Config_Words(36), killer->component->baseAtt->name());
		message.set_content(buf);
		message.set_count(1);
		GCAgent_SendProtoToAllClients(&message);

		if (killer->component->player != NULL) {
			PlayerEntity_SetFixedEvent(killer->component->player, 31, killer->component->playerAtt->fixedEvent(31) | (1 << 16));
		} else if (killer->component->npc != NULL) {
			struct Component *master = NPCEntity_Master(killer->component->npc);
			if (Component_IsValid(master)) {
				PlayerEntity_SetFixedEvent(master->player, 31, master->playerAtt->fixedEvent(31) | (1 << 16));
			}
		}

		RecordInfo *finalKiller = proto.mutable_finalKiller();
		finalKiller->set_arg1(MapPool_WorldBossNum());
		finalKiller->mutable_role()->set_roleID(killer->component->baseAtt->roleID());
		finalKiller->mutable_role()->set_name(killer->component->baseAtt->name());
		finalKiller->mutable_role()->set_professionType((PB_ProfessionInfo::Type)killer->component->baseAtt->professionType());
	}

	GCAgent_SendProtoToDCAgent(&proto);
}

static void ClearFactionBoss(struct Fight *fight, struct Fight *killer) {
	const char *faction = MapPool_FactionOfBoss(fight->component->roleAtt->movementAtt().mapID());
	if (faction == NULL)
		return;

	int exp, item, contrib;
	Config_FactionBossAward(&exp, &item, &contrib);

	FactionPool_AddExpAndItem(faction, exp, item);

	int64_t roles[CONFIG_FIXEDARRAY];
	int count = 0;
	for (map<int64_t, int32_t>::iterator it = fight->enemies.begin(); it != fight->enemies.end(); it++) {
		roles[count++] = it->first;
	}
	if (count > 0) {
		FactionPool_AddMemContribute(faction, contrib, roles, count);
	}
}

void Fight_Die(struct Fight *fight, struct Fight *killer) {
	if (!Fight_IsValid(fight))
		return;

	if (fight->att->status() == FightAtt::DEAD)
		return;

	// DONT MOVE THESE LINES!
	fight->att->set_hp(0);
	fight->att->set_status(FightAtt::DEAD);
	fight->deadTime = Time_ElapsedTime();

	Fight_Cancel(fight);
	Fight_Transform(fight, false);
	Fight_SetSpecial(fight, false);
	Fight_ClearSkills(fight);
	Status_Clear(fight->component->status);
	AI_Idle(fight->component->ai);
	Fight_SetCantBeAttacked(fight, true);
	Fight_SetCantAttack(fight, true);

	static NetProto_Die die;
	die.Clear();
	if (fight->component->player != NULL) {
		die.set_type(NetProto_Die::PLAYER);
		die.set_id(PlayerEntity_ID(fight->component->player));
	} else if (fight->component->npc != NULL) {
		die.set_type(NetProto_Die::NPC);
		die.set_id(NPCEntity_ID(fight->component->npc));
	} else {
		assert(0);
	}

	if (Fight_IsValid(killer)) {
		if (killer->component->player != NULL) {
			die.set_mType(NetProto_Die::PLAYER);
			die.set_mID(PlayerEntity_ID(killer->component->player));
		} else if (killer->component->npc != NULL) {
			die.set_mType(NetProto_Die::NPC);
			die.set_mID(NPCEntity_ID(killer->component->npc));
		} else {
			assert(0);
		}
	} else {
		DEBUG_LOGERROR("Failed to find killer");
		die.set_mID(-1);
	}

	int32_t mapID = Movement_Att(fight->component->movement)->mapID();
	GCAgent_SendProtoToAllClientsInSameScene(mapID, fight->component->player, &die);



	/*
	   ShareExp(fight);

	   const DropList *dropList = GenDropList(fight);
	   if (dropList != NULL) {
	   static NetProto_GenDropList genDropList;
	   genDropList.Clear();
	   dropList->ToPB(genDropList.add_dropList());
	   GCAgent_SendProtoToAllClientsInSameScene(fight->component->player, &genDropList);
	   }
	   */

	//Fight_ClearEnemy(fight);

	if (fight->component->player != NULL) {
		//Fight_DropTower(fight);

		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
		if (mapInfo != NULL) {
			if (mapInfo->mapType() == MapInfo::PRACTICE
					|| mapInfo->mapType() == MapInfo::PVP
					|| mapInfo->mapType() == MapInfo::HELL) {
				DieInPVP(fight, killer, mapID);
				if (Config_ScribeHost() != NULL) {
					static char buf[CONFIG_FIXEDARRAY];
					memset(buf, 0, sizeof(buf) / sizeof(char));
					char *index = buf;

					SNPRINTF2(buf, index, "{\"dieLevel\":\"%d", fight->component->roleAtt->fightAtt().level());
					index += strlen(index);
					if (Fight_IsValid(killer)) {
						SNPRINTF2(buf, index, "\",\"killerLevel\":\"%d", killer->component->roleAtt->fightAtt().level());
						index += strlen(index);
					}
					SNPRINTF2(buf, index, "\",\"mapID\":\"%d", mapInfo->id());
					index += strlen(index);
					SNPRINTF2(buf, index, "\"}");
					ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "DieInPvp", buf));
				}
			} else if (mapInfo->mapType() == MapInfo::SINGLE || mapInfo->mapType() == MapInfo::ROOM || mapInfo->mapType() == MapInfo::ONLY_ROOM) {
				if (Config_ScribeHost() != NULL) {
					static char buf[CONFIG_FIXEDARRAY];
					memset(buf, 0, sizeof(buf) / sizeof(char));
					char *index = buf;

					SNPRINTF2(buf, index, "{\"dieLevel\":\"%d", fight->component->roleAtt->fightAtt().level());
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"mapID\":\"%d", mapInfo->id());
					index += strlen(index);
					SNPRINTF2(buf, index, "\"}");
					ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "DieInInstance", buf));
				}
			}
		}

		struct NPCEntity *pet = NPCPool_Pet(mapID, fight->component);
		if (pet != NULL) {
			struct Component *petCom = NPCEntity_Component(pet);
			if (petCom != NULL) {
				if (petCom->fight->att->status() != FightAtt::DEAD) {
					Fight_Die(petCom->fight, killer);
					fight->reviveWithPet = true;
				} else {
					fight->reviveWithPet = false;
				}
			}
		}
	} else if (fight->component->npc != NULL) {
		if (Fight_IsValid(killer) && killer->component->player != NULL)
			Mission_CompleteTarget(killer->component->mission, MissionTarget::KILLNPC, NPCEntity_ResID(fight->component->npc), -1);

		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
		if (mapInfo != NULL) {
			if (mapInfo->mapType() == MapInfo::ONLY_ROOM
					|| (mapInfo->mapType() == MapInfo::ROOM && !MapPool_IsSingle(mapID))) {
				const MapUnit *unit = MapInfoManager_MapUnit(mapInfo);
				int32_t npcID = NPCEntity_ID(fight->component->npc);
				if (npcID != -1 && unit->boss() == npcID) {
					if (mapID == MapPool_WorldBoss()) {
						ClearWorldBoss(fight, killer);
					} else if (mapInfo->id() == Config_FactionBossMap()) {
						ClearFactionBoss(fight, killer);
					}
					MapPool_ClearRoom(mapID);
				} else if (npcID != -1 && unit->protectNPC() == npcID) {
					MapPool_ClearRoom(mapID, false);
				} else if (NPCEntity_Master(fight->component->npc) != NULL){
					// pet can revive
				} else {
					Component_Destroy(fight->component);
				}
			} else if (mapInfo->mapType() == MapInfo::PVP
					|| mapInfo->mapType() == MapInfo::PRACTICE
					|| mapInfo->mapType() == MapInfo::HELL) {
				DieInPVP(fight, killer, mapID);
			}
		}

	}
	Fight_ClearEnemy(fight);
}

void Fight_RevivePetInSingle(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;

	int32_t curMap = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(curMap));
	if (mapInfo == NULL || mapInfo->mapType() == MapInfo::PEACE)
		return;
	if (!Movement_InSingle(fight->component->movement))
		return;

	if (fight->component->player != NULL) {
		if (fight->petReviveCount >= Config_PetReviveCount())
			return;
		if (!Item_HasItem(fight->component->item, ItemInfo::GOODS, Config_PetReviveGoods(), 1))
			return;
		Item_DelFromPackage(fight->component->item, ItemInfo::GOODS, Config_PetReviveGoods(), 1);
		fight->petReviveCount++;

		static NetProto_Revive revive;
		revive.Clear();
		revive.set_type(NetProto_Revive::PET);
		revive.set_id(PlayerEntity_ID(fight->component->player));
		GCAgent_SendProtoToAllClientsInSameScene(Movement_Att(fight->component->movement)->mapID(), fight->component->player, &revive);
	}
}

void Fight_Revive(struct Fight *fight, const Vector2i *coord, float hp, float mana, bool free) {
	if (!Fight_IsValid(fight) || coord == NULL)
		return;

	int32_t curMap = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(curMap));
	if (mapInfo == NULL || mapInfo->mapType() == MapInfo::PEACE)
		return;

	if (fight->component->player != NULL) {
		if (mapInfo->id() == Config_WorldBossMap()
				|| mapInfo->id() == Config_FactionBossMap()) {
		} else if (mapInfo->id() == Config_FactionWarMap()
				|| mapInfo->mapType() == MapInfo::HELL) {
			static Vector2i newCoord;
			MapInfoManager_EnterCoord(mapInfo, MapInfoManager_RandomEnterCoord(mapInfo), &newCoord);
			coord = &newCoord;
		} else {
			if (fight->reviveCount >= Config_FreeReviveCount()) {
				const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
				//int delta = fight->reviveCount - Config_FreeReviveCount();
				int delta = fight->reviveCount - Config_ReviveBaseCount();
				if (delta >= vip->reviveDelta())
					return;
				int cost = 0;
				if(delta == -1)
					cost = Config_ReviveFirst();
				else if(delta >= 0)
					cost = Config_ReviveCost() * (delta + 1);
				//if (fight->component->playerAtt->itemPackage().rmb() < cost)
				if (!Item_HasRMB(fight->component->item, cost))
					return;
				Item_ModifyRMB(fight->component->item, -cost, false, 17, 0, 0);
			}
			fight->reviveCount++;
		}

		if (fight->reviveWithPet) {
			struct NPCEntity *pet = NPCPool_Pet(curMap, fight->component);
			if (pet != NULL) {
				struct Component *petCom = NPCEntity_Component(pet);
				if (petCom != NULL && petCom->fight->att->status() == FightAtt::DEAD) {
					Fight_Revive(petCom->fight, &petCom->roleAtt->movementAtt().logicCoord(), hp, mana, true);
				}
			}
		}
	} else if (fight->component->npc != NULL) {
		struct Component *master = NPCEntity_Master(fight->component->npc);
		if (master != NULL) {
			if (master->player != NULL) {
				if (!free) {
					if (fight->reviveCount >= Config_PetReviveCount())
						return;
					if (!Item_HasItem(master->item, ItemInfo::GOODS, Config_PetReviveGoods(), 1))
						return;
					Item_DelFromPackage(master->item, ItemInfo::GOODS, Config_PetReviveGoods(), 1);
					fight->reviveCount++;
				}
			} else {
				return;
			}
		}
	}

	if (!Movement_InSingle(fight->component->movement)) {
		int32_t maxHP = Fight_FinalProperty(fight, FightAtt::MAXHP);
		fight->att->set_hp((int32_t)(maxHP * hp));
		if (fight->att->hp() > maxHP)
			fight->att->set_hp(maxHP);
		else if (fight->att->hp() <= 0)
			fight->att->set_hp(1);

		int32_t maxMana = Config_MaxMana();
		fight->att->set_mana((int32_t)(maxMana * mana));
		if (fight->att->mana() > maxMana)
			fight->att->set_mana(maxMana);
		else if (fight->att->mana() <= 0)
			fight->att->set_mana(1);

		fight->att->set_status(FightAtt::IDLE);
		Fight_SetCantBeAttacked(fight, false);
		Fight_SetCantAttack(fight, false);
		Fight_SetSpecial(fight, false);
		InspireEffect(fight);

		Vector3f pos;
		MapInfoManager_PositionByLogicCoord(coord, &pos);
		Movement_SetPosition(fight->component->movement, &pos);
	}

	static NetProto_Revive revive;
	revive.Clear();
	if (fight->component->player != NULL) {
		revive.set_type(NetProto_Revive::PLAYER);
		revive.set_id(PlayerEntity_ID(fight->component->player));
	} else if (fight->component->npc != NULL) {
		struct Component *master = NPCEntity_Master(fight->component->npc);
		if (master != NULL) {
			revive.set_type(NetProto_Revive::PET);
			revive.set_id(PlayerEntity_ID(master->player));
		} else {
			revive.set_type(NetProto_Revive::NPC);
			revive.set_id(NPCEntity_ID(fight->component->npc));
		}
	} else {
		assert(0);
	}
	coord->ToPB(revive.mutable_coord());
	GCAgent_SendProtoToAllClientsInSameScene(Movement_Att(fight->component->movement)->mapID(), fight->component->player, &revive);
}

static int FinalTransformTime(struct Fight *fight) {
	return Config_TransformTime() + fight->transformTimeDelta;
}

static const TransformInfo * CurTransformInfo(struct Fight *fight) {
	int quality = 0, level = 0;
	int id = fight->att->transformID();
	if (fight->component->player != NULL) {
		for (int i = 0; i < fight->component->playerAtt->itemPackage().transforms_size(); i++) {
			const TransformAsset *asset = &fight->component->playerAtt->itemPackage().transforms(i);
			if (asset->id() != -1 && asset->id() == id) {
				quality = asset->quality();
				level = asset->level();
				break;
			}
		}
	}
	return TransformInfoManager_TransformInfo(id, quality, level);
}

int Fight_Transform(struct Fight *fight, bool transform) {
	if (!Fight_IsValid(fight))
		return -1;

	if (!fight->isTransforming && transform) {
		if (fight->att->status() == FightAtt::DEAD)
			return -1;

		fight->isTransforming = true;
		Fight_SetSpecial(fight, true);
		PlayerEntity_SetFixedEvent(fight->component->player, 8, 1);
		fight->prevTransformTime = Time_ElapsedTime();

		const TransformInfo *info = CurTransformInfo(fight);
		if (info != NULL) {
			for (int i = 0; i < info->atts_size(); i++) {
				Fight_ModifyProperty(fight, (FightAtt::PropertyType)i, info->atts(i), false);
			}
			Fight_ModifyHP(fight, NULL, info->atts(FightAtt::MAXHP));

			if (fight->component->baseAtt->professionType() > 0) {
				int skill = info->skill(fight->component->baseAtt->professionType() - 1);
				if (skill != -1) {
					fight->att->mutable_skills(0)->set_id(skill);
				}

				int status = info->status(fight->component->baseAtt->professionType() - 1);
				if (status != -1) {
					const StatusInfo *statusInfo = StatusInfoManager_StatusInfo(status);
					bool resistControl;
					if (StatusEntity_Create(fight->component->status, fight->component->status, NULL, StatusInfo::ATTACKER, statusInfo, NULL, &fight->transformStatus, 1, 0, &resistControl) == -1) {
						// DEBUG_LOGERROR("Failed to create status");
					}
				}
			}
		}

		if (fight->component->npc != NULL) {
			static NetProto_Transform proto;
			proto.set_type(NetProto_Transform::NPC);
			proto.set_id(NPCEntity_ID(fight->component->npc));
			GCAgent_SendProtoToAllClientsInSameScene(fight->component->roleAtt->movementAtt().mapID(), &proto);
		}
	} else if (fight->isTransforming && !transform) {
		fight->isTransforming = false;
		Fight_SetSpecial(fight, false);
		PlayerEntity_SetFixedEvent(fight->component->player, 8, 0);
		fight->prevTransformTime = Time_ElapsedTime();

		const TransformInfo *info = CurTransformInfo(fight);
		if (info != NULL) {
			int maxHP = Fight_FinalProperty(fight, FightAtt::MAXHP);
			float percent = (float)fight->att->hp() / (float)maxHP;
			for (int i = 0; i < info->atts_size(); i++) {
				Fight_ModifyProperty(fight, (FightAtt::PropertyType)i, -info->atts(i), false);
			}
			if (fight->att->status() != FightAtt::DEAD) {
				maxHP = Fight_FinalProperty(fight, FightAtt::MAXHP);
				int hp = (int)(maxHP * percent);
				if (hp <= 0)
					hp = 1;
				if (fight->att->hp() != hp) {
					Fight_ModifyHP(fight, NULL, hp - fight->att->hp());
					if (fight->component->player != NULL) {
						static NetProto_ModifyHP proto;
						int32_t player = PlayerEntity_ID(fight->component->player);
						proto.set_id(player);
						proto.set_hp(fight->att->hp());
						GCAgent_SendProtoToClients(&player, 1, &proto);
					}
				}
			}

			if (fight->component->baseAtt->professionType() > 0) {
				int skill = info->skill(fight->component->baseAtt->professionType() - 1);
				if (skill != -1) {
					AdjustOriginSkill(fight);
				}

				if (fight->transformStatus != NULL) {
					StatusEntity_Destroy(fight->transformStatus);
					fight->transformStatus = NULL;
				}
			}
		}
	} else {
		return -1;
	}
	return 0;
}

bool Fight_IsTransforming(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return false;

	return fight->isTransforming;
}

int Fight_AddBloodNode(struct Fight *fight, NetProto_AddBloodNode::Type type) {
	if (!Fight_IsValid(fight))
		return -1;
	if (fight->component->player == NULL)
		return -1;

	if (fight->att->bloodNode() >= BloodInfoManager_MaxBloodNode())
		return -1;

	/*
	   const BloodInfo *bloodInfo = BloodInfoManager_BloodInfo(fight->att->bloodLevel());
	   if (bloodInfo == NULL)
	   return -1;
	   if (fight->att->bloodNode() >= bloodInfo->lastNode() + 1)
	   return -1;
	   */
	if (fight->att->bloodNode() >= BloodInfoManager_MaxBloodNode())
		return -1;

	switch(type) {
		case NetProto_AddBloodNode::NORMAL:
			{
				const BloodNodeInfo *bloodNode = BloodInfoManager_BloodNodeInfo(fight->att->bloodNode());
				assert(bloodNode != NULL);

				if (!Item_HasItem(fight->component->item, ItemInfo::GOODS, bloodNode->goods(), bloodNode->count())) {
					DEBUG_LOG("Fight_AddBloodNode: NORMAL: item has not enough");
					return -1;
				}
				Item_DelFromPackage(fight->component->item, ItemInfo::GOODS, (int64_t)bloodNode->goods(), bloodNode->count());
				if (!Time_CanPass(bloodNode->successRate()))
					return -2;

				AddBloodNodeEffect(fight, fight->att->bloodNode());
				fight->att->set_bloodNode(fight->att->bloodNode() + 1);
			}
			break;
		case NetProto_AddBloodNode::PERFECT:
			{
				const BloodNodeInfo *bloodNode = BloodInfoManager_BloodNodeInfo(fight->att->bloodNode());
				assert(bloodNode != NULL);

				if (!Item_HasItem(fight->component->item, ItemInfo::GOODS, bloodNode->goods(), bloodNode->count()) || !Item_HasRMB(fight->component->item, bloodNode->requiredGem())) {
					DEBUG_LOG("Fight_AddBloodNode: PERFECT : item or RMB has not enough");
					return -1;
				}
				Item_DelFromPackage(fight->component->item, ItemInfo::GOODS, (int64_t)bloodNode->goods(), bloodNode->count());				
				Item_ModifyRMB(fight->component->item, -bloodNode->requiredGem(), false, 1, fight->att->bloodNode(), 0);

				AddBloodNodeEffect(fight, fight->att->bloodNode());
				fight->att->set_bloodNode(fight->att->bloodNode() + 1);
			}
			break;
		default:
			return -1;
	}

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		char *index = buf;
		memset(buf, 0, sizeof(buf) / sizeof(char));

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(fight->component->player));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", fight->component->baseAtt->name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"bloodNode\":\"%d\"}", fight->att->bloodNode());
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "BloodNode", buf));
	}
	Mission_CompleteTarget(fight->component->mission, MissionTarget::UNLOCK_BLOODNODE, -1, -1);
	return 0;
}

int32_t Fight_AddBloodEffect(struct Fight *fight, int soul, bool all, int *count) {
	if (!Fight_IsValid(fight) || soul < 0 || soul >= Config_SoulCount() || count == NULL)
		return -1;

	// for (int i = 0; i <= fight->att->bloodLevel(); i++) {
	for (int i = 0; i < BloodInfoManager_MaxBloodLevel(); i++) {
		const BloodInfo *bloodInfo = BloodInfoManager_BloodInfo(i);
		BloodDelta *delta = fight->att->mutable_bloodDelta(i);
		if (delta->toAtk() < bloodInfo->limit()
				|| delta->toDef() < bloodInfo->limit()
				|| delta->toDodge() < bloodInfo->limit()
				|| delta->toAccuracy() < bloodInfo->limit()) {
			bool add = false;
			int cur = 0;
			if (soul == 0 && delta->toAtk() < bloodInfo->limit()) {
				add = true;
				cur = delta->toAtk();
			} else if (soul == 1 && delta->toDef() < bloodInfo->limit()) {
				add = true;
				cur = delta->toDef();
			} else if (soul == 2 && delta->toDodge() < bloodInfo->limit()) {
				add = true;
				cur = delta->toDodge();
			} else if (soul == 3 && delta->toAccuracy() < bloodInfo->limit()) {
				add = true;
				cur = delta->toAccuracy();
			}

			if (!add)
				return -1;

			int value = 0;
			const ItemPackage *itemPackage = Item_Package(fight->component->item);
			if (itemPackage->soulStone() >= bloodInfo->requiredMoney()) {
				if (all) {
					*count = bloodInfo->limit() - cur;
					int32_t cost = *count * bloodInfo->requiredMoney();
					if (cost > itemPackage->soulStone()) {
						*count = itemPackage->soulStone() / bloodInfo->requiredMoney();
						cost = *count * bloodInfo->requiredMoney();
					}
					Item_ModifySoulStone(fight->component->item, -cost);
					value = *count;
				} else {
					Item_ModifySoulStone(fight->component->item, -bloodInfo->requiredMoney());
					value = 1;
				}
			} else {
				return -1;
			}
			assert(value > 0);

			if (soul == 0) {
				delta->set_toAtk(delta->toAtk() + value);
				if (delta->toAtk() > bloodInfo->limit()) {
					value -= delta->toAtk() - bloodInfo->limit();
					delta->set_toAtk(bloodInfo->limit());
				}
				AddBloodEffect(fight, i, FightAtt::ATK, value);
			} else if (soul == 1) {
				delta->set_toDef(delta->toDef() + value);
				if (delta->toDef() > bloodInfo->limit()) {
					value -= delta->toDef() - bloodInfo->limit();
					delta->set_toDef(bloodInfo->limit());
				}
				AddBloodEffect(fight, i, FightAtt::DEF, value);
			} else if (soul == 2) {
				delta->set_toDodge(delta->toDodge() + value);
				if (delta->toDodge() > bloodInfo->limit()) {
					value -= delta->toDodge() - bloodInfo->limit();
					delta->set_toDodge(bloodInfo->limit());
				}
				AddBloodEffect(fight, i, FightAtt::DODGE, value);
			} else if (soul == 3) {
				delta->set_toAccuracy(delta->toAccuracy() + value);
				if (delta->toAccuracy() > bloodInfo->limit()) {
					value -= delta->toAccuracy() - bloodInfo->limit();
					delta->set_toAccuracy(bloodInfo->limit());
				}
				AddBloodEffect(fight, i, FightAtt::ACCURACY, value);
			}

			return value;
		}
	}

	return -1;
}

static void NoticeBlood(struct Fight *fight) {
	if (!Config_CanNoticeBlood(fight->att->bloodLevel()))
		return;

	static NetProto_Message message;
	message.Clear();
	char buf[CONFIG_FIXEDARRAY];
	const BloodInfo *info = BloodInfoManager_BloodInfo(fight->att->bloodLevel());
	SNPRINTF1(buf, Config_Words(3), fight->component->baseAtt->name(), fight->att->bloodLevel(), info->name().c_str());
	message.set_content(buf);
	message.set_count(1);
	GCAgent_SendProtoToAllClients(&message);
}

int32_t Fight_UnlockBlood(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return -1;
	if (fight->att->bloodLevel() >= BloodInfoManager_MaxBloodLevel() - 1)
		return -1;

	/*
	   const BloodInfo *curBlood = BloodInfoManager_BloodInfo(fight->att->bloodLevel());
	   if (curBlood->lastNode() + 1 != fight->att->bloodNode())
	   return -1;
	   */

	const BloodInfo *nextBlood = BloodInfoManager_BloodInfo(fight->att->bloodLevel() + 1);
	const ItemPackage *package = Item_Package(fight->component->item);
	if (nextBlood->requiredLevel() > fight->att->level())
		return -1;

	if (nextBlood->soulJadeType() == ExploreInfo::SMALL) {
		if (nextBlood->soulJadeCount() > package->smallSoul())
			return -1;
	} else if (nextBlood->soulJadeType() == ExploreInfo::MEDIUM) {
		if (nextBlood->soulJadeCount() > package->mediumSoul())
			return -1;
	} else if (nextBlood->soulJadeType() == ExploreInfo::BIG) {
		if (nextBlood->soulJadeCount() > package->bigSoul())
			return -1;
	} else if (nextBlood->soulJadeType() == ExploreInfo::PERFECT) {
		if (nextBlood->soulJadeCount() > package->perfectSoul())
			return -1;
	}

	Item_ModifySoulJade(fight->component->item, nextBlood->soulJadeType(), -nextBlood->soulJadeCount());
	fight->att->set_bloodLevel(fight->att->bloodLevel() + 1);
	NoticeBlood(fight);
	return 0;
}

int32_t Fight_SkillAreaDelta(struct Fight *fight, int32_t id) {
	if (!Fight_IsValid(fight))
		return 0;
	if (fight->component->player == NULL)
		return 0;

	// if (!fight->isTransforming)
	//	return 0;

	map<int32_t, int32_t>::iterator it = fight->skillAreaDelta.find(id);
	return it == fight->skillAreaDelta.end() ? 0 : it->second;
}

int32_t Fight_SkillIncMana(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return 0;
	if (fight->component->player == NULL)
		return 0;

	return fight->isTransforming ? fight->skillIncMana : 0;
}

int32_t Fight_SkillEnergyDelta(struct Fight *fight, int32_t id) {
	if (!Fight_IsValid(fight))
		return 0;
	if (fight->component->player == NULL)
		return 0;

	map<int32_t, int32_t>::iterator it = fight->skillEnergyDelta.find(id);
	return it == fight->skillEnergyDelta.end() ? 0 : it->second;
}

static void UpdateTransform(struct Fight *fight) {
	if (fight->isTransforming) {
		int64_t cur = Time_ElapsedTime();
		if (cur - fight->prevTransformTime < FinalTransformTime(fight))
			return;

		Fight_Transform(fight, false);
	}
}

void Fight_EnterScene(struct Fight *fight, const MapInfo *info) {
	if (!Fight_IsValid(fight))
		return;

	fight->clearRoom = false;
	fight->openRoomFreeBox = false;
	fight->openRoomGemBox = false;
	fight->recoverCount = 0;
	fight->enterRoomTime = Time_ElapsedTime();
	fight->reviveCount = 0;
	fight->petReviveCount = 0;
	fight->killCount = 0;
	fight->dieCount = 0;
	fight->reviveWithPet = false;

	Fight_SetSpecial(fight, false);
	InspireEffect(fight);
	AdjustOriginSkill(fight);
	fight->npcID.clear();

	// playofftest
	/*
	   if (!fight->change) {
	   if (info != NULL && (info->id() == Config_PlayOffMap() || info->id() == Config_GodMap())) {
	   fight->level = fight->att->level();
	   for (int i = 0; i < fight->att->properties_size(); i++)
	   fight->atts[i] = fight->att->properties(i);
	   for (int i = 0; i < fight->att->propertiesDelta_size(); i++)
	   fight->delta[i] = fight->att->propertiesDelta(i);
	   fight->att->set_level(100);
	   fight->att->set_properties(FightAtt::ATK, 20000);
	   fight->att->set_properties(FightAtt::DEF, 15000);
	   fight->att->set_properties(FightAtt::MAXHP, 1000000);
	   fight->att->set_properties(FightAtt::ACCURACY, 10000);
	   fight->att->set_properties(FightAtt::DODGE, 10000);
	   fight->att->set_properties(FightAtt::CRIT, 5000);
	   for (int i = 0; i < fight->att->propertiesDelta_size(); i++)
	 *fight->att->mutable_propertiesDelta(i) = FightPropertyDelta();
	 fight->att->set_hp(Fight_FinalProperty(fight, FightAtt::MAXHP));
	 fight->change = true;
	 }
	 }
	 */
}

static inline int32_t PassAward(int passCount, int32_t base) {
	return passCount * base + base * passCount * (passCount - 1) / 20;
}

static bool LoseIsWin(const struct PlayOffUnit *unit) {
	const PlayOff *info = PlayOffManager_PlayOff(unit->playoff);
	if (info->over(unit->day) == 2) {
		int begin = unit->day == 0 ? info->limit() : info->over(unit->day - 1);
		for (int i = 0; i <= unit->pass; i++)
			begin /= 2;
		if (begin == 2)
			return true;
	} else if (info->over(unit->day) == 1) {
		return true;
	}
	return false;
}

static void NoticeWinner(const char *account, const char *name, struct PlayOffUnit *unit, bool win) {
	if (unit == NULL)
		return;

	int begin, end;
	if (PlayOffManager_BeginAndEnd(unit->playoff, unit->day, unit->pass, &begin, &end)) {
		char buf[CONFIG_FIXEDARRAY];
		if (end == 0) {
			if (!win)
				return;
			SNPRINTF1(buf, Config_Words(58), name);
		} else if (end == 1) {
			if (win) {
				SNPRINTF1(buf, Config_Words(56), name);
			}
			else {
				SNPRINTF1(buf, Config_Words(57), name);
			}
		} else {
			if (!win)
				return;
			SNPRINTF1(buf, Config_Words(55), name, begin, end);
		}
		static NetProto_Message proto;
		proto.set_content(buf);
		proto.set_count(1);
		GCAgent_SendProtoToAllClients(&proto);
		// playofftest
		/*
		   FILE *file = fopen("PlayOffRecord.txt", "a");
		   if (file != NULL) {
		   char cur[32];
		   Time_ToDateTime((int)Time_TimeStamp(), cur);
		   char str[CONFIG_FIXEDARRAY];
		   SNPRINTF1(str, "[%s]%s.%s\n", cur, account == NULL ? "" : account, buf);
		   fputs(str, file);
		   fclose(file);
		   }
		   */
	}
}

static void ClearPVP(struct Fight *fight, bool win) {
	int32_t map = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
	if (mapInfo == NULL) {
		map = Movement_NextMap(fight->component->movement);
		mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
	}
	if (mapInfo->mapType() == MapInfo::HELL) {
		int honor = fight->killCount * Config_HellHonorBase();
		Item_ModifyHonor(fight->component->item, honor);
		int score = fight->killCount * Config_HellScoreBase();
		Item_ModifyPKScore(fight->component->item, score);

		static NetProto_Win proto;
		proto.Clear();
		proto.set_honor(honor);
		proto.set_pvpScore(score);
		int32_t id = PlayerEntity_ID(fight->component->player);
		GCAgent_SendProtoToClients(&id, 1, &proto);

		if (Config_ScribeHost() != NULL) {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;

			SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(fight->component->player));
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"roleName\":\"%s", fight->component->baseAtt->name());
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"killCount\":\"%d", fight->killCount);
			index += strlen(index);
			SNPRINTF2(buf, index, "\",\"hellScore\":\"%d\"}", score);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "Hell", buf));
		}
	} else if (mapInfo->id() == Config_PlayOffMap()) {
		struct PlayOffUnit *unit = MapPool_SelectPlayOff(fight->component->baseAtt->roleID());
		if (win) {
			static NetProto_Win proto;
			int32_t id = PlayerEntity_ID(fight->component->player);
			GCAgent_SendProtoToClients(&id, 1, &proto);
			if (unit != NULL) {
				if (fight->component->baseAtt->roleID() == unit->lhs)
					unit->lWin++;
				else
					unit->rWin++;
				// playofftest
				/*
				   FILE *file = fopen("PlayOffRecord.txt", "a");
				   if (file != NULL) {
				   char cur[32];
				   Time_ToDateTime((int)Time_TimeStamp(), cur);
				   char buf[CONFIG_FIXEDARRAY];
				   const PlayerInfo *account = AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player));
				   SNPRINTF1(buf, "[%s %d %s]%s.%s(%lld) win, day: %d, pass: %d\n", __FILE__, __LINE__, cur, account == NULL ? "" : account->account().c_str(), fight->component->baseAtt->name(), (long long)fight->component->baseAtt->roleID(), unit->day, unit->pass);
				   fputs(buf, file);
				   fclose(file);
				   }
				   */
			}
		} else {
			static NetProto_Lose proto;
			int32_t id = PlayerEntity_ID(fight->component->player);
			GCAgent_SendProtoToClients(&id, 1, &proto);
			// playofftest
			/*
			   if (unit != NULL) {
			   FILE *file = fopen("PlayOffRecord.txt", "a");
			   if (file != NULL) {
			   char cur[32];
			   Time_ToDateTime((int)Time_TimeStamp(), cur);
			   char buf[CONFIG_FIXEDARRAY];
			   const PlayerInfo *account = AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player));
			   SNPRINTF1(buf, "[%s %d %s]%s.%s(%lld) lose, day: %d, pass: %d\n", __FILE__, __LINE__, cur, account == NULL ? "" : account->account().c_str(), fight->component->baseAtt->name(), (long long)fight->component->baseAtt->roleID(), unit->day, unit->pass);
			   fputs(buf, file);
			   fclose(file);
			   }
			   }
			   */
		}

		static vector<struct PlayerEntity *> players;
		players.clear();
		int count = PlayerPool_Players(map, &players);
		for (int i = 0; i < count; i++) {
			struct Component *component = PlayerEntity_Component(players[i]);
			if (component == NULL)
				continue;
			if (component->fight == fight)
				continue;
			component->fight->clearRoom = true;
			struct PlayOffUnit *unit = MapPool_SelectPlayOff(component->baseAtt->roleID());
			if (win) {
				static NetProto_Lose proto;
				int32_t id = PlayerEntity_ID(component->player);
				GCAgent_SendProtoToClients(&id, 1, &proto);
				// playofftest
				/*
				   if (unit != NULL) {
				   FILE *file = fopen("PlayOffRecord.txt", "a");
				   if (file != NULL) {
				   char cur[32];
				   Time_ToDateTime((int)Time_TimeStamp(), cur);
				   char buf[CONFIG_FIXEDARRAY];
				   const PlayerInfo *account = AccountPool_IDToAccount(PlayerEntity_ID(component->player));
				   SNPRINTF1(buf, "[%s %d %s]%s.%s(%lld) lose, day: %d, pass: %d\n", __FILE__, __LINE__, cur, account == NULL ? "" : account->account().c_str(), component->baseAtt->name(), (long long)component->baseAtt->roleID(), unit->day, unit->pass);
				   fputs(buf, file);
				   fclose(file);
				   }
				   }
				   */
			} else {
				static NetProto_Win proto;
				int32_t id = PlayerEntity_ID(component->player);
				GCAgent_SendProtoToClients(&id, 1, &proto);
				if (unit != NULL) {
					if (component->baseAtt->roleID() == unit->lhs)
						unit->lWin++;
					else
						unit->rWin++;
					// playofftest
					/*
					   if (unit != NULL) {
					   FILE *file = fopen("PlayOffRecord.txt", "a");
					   if (file != NULL) {
					   char cur[32];
					   Time_ToDateTime((int)Time_TimeStamp(), cur);
					   char buf[CONFIG_FIXEDARRAY];
					   const PlayerInfo *account = AccountPool_IDToAccount(PlayerEntity_ID(component->player));
					   SNPRINTF1(buf, "[%s %d %s]%s.%s(%lld) win, day: %d, pass: %d\n", __FILE__, __LINE__, cur, account == NULL ? "" : account->account().c_str(), component->baseAtt->name(), (long long)component->baseAtt->roleID(), unit->day, unit->pass);
					   fputs(buf, file);
					   fclose(file);
					   }
					   }
					   */
				}
			}
		}

		struct Movement *linkers[MAX_ROOM_PLAYERS];
		count = MapPool_Linkers(map, linkers, MAX_ROOM_PLAYERS);
		for (int i = 0; i < count; i++) {
			struct Component *component = Movement_Component(linkers[i]);
			if (component == NULL)
				continue;
			if (component->fight == fight)
				continue;
			component->fight->clearRoom = true;
			static NetProto_Lose proto;
			int32_t id = PlayerEntity_ID(component->player);
			GCAgent_SendProtoToClients(&id, 1, &proto);
			// playofftest
			/*
			   if (unit != NULL) {
			   FILE *file = fopen("PlayOffRecord.txt", "a");
			   if (file != NULL) {
			   char cur[32];
			   Time_ToDateTime((int)Time_TimeStamp(), cur);
			   char buf[CONFIG_FIXEDARRAY];
			   const PlayerInfo *account = AccountPool_IDToAccount(PlayerEntity_ID(component->player));
			   SNPRINTF1(buf, "[%s %d %s]%s.%s(%lld) lose, day: %d, pass: %d\n", __FILE__, __LINE__, cur, account == NULL ? "" : account->account().c_str(), component->baseAtt->name(), (long long)component->baseAtt->roleID(), unit->day, unit->pass);
			   fputs(buf, file);
			   fclose(file);
			   }
			   }
			   */
		}

		if (unit != NULL) {
			const PlayOff *info = PlayOffManager_PlayOff(unit->playoff);
			bool over = false;
			int door = PlayOffManager_Count(info->over(unit->day)) / 2 + 1;
			struct PlayerEntity *winner = NULL, *loser = NULL;
			int64_t winnerID = -1, loserID = -1;
			const char *winnerName = "", *loserName = "";
			if (unit->lWin >= door) {
				over = true;
				winner = PlayerEntity_PlayerByRoleID(unit->lhs);
				loser = PlayerEntity_PlayerByRoleID(unit->rhs);
				winnerID = unit->lhs;
				loserID = unit->rhs;
				winnerName = unit->lName.c_str();
				loserName = unit->rName.c_str();
			} else if (unit->rWin >= door) {
				over = true;
				winner = PlayerEntity_PlayerByRoleID(unit->rhs);
				loser = PlayerEntity_PlayerByRoleID(unit->lhs);
				winnerID = unit->rhs;
				loserID = unit->lhs;
				winnerName = unit->rName.c_str();
				loserName = unit->lName.c_str();
			}
			if (over) {
				int index = PlayOffManager_NextIndex(unit->playoff, unit->day, unit->pass, true);
				if (winner != NULL) {
					struct Component *component = PlayerEntity_Component(winner);
					int event = component->playerAtt->fixedEvent(47) | (1 << index);
					PlayerEntity_SetFixedEvent(component->player, 47, event);
				} else {
					if (winnerID != -1) {
						static DCProto_ModifyFixedEventBit proto;
						proto.set_roleID(winnerID);
						proto.set_id(47);
						proto.set_bit(index);
						proto.set_one(true);
						GCAgent_SendProtoToDCAgent(&proto);
					}
				}
				if (winnerID != -1) {
					const PlayerInfo *account = AccountPool_IDToAccount(PlayerEntity_ID(winner));
					NoticeWinner(account == NULL ? NULL : account->account().c_str(), winnerName, unit, true);
				}

				if (LoseIsWin(unit)) {
					index = PlayOffManager_NextIndex(unit->playoff, unit->day, unit->pass, false);
					if (loser != NULL) {
						struct Component *component = PlayerEntity_Component(loser);
						int event = component->playerAtt->fixedEvent(47) | (1 << index);
						PlayerEntity_SetFixedEvent(component->player, 47, event);
					} else {
						if (loserID != -1) {
							static DCProto_ModifyFixedEventBit proto;
							proto.set_roleID(loserID);
							proto.set_id(47);
							proto.set_bit(index);
							proto.set_one(true);
							GCAgent_SendProtoToDCAgent(&proto);
						}
					}
					if (loserID != -1) {
						const PlayerInfo *account = AccountPool_IDToAccount(PlayerEntity_ID(loser));
						NoticeWinner(account == NULL ? NULL : account->account().c_str(), loserName, unit, false);
					}
				}
			}
		}
	} else {
		static vector<struct PlayerEntity *> players;
		players.clear();
		int count1 = PlayerPool_Players(map, NULL, &players);
		if (count1 < 0)
			count1 = 0;

		struct Movement *movements[CONFIG_FIXEDARRAY];
		int count2 = MapPool_Linkers(map, movements, CONFIG_FIXEDARRAY);
		if (count2 < 0)
			count2 = 0;

		struct NPCEntity *npcs[CONFIG_FIXEDARRAY];
		int count3 = NPCPool_Monsters(map, npcs, CONFIG_FIXEDARRAY);

		if (count1 + count2 + count3 >= 2) {
			struct Fight *another = NULL;
			for (int i = 0; i < count1; i++) {
				if (players[i] != fight->component->player) {
					another = PlayerEntity_Component(players[i])->fight;
					break;
				}
			}
			if (another == NULL) {
				for (int i = 0; i < count2; i++) {
					if (Movement_Component(movements[i])->fight != fight) {
						another = Movement_Component(movements[i])->fight;
						break;
					}
				}
				if (another == NULL) {
					for (int i = 0; i < count3; i++) {
						struct Component *component = NPCEntity_Component(npcs[i]);
						if (component == NULL)
							continue;
						another = component->fight;
					}
				}
			}
			if (another == NULL) {
				DEBUG_LOGERROR("Failed to find another");
			} else {
				int32_t winner = -1;
				int32_t loser = -1;
				int32_t awardHonor = 0;
				int32_t awardPVPScore = 0;
				if (win) {
					if (mapInfo->mapType() == MapInfo::PVP) {
						if (fight->component->player != NULL) {
							int count = (fight->component->playerAtt->dayEvent(30) & 0xff0000) >> 16;
							float scoreBase = Config_PVPScoreBase(count);
							if (scoreBase > 0) {
								awardHonor = (int32_t)((Config_PVPBaseHonor() + another->att->level()) * scoreBase);
								Item_ModifyHonor(fight->component->item, awardHonor);
								awardPVPScore = (int32_t)((Config_PVPBaseScore() + another->att->level()) * scoreBase);
								Item_ModifyPKScore(fight->component->item, awardPVPScore);
								PlayerEntity_SetDayEvent(fight->component->player, 30, (fight->component->playerAtt->dayEvent(30) & (~(0xff0000))) | ((count + 1) << 16));

								if (Config_ScribeHost() != NULL) {
									static char buf[CONFIG_FIXEDARRAY];
									memset(buf, 0, sizeof(buf) / sizeof(char));
									char *index = buf;

									SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(fight->component->player));
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"roleName\":\"%s", fight->component->baseAtt->name());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"pvpScore\":\"%d\"}", awardPVPScore);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "Pvp", buf));
								}
							}
						}
						if (another->component->player != NULL) {
							int count = (another->component->playerAtt->dayEvent(30) & 0xff0000) >> 16;
							float scoreBase = Config_PVPScoreBase(count);
							if (scoreBase > 0) {
								PlayerEntity_SetDayEvent(another->component->player, 30, (another->component->playerAtt->dayEvent(30) & (~(0xff0000))) | ((count + 1) << 16));
							}
						}
						fight->att->set_winPVP(fight->att->winPVP() + 1);
						another->att->set_losePVP(another->att->losePVP() + 1);
					}

					winner = PlayerEntity_ID(fight->component->player);
					loser = PlayerEntity_ID(another->component->player);
				} else {
					if (mapInfo->mapType() == MapInfo::PVP) {
						if (another->component->player != NULL) {
							int count = (another->component->playerAtt->dayEvent(30) & 0xff0000) >> 16;
							float scoreBase = Config_PVPScoreBase(count);
							if (scoreBase > 0) {
								awardHonor = (int32_t)((Config_PVPBaseHonor() + fight->att->level()) * scoreBase);
								Item_ModifyHonor(another->component->item, awardHonor);
								awardPVPScore = (int32_t)((Config_PVPBaseScore() + fight->att->level()) * scoreBase);
								Item_ModifyPKScore(another->component->item, awardPVPScore);
								PlayerEntity_SetDayEvent(another->component->player, 30, (another->component->playerAtt->dayEvent(30) & (~(0xff0000))) | ((count + 1) << 16));
							}
						}
						if (fight->component->player != NULL) {
							int count = (fight->component->playerAtt->dayEvent(30) & 0xff0000) >> 16;
							float scoreBase = Config_PVPScoreBase(count);
							if (scoreBase > 0) {
								PlayerEntity_SetDayEvent(fight->component->player, 30, (fight->component->playerAtt->dayEvent(30) & (~(0xff0000))) | ((count + 1) << 16));
							}
						}
						another->att->set_winPVP(another->att->winPVP() + 1);
						fight->att->set_losePVP(fight->att->losePVP() + 1);
					}

					winner = PlayerEntity_ID(another->component->player);
					loser = PlayerEntity_ID(fight->component->player);
				}
				PlayerEntity_ActiveAddGoods(fight->component->player, 3);
				PlayerEntity_ActiveAddGoods(another->component->player, 3);

				if (winner != -1) {
					static NetProto_Win win;
					win.Clear();
					win.set_honor(awardHonor);
					win.set_pvpScore(awardPVPScore);
					GCAgent_SendProtoToClients(&winner, 1, &win);
				}
				if (loser != -1) {
					static NetProto_Lose lose;
					lose.Clear();
					GCAgent_SendProtoToClients(&loser, 1, &lose);
				}

				another->clearRoom = true;
			}
		}
	}
}

int Fight_ClearRoom(struct Fight *fight, bool win, int group, NetProto_ClearRoom *award) {
	if (!Fight_IsValid(fight))
		return -1;
	if (fight->component->player == NULL)
		return -2;
	if (fight->clearRoom)
		return -3;

	bool pvp = false;
	int32_t map = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
	if (mapInfo != NULL) {
		if (mapInfo->mapType() != MapInfo::SINGLE
				&& mapInfo->mapType() != MapInfo::ROOM
				&& mapInfo->mapType() != MapInfo::PRACTICE
				&& mapInfo->mapType() != MapInfo::PVP
				&& mapInfo->mapType() != MapInfo::HELL
				&& mapInfo->mapType() != MapInfo::ONLY_ROOM)
			return -4;

		if (mapInfo->id() == Config_WorldBossMap()
				|| mapInfo->id() == Config_FactionBossMap()
				|| mapInfo->id() == Config_FactionWarMap()) {
			if (win) {
				static NetProto_Win win;
				win.Clear();
				int32_t id = PlayerEntity_ID(fight->component->player);
				GCAgent_SendProtoToClients(&id, 1, &win);
			} else {
				static NetProto_Lose lose;
				lose.Clear();
				int32_t id = PlayerEntity_ID(fight->component->player);
				GCAgent_SendProtoToClients(&id, 1, &lose);
			}
			fight->clearRoom = true;
			Fight_SetCantBeAttacked(fight, true);
			return 1;
		} else if (mapInfo->mapType() == MapInfo::PRACTICE
				|| mapInfo->mapType() == MapInfo::PVP
				|| mapInfo->mapType() == MapInfo::HELL) {
			pvp = true;
		} else {
			static char buf[CONFIG_FIXEDARRAY];
			memset(buf, 0, sizeof(buf) / sizeof(char));
			char *index = buf;

			if (win) {
				{
					if (Config_ScribeHost() != NULL) {
						SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(fight->component->player));
						index += strlen(index);
						SNPRINTF2(buf, index, "\",\"mapID\":\"%d", mapInfo->id());
						index += strlen(index);
						SNPRINTF2(buf, index, "\",\"status\":win");
						index += strlen(index);
						SNPRINTF2(buf, index, "\"}");
						ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "ClearRoom", buf));
					}
				}

				if (mapInfo->id() >= Config_TowerBegin() && mapInfo->id() <= Config_TowerEnd()) {
					Mission_CompleteTarget(fight->component->mission, MissionTarget::CLEAR_TOWER, -1, -1);
				}
				if (mapInfo->id() >= Config_BossBegin() && mapInfo->id() <= Config_BossEnd()) {
					Mission_CompleteTarget(fight->component->mission, MissionTarget::CLEAR_HERO, -1, -1);
					Mission_CompleteTarget(fight->component->mission, MissionTarget::CLEAR_ROOM, -1, -1);
					int events[] = {31, 42, 43};
					int index = (mapInfo->id() - Config_BossBegin()) / 15;
					if (index < (int)(sizeof(events) / sizeof(events[0]))) {
						index = events[index];
						int bit = ((mapInfo->id() - Config_BossBegin()) % 15) * 2;
						int count = (fight->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
						PlayerEntity_SetDayEvent(fight->component->player, index, (fight->component->playerAtt->dayEvent(index) & (~(3 << bit))) | ((count + 1) << bit));
						count = (fight->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
					}
				}
				if (mapInfo->id() >= Config_SingleBegin() && mapInfo->id() <= Config_SingleEnd()) {
					Mission_CompleteTarget(fight->component->mission, MissionTarget::CLEAR_ROOM, -1, -1);
				}
				if (mapInfo->id() >= Config_SingleEnhanceBegin() && mapInfo->id() <= Config_SingleEnhanceEnd()) {
					Mission_CompleteTarget(fight->component->mission, MissionTarget::CLEAR_ROOM, -1, -1);
					int events[] = {32, 33, 36, 37, 38, 39, 40, 41};
					int index = (mapInfo->id() - Config_SingleEnhanceBegin()) / 15;
					if (index < (int)(sizeof(events) / sizeof(events[0]))) {
						index = events[index];
						int bit = ((mapInfo->id() - Config_SingleEnhanceBegin()) % 15) * 2;
						int count = (fight->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
						PlayerEntity_SetDayEvent(fight->component->player, index, (fight->component->playerAtt->dayEvent(index) & (~(3 << bit))) | ((count + 1) << bit));
					}
				}
				if (mapInfo->id() >= Config_RoomBegin() && mapInfo->id() <= Config_RoomEnd()) {
					Mission_CompleteTarget(fight->component->mission, MissionTarget::CLEAR_ONLYROOM, -1, -1);
				}
				int64_t upRank = -1;
				int godScore = 0;
				if (mapInfo->id() == Config_GodMap()) {
					PlayerPool_SaveGodRankRecord(PlayerEntity_RoleID(fight->component->player), upRank, true, godScore);
				}

				if (mapInfo->id() >= Config_SingleBegin() && mapInfo->id() <= Config_SingleEnd()) {
					int index = mapInfo->id() - Config_SingleBegin();
					if (index < 0)
						index = 0;
					int events[] = {41, 42, 48, 49};
					for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
						if (index <= 30) {
							int event = fight->component->playerAtt->fixedEvent(events[i]);
							PlayerEntity_SetFixedEvent(fight->component->player, events[i], event | (1 << index));
							break;
						}
						index -= 30;
					}

					if (Config_RoomFinalLevel(group) >= 3) {
						int events[] = {57, 58, 59, 60};
						int index = (mapInfo->id() - Config_SingleBegin()) / 30;
						if (index < (int)(sizeof(events) / sizeof(events[0]))) {
							index = events[index];
							int bit = (mapInfo->id() - Config_SingleBegin()) % 30;
							int event = fight->component->playerAtt->fixedEvent(index);
							if ((event & (1 << bit)) == 0)
								PlayerEntity_SetFixedEvent(fight->component->player, index, event | (1 << bit));
						}
					}

					{
						int events[] = {72, 73, 74, 75, 76, 77, 78, 79};
						int index = (mapInfo->id() - Config_SingleBegin()) / 15;
						if (index < (int)(sizeof(events) / sizeof(events[0]))) {
							index = events[index];
							int bit = (mapInfo->id() - Config_SingleBegin()) % 15 * 2;
							int event = fight->component->playerAtt->fixedEvent(index);
							if ((event & (3 << bit)) < (Config_RoomFinalLevel(group) << bit)) {
								PlayerEntity_SetFixedEvent(fight->component->player, index, (event & (~(3 << bit))) | (Config_RoomFinalLevel(group) << bit));
							}
						}
					}
				} else if (mapInfo->id() >= Config_SingleEnhanceBegin() && mapInfo->id() <= Config_SingleEnhanceEnd()) {
					int index = mapInfo->id() - Config_SingleEnhanceBegin();
					if (index < 0)
						index = 0;
					int events[] = {43, 50, 51, 52};
					for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
						if (index <= 30) {
							int event = fight->component->playerAtt->fixedEvent(events[i]);
							PlayerEntity_SetFixedEvent(fight->component->player, events[i], event | (1 << index));
							break;
						}
						index -= 30;
					}

					if (Config_RoomFinalLevel(group) >= 3) {
						int events[] = {61, 62, 63, 64};
						int index = (mapInfo->id() - Config_SingleEnhanceBegin()) / 30;
						if (index < (int)(sizeof(events) / sizeof(events[0]))) {
							index = events[index];
							int bit = (mapInfo->id() - Config_SingleEnhanceBegin()) % 30;
							int event = fight->component->playerAtt->fixedEvent(index);
							if ((event & (1 << bit)) == 0)
								PlayerEntity_SetFixedEvent(fight->component->player, index, event | (1 << bit));
						}
					}

					{
						int events[] = {80, 81, 82, 83};
						int index = (mapInfo->id() - Config_SingleEnhanceBegin()) / 15;
						if (index < (int)(sizeof(events) / sizeof(events[0]))) {
							index = events[index];
							int bit = (mapInfo->id() - Config_SingleEnhanceBegin()) % 15 * 2;
							int event = fight->component->playerAtt->fixedEvent(index);
							if ((event & (3 << bit)) < (Config_RoomFinalLevel(group) << bit)) {
								PlayerEntity_SetFixedEvent(fight->component->player, index, (event & (~(3 << bit))) | (Config_RoomFinalLevel(group) << bit));
							}
						}
					}
				}

				if (mapInfo->id() >= Config_SingleEnhanceBegin() && mapInfo->id() <= Config_SingleEnhanceEnd()) {
					PlayerEntity_ActiveFight(fight->component->player);
					PlayerEntity_ActiveAddGoods(fight->component->player, 2);
				}

				if (mapInfo->id() >= Config_RoomBegin() && mapInfo->id() <= Config_RoomEnd()) {
					PlayerEntity_ActiveAddGoods(fight->component->player, 2);
				}

				if (mapInfo->id() >= Config_SingleBegin() && mapInfo->id() <= Config_SingleEnd()) {
					PlayerEntity_ActiveAddGoods(fight->component->player, 2);
				}

				if (mapInfo->id() == Config_EventMap1_Easy()
						|| mapInfo->id() == Config_EventMap1_Normal()
						|| mapInfo->id() == Config_EventMap1_Hard()) {
					int event = fight->component->playerAtt->dayEvent(28);
					int count = (event & 0xf00) >> 8;
					PlayerEntity_SetDayEvent(fight->component->player, 28, (event & (~0xf00)) | ((count + 1) << 8));
					PlayerEntity_SetDayEvent(fight->component->player, 44, (int)Time_TimeStamp());
				}
				if (mapInfo->id() == Config_EventMap_ProtectCrystal_Easy()
						|| mapInfo->id() == Config_EventMap_ProtectCrystal_Normal()
						|| mapInfo->id() == Config_EventMap_ProtectCrystal_Hard()) {
					int event = fight->component->playerAtt->dayEvent(28);
					int count = (event & 0xf000) >> 12;
					PlayerEntity_SetDayEvent(fight->component->player, 28, (event & (~0xf000)) | ((count + 1) << 12));
					PlayerEntity_SetDayEvent(fight->component->player, 45, (int)Time_TimeStamp());
				}
				if (mapInfo->id() == Config_EventMap_GoldPig_Easy()
						|| mapInfo->id() == Config_EventMap_GoldPig_Normal()
						|| mapInfo->id() == Config_EventMap_GoldPig_Hard()) {
					int event = fight->component->playerAtt->dayEvent(28);
					int count = (event & 0xf0000) >> 16;
					PlayerEntity_SetDayEvent(fight->component->player, 28, (event & (~0xf0000)) | ((count + 1) << 16));
					PlayerEntity_SetDayEvent(fight->component->player, 62, (int)Time_TimeStamp());
				}
				if (mapInfo->id() == Config_EventMap_PetBattle_Easy()
						|| mapInfo->id() == Config_EventMap_PetBattle_Normal()
						|| mapInfo->id() == Config_EventMap_PetBattle_Hard()) {
					int event = fight->component->playerAtt->dayEvent(28);
					int count = (event & 0xf00000) >> 20;
					PlayerEntity_SetDayEvent(fight->component->player, 28, (event & (~0xf00000)) | ((count + 1) << 20));
					PlayerEntity_SetDayEvent(fight->component->player, 63, (int)Time_TimeStamp());
				}
				if (mapInfo->id() == Config_EventMap_RobRes_Easy()
						|| mapInfo->id() == Config_EventMap_RobRes_Normal()
						|| mapInfo->id() == Config_EventMap_RobRes_Hard()) {
					int event = fight->component->playerAtt->dayEvent(76);
					PlayerEntity_SetDayEvent(fight->component->player, 76, event + (1 << 8));
					PlayerEntity_SetDayEvent(fight->component->player, 75, (int)Time_TimeStamp());
				}
				if (mapInfo->id() == Config_EventMap_PetStone_Easy()
						|| mapInfo->id() == Config_EventMap_PetStone_Normal()
						|| mapInfo->id() == Config_EventMap_PetStone_Hard()) {
					int event = fight->component->playerAtt->dayEvent(76);
					PlayerEntity_SetDayEvent(fight->component->player, 76, event + (1 << 4));
					PlayerEntity_SetDayEvent(fight->component->player, 74, (int)Time_TimeStamp());
				}
				if (mapInfo->id() == Config_EventMap_ObstacleFan_Easy()
						|| mapInfo->id() == Config_EventMap_ObstacleFan_Normal()
						|| mapInfo->id() == Config_EventMap_ObstacleFan_Hard()) {
					int event = fight->component->playerAtt->dayEvent(76);
					PlayerEntity_SetDayEvent(fight->component->player, 76, event + 1);
					PlayerEntity_SetDayEvent(fight->component->player, 73, (int)Time_TimeStamp());
				}

				if (Config_MulTowerType(mapInfo->id()) == 1) {
					int groupID = Config_MulTowerGroup(mapInfo->id());
					int event = fight->component->playerAtt->dayEvent(48);
					int count = (event & (3 << (groupID * 2))) >> (groupID * 2);
					PlayerEntity_SetDayEvent(fight->component->player, 48, (event & (~(3 << (groupID * 2)))) | (count << (groupID * 2)));
				}

				int32_t money = mapInfo->money();
				int32_t exp = mapInfo->exp();
				int32_t honor = mapInfo->honor();
				const ItemPackage *package = Item_Package(fight->component->item);
				if (mapInfo->id() == Config_SurviveMap()) {
					if (group < 0 || group > Config_TowerEnd() - Config_TowerBegin() + 1)
						return -5;
					if (group > 0) {
						/*
						   if (Movement_InSingle(fight->component->movement)) {
						   const MapInfo *curPowerInfo = MapInfoManager_MapInfo(Config_TowerBegin() + group - 1);
						   if (curPowerInfo == NULL)
						   return -1;
						   if (Fight_Power(fight) < curPowerInfo->requiredPower())
						   return -1;
						   }
						   */
						exp = PassAward(group, mapInfo->exp());
						DEBUG_LOG("group: %d, base: %d, exp: %d", group, mapInfo->exp(), exp);
						money = PassAward(group, mapInfo->money());
						honor = PassAward(group, mapInfo->honor());

						Fight_AddSurvive(fight, group);
						Mission_CompleteTarget(fight->component->mission, MissionTarget::SURVIVE_TO, group, 0);
						int count = (fight->component->playerAtt->dayEvent(30) & 0xf00) >> 8;
						PlayerEntity_SetDayEvent(fight->component->player, 30, (fight->component->playerAtt->dayEvent(30) & (~(0xf00))) | ((count + 1) << 8));
					} else {
						money = 0;
						exp = 0;
						honor = 0;
						DEBUG_LOG("group %d", group);
					}
				} else if (mapInfo->id() >= Config_TowerBegin() && mapInfo->id() <= Config_TowerEnd()) {
					Fight_AddTower(fight);
				} else if (mapInfo->id() >= Config_BossBegin() && mapInfo->id() <= Config_BossEnd()) {
					int event = mapInfo->id() - Config_BossBegin() + 1;
					if (event > fight->component->playerAtt->fixedEvent(4))
						PlayerEntity_SetFixedEvent(fight->component->player, 4, event);
				} else if (mapInfo->id() == Config_GodMap()) {
					/*	godScore = Config_GodScore(fight->godTargetPower);
						int count = fight->component->playerAtt->dayEvent(29) & 0xffff;
						godScore = (int32_t)(godScore * Config_GodAward(count));
						*/
				}

				struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
				int count = Status_GetType(fight->component->status, StatusInfo::ROOM_EXP, statuses, CONFIG_FIXEDARRAY);
				if (count > 0) {
					int v = 0;
					for (int i = 0; i < count; i++) {
						struct StatusEntity *status = statuses[i];
						const StatusInfo *info = StatusEntity_Info(status);
						v += (int)(exp * info->percent()) + info->value();
					}
					exp += v;
				}
				if (package->vip() > 0) {
					const VIPInfo *vipInfo = VIPInfoManager_VIPInfo(package->vip());
					exp = (int32_t)(exp * (1.0f + vipInfo->expDelta() / 100.0f));
				}
				exp *= Config_MulExp();

				if (Config_InActiveTime(4)) {
					exp *= 2;
					money *= 2;
				}

				Fight_ModifyExp(fight, exp);
				Item_ModifyMoney(fight->component->item, money);
				Item_ModifyHonor(fight->component->item, honor);
				Item_ModifyGodScore(fight->component->item, godScore);

				if (award != NULL) {
					if (exp > 0)
						award->set_exp(exp);
					if (money > 0)
						award->set_money(money);
					if (honor > 0)
						award->set_honor(honor);
					if (godScore > 0)
						award->set_godScore(godScore);
					if (upRank >= 0)
						award->set_upRank((int)upRank);
				}
			} else {
				//Fight_DropTower(fight);
				static NetProto_Lose lose;
				lose.Clear();
				int32_t id = PlayerEntity_ID(fight->component->player);
				GCAgent_SendProtoToClients(&id, 1, &lose);

				if (Config_ScribeHost() != NULL) {
					SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(fight->component->player));
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"mapID\":\"%d", mapInfo->id());
					index += strlen(index);
					SNPRINTF2(buf, index, "\",\"status\":lose");
					index += strlen(index);
					SNPRINTF2(buf, index, "\"}");
					ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(fight->component->player)), PlayerEntity_ID(fight->component->player), "ClearRoom", buf));
				}

				if (mapInfo->id() == Config_GodMap()) {
					int64_t upRank = -1;
					int score = 0;
					PlayerPool_SaveGodRankRecord(PlayerEntity_RoleID(fight->component->player), upRank, false, score);
				}
			}
		}
	} else {
		map = Movement_NextMap(fight->component->movement);
		mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
		if (mapInfo != NULL && (mapInfo->mapType() == MapInfo::PRACTICE
					|| mapInfo->mapType() == MapInfo::PVP
					|| mapInfo->mapType() == MapInfo::HELL)) {
			pvp = true;
		} else {
			return -6;
		}
	}

	fight->clearRoom = true;
	Fight_SetCantBeAttacked(fight, true);

	if (pvp) {
		ClearPVP(fight, win);
		return 1;
	}
	return 0;
}

bool Fight_HasClearedRoom(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return false;

	return fight->clearRoom;
}

int Fight_OpenRoomBox(struct Fight *fight, bool free, int32_t *results, size_t size) {
	if (!Fight_IsValid(fight) || results == NULL)
		return -1;
	if (fight->component->player == NULL)
		return -1;
	if (!fight->clearRoom)
		return -1;

	if (free) {
		if (fight->openRoomFreeBox)
			return 0;
	} else {
		if (fight->openRoomGemBox)
			return 0;
	}

	int32_t map = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
	if (mapInfo == NULL)
		return -2;
	if (mapInfo->mapType() != MapInfo::SINGLE
			&& mapInfo->mapType() != MapInfo::ROOM
			&& mapInfo->mapType() != MapInfo::ONLY_ROOM)
		return -3;

	if (mapInfo->awardType() != MapInfo::FINAL)
		return -3;

	int32_t box = -1;
	if (free) {
		box = mapInfo->freeBox();
		fight->openRoomFreeBox = true;
	} else {
		box = mapInfo->gemBox();
		fight->openRoomGemBox = true;
	}

	int ret = Item_Lottery(fight->component->item, box, results, size);
	Item_NoticeBox(fight->component->item, box, results, ret, 0);
	return ret;
}

/*
   int Fight_Recover(struct Fight *fight) {
   if (!Fight_IsValid(fight))
   return -1;

   if (fight->att->status() == FightAtt::DEAD)
   return -1;

   if (fight->recoverCount >= Config_MaxFreeRecoverCount()) {
   if (Item_Package(fight->component->item)->rmb() < Config_RecoverGem())
   return -1;
   Item_ModifyRMB(fight->component->item, -Config_RecoverGem(), false);
   }
   fight->recoverCount++;

   Fight_ModifyHP(fight, NULL, Fight_FinalProperty(fight, FightAtt::MAXHP));
   Fight_ModifyMana(fight, NULL, Config_MaxMana());
   return 0;
   }
   */

int Fight_QuickFight(struct Fight *fight, int32_t room, int32_t count) {
	if (!Fight_IsValid(fight))
		return -1;
	if (count <= 0)
		return -1;

	int32_t map = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
	if (info == NULL)
		return -1;
	if (info->mapType() != MapInfo::PEACE)
		return -1;

	int cur = (int)Time_TimeStamp();
	bool updateCD = false;

	int begin = 0;
	if (room >= Config_TowerBegin() && room <= Config_TowerEnd()) {
		if (Config_TowerRecord(fight->att->maxTower()) <= 0) {
			return-1;
		}
		if (room != Config_TowerBegin() + Config_TowerRecord(fight->att->maxTower()) - 1)
			return -1;
		int dayEvent = fight->component->playerAtt->dayEvent(27);
		if ((dayEvent & 0x01) == 1) {
			return -1;
		} else {
			PlayerEntity_SetDayEvent(fight->component->player, 27, dayEvent | 0x01, false);
			fight->att->set_curTower(Config_TowerRecord(fight->att->maxTower()) + 1);
		}
		begin = Config_TowerBegin();
		count = 1;
	} else {
		info = MapInfoManager_MapInfo(room);
		if (info == NULL)
			return -1;

		if (cur < fight->component->playerAtt->fixedEvent(56))
			return -1;

		const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
		bool can = false;
		if (vip != NULL)
			can = vip->quickFight();

		if (info->id() >= Config_SingleBegin() && info->id() <= Config_SingleEnd()) {
			if (!can) {
				int events[] = {57, 58, 59, 60};
				int index = (info->id() - Config_SingleBegin()) / 30;
				if (index < (int)(sizeof(events) / sizeof(events[0]))) {
					index = events[index];
					int bit = (info->id() - Config_SingleBegin()) % 30;
					if ((fight->component->playerAtt->fixedEvent(index) & (1 << bit)) == 0)
						return -1;
				} else {
					return -1;
				}
			}

			int index = info->id() - Config_SingleBegin();
			if (index < 0)
				index = 0;
			int events[] = {41, 42, 48, 49};
			for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
				if (index <= 30) {
					if (!(fight->component->playerAtt->fixedEvent(events[i]) & (1 << index)))
						return -1;
					break;
				}
				index -= 30;
			}
		} else if (info->id() >= Config_SingleEnhanceBegin() && info->id() <= Config_SingleEnhanceEnd()) {
			if (!can) {
				int events[] = {61, 62, 63, 64};
				int index = (info->id() - Config_SingleEnhanceBegin()) / 30;
				if (index < (int)(sizeof(events) / sizeof(events[0]))) {
					index = events[index];
					int bit = (info->id() - Config_SingleEnhanceBegin()) % 30;
					if ((fight->component->playerAtt->fixedEvent(index) & (1 << bit)) == 0)
						return -1;
				} else {
					return -1;
				}
			}

			{
				int index = info->id() - Config_SingleEnhanceBegin();
				if (index < 0)
					index = 0;
				int events[] = {43, 50, 51, 52};
				for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
					if (index <= 30) {
						if (!(fight->component->playerAtt->fixedEvent(events[i]) & (1 << index)))
							return -1;
						break;
					}
					index -= 30;
				}
			}
			{
				int events[] = {32, 33, 36, 37, 38, 39, 40, 41};
				int index = (info->id() - Config_SingleEnhanceBegin()) / 15;
				if (index < (int)(sizeof(events) / sizeof(events[0]))) {
					index = events[index];
					int bit = ((info->id() - Config_SingleEnhanceBegin()) % 15) * 2;
					int countEnhance = (fight->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
					if (countEnhance >= Config_SingleEnhanceCount())
						return -1;
					if (countEnhance + count > Config_SingleEnhanceCount())
						count = Config_SingleEnhanceCount() - countEnhance;
				}
			}
		} else {
			return -1;
		}

		updateCD = true;

		const ItemPackage *package = Item_Package(fight->component->item);
		if (NULL == package) {
			return -1;
		}

		if (package->durability() < info->durability() * count) {
			DEBUG_LOG("durability not enough!");
			return -1;
		}

		if (info->mapType() != MapInfo::SINGLE)
			return -1;
		if (Fight_Power(fight) < info->requiredPower())
			return -1;

		if (package->money() < info->sweepMoney() * count) {
			DEBUG_LOG("money not enough!");
			return -1;
		}

		Item_ModifyMoney(fight->component->item, -info->sweepMoney() * count);
		Item_ModifyDurability(fight->component->item, -info->durability() * count);
		begin = room;
	}

	static NetProto_QuickFight qf;
	qf.Clear();
	qf.set_map(room);
	for (int i = begin; i <= room; i++) {
		info = MapInfoManager_MapInfo(i);
		assert(info != NULL);

		for (int j = 0; j < count; ++j) {
			static NetProto_GetRes gr;
			gr.Clear();
			if (info->exp() > 0) {
				Fight_ModifyExp(fight, info->exp());
				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::EXP);
				item->set_count(info->exp());
			}
			if (info->money() > 0) {
				Item_ModifyMoney(fight->component->item, info->money());
				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::MONEY);
				item->set_count(info->money());
			}
			if (info->honor() > 0) {
				Item_ModifyHonor(fight->component->item, info->honor());
				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::HONOR);
				item->set_count(info->honor());
			}
			if (info->soul() > 0) {
				Item_ModifySoul(fight->component->item, info->soul());
				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::SOUL);
				item->set_count(info->soul());
			}
			if (info->smallSoulJade() > 0) {
				Item_ModifySoulJade(fight->component->item, ExploreInfo::SMALL, info->smallSoulJade());
				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::SOULJADE);
				item->set_count(info->smallSoulJade());
			}
			if (info->soulStone() > 0) {
				Item_ModifySoulStone(fight->component->item, info->soulStone());
				PB_ItemInfo *item = gr.add_items();
				item->set_type(PB_ItemInfo::SOULSTONE);
				item->set_count(info->soulStone());
			}
			if (info->freeBox() != -1) {
				int32_t results[CONFIG_FIXEDARRAY];
				Item_Lottery(fight->component->item, info->freeBox(), results, CONFIG_FIXEDARRAY, false, &gr);
			}
			*qf.add_res() = gr;

			Mission_CompleteTarget(fight->component->mission, MissionTarget::ENTER_ANY_ROOM, info->id(), 0);
		}
	}
	int32_t player = PlayerEntity_ID(fight->component->player);
	GCAgent_SendProtoToClients(&player, 1, &qf);

	for (int i = 0; i < count; ++i) {
		if (info->id() >= Config_SingleEnhanceBegin() && info->id() <= Config_SingleEnhanceEnd()) {
			PlayerEntity_ActiveFight(fight->component->player);
			PlayerEntity_ActiveAddGoods(fight->component->player, 2);
		}

		if (info->id() >= Config_RoomBegin() && info->id() <= Config_RoomEnd()) {
			PlayerEntity_ActiveAddGoods(fight->component->player, 2);
		}

		if (info->id() >= Config_SingleBegin() && info->id() <= Config_SingleEnd()) {
			PlayerEntity_ActiveAddGoods(fight->component->player, 2);
		}
	}

	//	Item_ModifyRMB(fight->component->item, -info->rmb(), false, 3, info->id(), 0);

	if (room >= Config_SingleEnhanceBegin() && room <= Config_SingleEnhanceEnd()) {
		int events[] = {32, 33, 36, 37, 38, 39, 40, 41};
		int index = (room - Config_SingleEnhanceBegin()) / 15;
		if (index < (int)(sizeof(events) / sizeof(events[0]))) {
			index = events[index];
			int bit = ((room - Config_SingleEnhanceBegin()) % 15) * 2;
			int countEnhance = (fight->component->playerAtt->dayEvent(index) & (3 << bit)) >> bit;
			PlayerEntity_SetDayEvent(fight->component->player, index, (fight->component->playerAtt->dayEvent(index) & (~(3 << bit))) | ((count + countEnhance) << bit));
		}
	}

	if (updateCD) {
		PlayerEntity_SetFixedEvent(fight->component->player, 56, cur + Config_QuickFightCD() * count);
	}

	return 0;
}

int Fight_Timeout(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return -1;

	if (fight->clearRoom)
		return -1;

	int32_t map = Movement_Att(fight->component->movement)->mapID();
	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
	if (info == NULL)
		return -1;
	if (info->mapType() != MapInfo::PRACTICE && info->mapType() != MapInfo::PVP)
		return -1;

	struct Movement *movements[CONFIG_FIXEDARRAY];
	int count = MapPool_Linkers(map, movements, CONFIG_FIXEDARRAY);
	/*
	   if (count <= 0)
	   return -1;
	   */

	for (int i = 0; i < count; i++) {
		if (movements[i] == fight->component->movement)
			return -1;
	}

	Fight_ClearRoom(fight, true, 0);
	return 0;
}

static bool DoPVP(vector<struct Fight *> &fights, int32_t map, vector<Vector2i> &coords) {
	int32_t room = MapPool_Gen(map, true);
	if (room == -1) {
		DEBUG_LOGERROR("Failed to gen room");
		return false;
	}

	for (size_t i = 0; i < fights.size(); i++) {
		fights[i]->att->set_selfFaction(1 << i);
		fights[i]->att->set_friendlyFaction(fights[i]->att->selfFaction());
	}
	for (size_t i = 0; i < fights.size(); i++)
		Movement_BeginChangeScene(fights[i]->component->movement, room, &coords[i]);

	if (map == Config_PVPMap()) {
		if (fights.size() < 2) {
			static vector<PlayerAtt> atts;
			atts.clear();
			static vector<int64_t> exists;
			exists.clear();
			exists.push_back(fights[0]->component->baseAtt->roleID());
			PlayerPool_RandomFreeRoles(1, &exists, &atts);
			if (atts.size() > 0) {
				PlayerAtt *playerAtt = &atts[0];
				for (int k = 0; k < playerAtt->att().fightAtt().properties_size(); k++) {
					playerAtt->mutable_att()->mutable_fightAtt()->set_properties(k, fights[0]->att->properties(k));
					*playerAtt->mutable_att()->mutable_fightAtt()->mutable_propertiesDelta(k) =  fights[0]->att->propertiesDelta(k);
				}
				NPCAtt att;
				PlayerEntity_CopyAsNPC(playerAtt, &att, 0.8f, false);
				MapPool_GenNPC(room, 0, &att, &coords[1]);
			} else {
				NPCAtt att;
				PlayerEntity_CopyAsNPC(fights[0]->component->playerAtt, &att);
				MapPool_GenNPC(room, 0, &att, &coords[1]);
			}
		}
	}

	return true;
}

static void RemoveWaitPVP(set<struct Fight *, PVPComp> &container, struct Fight *fight) {
	for (set<struct Fight *, PVPComp>::iterator it = container.begin(); it != container.end(); it++) {
		if (*it == fight) {
			container.erase(it);
			break;
		}
	}
}

static int UpdateWaitPVP(set<struct Fight *, PVPComp> &pool, size_t num, int32_t map) {
	if (pool.size() < num)
		return 0;
	static vector<struct Fight *> container;
	container.clear();
	for (set<struct Fight *, PVPComp>::iterator it = pool.begin(); it != pool.end(); it++) {
		if (!Fight_IsValid(*it)) {
			RemoveWaitPVP(pool, *it);
			return 0;
		}
		container.push_back(*it);
	}
	if (container.size() < num)
		return 0;

	assert(map != -1);
	const MapInfo *info = MapInfoManager_MapInfo(map);
	assert(info != NULL);
	static vector<Vector2i> coords;
	if (coords.size() < num)
		coords.resize(num);
	for (size_t i = 0; i < num; i++) {
		if (!MapInfoManager_EnterCoord(info, i, &coords[i])) {
			DEBUG_LOGERROR("Failed to get enter coord");
			assert(0);
		}
	}

	return DoPVP(container, map, coords) ? 0 : -1;
}

int Fight_BeginWaitPVP(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return -1;
	if (fight->waitPVP)
		return -1;

	Movement_EndWaitRoom(fight->component->movement);
	Movement_DestroyRoom(fight->component->movement);
	Movement_LeaveRoom(fight->component->movement);
	Fight_EndWaitHell(fight);

	int32_t curMap = Movement_Att(fight->component->movement)->mapID();
	if (curMap < MAP_START)
		return -1;

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(curMap));
	if (info == NULL)
		return -1;
	if (info->mapType() != MapInfo::PEACE)
		return -1;

	/*
	   const ItemPackage *itemPackage = Item_Package(fight->component->item);
	   if (itemPackage->durability() < Config_PVPDurability())
	   return -1;
	   */

	if (!package.pvp.insert(fight).second)
		return -1;

	fight->waitPVP = true;
	fight->waitPVPTime = Time_ElapsedTime();

	static NetProto_Chat chat;
	chat.Clear();
	chat.set_channel(NetProto_Chat::SYSTEM);
	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, Config_Words(21), fight->component->baseAtt->name());
	chat.set_content(buf);
	GCAgent_SendProtoToAllClients(&chat);

	return UpdateWaitPVP(package.pvp, 2, Config_PVPMap());
}

void Fight_EndWaitPVP(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;

	if (fight->waitPVP) {
		RemoveWaitPVP(package.pvp, fight);
		fight->waitPVP = false;
	}
}

int Fight_PVPWaitCount() {
	return (int)package.pvp.size();
}

int Fight_BeginWaitHell(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return -1;
	if (fight->waitHell)
		return -1;

	/*
	   time_t cur = Time_TimeStamp();
	   if (!Config_IsHellOpen(cur)) {
	   static NetProto_Error error;
	   error.set_content(Config_Words(32));
	   int32_t id = PlayerEntity_ID(fight->component->player);
	   GCAgent_SendProtoToClients(&id, 1, &error);
	   return 1;
	   }
	   */

	Movement_EndWaitRoom(fight->component->movement);
	Movement_DestroyRoom(fight->component->movement);
	Movement_LeaveRoom(fight->component->movement);
	Fight_EndWaitPVP(fight);

	int32_t curMap = Movement_Att(fight->component->movement)->mapID();
	if (curMap < MAP_START)
		return -1;

	const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(curMap));
	if (info == NULL)
		return -1;
	if (info->mapType() != MapInfo::PEACE)
		return -1;

	const ItemPackage *itemPackage = Item_Package(fight->component->item);
	if (itemPackage->durability() < Config_PVPDurability())
		return -1;

	if (!package.hell.insert(fight).second)
		return -1;

	fight->waitHell = true;

	static NetProto_Chat chat;
	chat.Clear();
	chat.set_channel(NetProto_Chat::SYSTEM);
	char buf[CONFIG_FIXEDARRAY];
	SNPRINTF1(buf, Config_Words(45), fight->component->baseAtt->name());
	chat.set_content(buf);
	GCAgent_SendProtoToAllClients(&chat);

	return UpdateWaitPVP(package.hell, Config_MaxPlayersOfLineInWorldBoss(), Config_HellMap());
}

void Fight_EndWaitHell(struct Fight *fight) {
	if (!Fight_IsValid(fight))
		return;

	if (fight->waitHell) {
		RemoveWaitPVP(package.hell, fight);
		fight->waitHell = false;
	}
}

int Fight_HellWaitCount() {
	return (int)package.hell.size();
}

void Fight_SetIgnorePK(struct Fight *fight, bool value) {
	if (!Fight_IsValid(fight))
		return;

	fight->ignorePK = value;
	if (value)
		fight->ignorePKTime = Time_ElapsedTime();
}

int Fight_StrongBaseWing(struct Fight *fight, bool useRMB) {
	if (!Fight_IsValid(fight))
		return -1;

	int level = fight->att->baseWingLevel();
	int degree = fight->att->baseWingDegree() + 1;
	if (degree >= Config_MaxBaseWingDegree()) {
		degree = 0;
		level++;
	}
	const BaseWing *info = EquipmentInfoManager_BaseWing(level, degree);
	if (info == NULL)
		return -1;

	int count = Item_Count(fight->component->item, ItemInfo::GOODS, info->goods());
	if (count >= info->count()) {
		Item_DelFromPackage(fight->component->item, ItemInfo::GOODS, (int64_t)info->goods(), info->count());
	} else if (useRMB) {
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(info->goods());
		assert(goods != NULL);
		int cost = goods->rmb() * (info->count() - count);
		//const ItemPackage *package = Item_Package(fight->component->item);
		//		if (package->rmb() < cost)
		//			return -1;

		if (!Item_HasRMB(fight->component->item, cost)) {
			return -1;
		}
		Item_DelFromPackage(fight->component->item, ItemInfo::GOODS, (int64_t)info->goods(), count);
		Item_ModifyRMB(fight->component->item, -cost, false, 13, level, degree);
	} else {
		return -1;
	}

	Item_UseBaseWing(fight->component->item, false);
	fight->att->set_baseWingLevel(level);
	fight->att->set_baseWingDegree(degree);
	Item_UseBaseWing(fight->component->item, true);

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(fight->component->player));
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", fight->component->baseAtt->name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"strongLevel\":\"%d", level);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"strongDegree\":\"%d\"}", degree);
		int32_t id = PlayerEntity_ID(fight->component->player);
		ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "StrongBaseWing", buf));
	}	
	return 0;
}

void Fight_Update(struct Fight *fight) {
	assert(Fight_IsValid(fight));

	int64_t cur = Time_ElapsedTime();
	UpdateTransform(fight);

	if (fight->ignorePK) {
		if (cur - fight->ignorePKTime > Config_IgnorePKInterval()) {
			fight->ignorePK = false;
		}
	}

	if (fight->waitPVP) {
		if (cur - fight->waitPVPTime > Config_MaxWaitPVPTime()) {
			static vector<struct Fight *> container;
			container.clear();
			container.push_back(fight);

			const MapInfo *info = MapInfoManager_MapInfo(Config_PVPMap());
			assert(info != NULL);
			static vector<Vector2i> coords;
			if (coords.size() < 2)
				coords.resize(2);
			for (size_t i = 0; i < 2; i++) {
				if (!MapInfoManager_EnterCoord(info, i, &coords[i])) {
					DEBUG_LOGERROR("Failed to get enter coord");
					assert(0);
				}
			}

			DoPVP(container, Config_PVPMap(), coords);
		}
	}

	if (fight->component->npc != NULL) {
		if (fight->att->status() != FightAtt::DEAD) {
			if (NPCEntity_Master(fight->component->npc) == NULL) { // not pet
				int32_t mapID = fight->component->roleAtt->movementAtt().mapID();
				const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
				if (mapInfo != NULL) {
					if (mapInfo->mapType() == MapInfo::PVP || mapInfo->mapType() == MapInfo::HELL) {
						if (!fight->isTransforming) {
							if (fight->prevTransformTime <= 0) {
								if (fight->att->hp() < Fight_FinalProperty(fight, FightAtt::MAXHP) * Config_AITransformHP()) {
									Fight_Transform(fight, true);
								}
							} else {
								if (cur - fight->prevTransformTime > Config_AITransformInterval()) {
									Fight_Transform(fight, true);
								}
							}
						}
					} else {
						if (!fight->isSpecial && fight->unspecialTime != 0) {
							if (cur - fight->unspecialTime > Config_SpecialInterval()) {
								Fight_SetSpecial(fight, true);
							}
						}
					}
				}
			}
		}
	}

	if (fight->att->status() == FightAtt::IDLE) {
		return;
	} else if (fight->att->status() == FightAtt::MOVE) {
		const Vector3f *tPos = NULL;
		// float extra = 0.0f;
		if (fight->tFight != NULL) {
			if (!Fight_IsValid(fight->tFight)) {
				Fight_Cancel(fight);
				return;
			}
			tPos = &Movement_Att(fight->tFight->component->movement)->position();
			// extra = (Movement_Att(fight->component->movement)->radius() + Movement_Att(fight->tFight->component->movement)->radius()) * MAP_BLOCKSIZE;
		} else {
			// extra = Movement_Att(fight->component->movement)->radius() * MAP_BLOCKSIZE;
			tPos = &fight->tPos;
		}

		if (IsNear(&Movement_Att(fight->component->movement)->position(), tPos, fight->curSkill->dist()/* + extra*/)) {
			Fight_DoAttack(fight, NULL, 0, Time_ElapsedTime());
		}
	} else if (fight->att->status() == FightAtt::ATTACK) {
		assert(fight->component->npc != NULL);
		assert(fight->prevSkill != NULL);
		int64_t cur = Time_ElapsedTime();
		if (cur - fight->publicCD > fight->prevSkill->fireActionTime())
			fight->att->set_status(FightAtt::IDLE);
	} else if (fight->att->status() == FightAtt::DEAD) {
		if (fight->att->reviveTime() > 0) {
			if (Time_ElapsedTime() - fight->deadTime >= (int64_t)fight->att->reviveTime()) {
				if (fight->component->player != NULL) {
					/*
					   const MovementAtt *movementAtt = Movement_Att(fight->component->movement);
					   const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(movementAtt->mapID()));
					   if (mapInfo == NULL)
					   return;
					   if (mapInfo->id() != Config_SurviveMap()
					   && !(mapInfo->id() >= Config_TowerBegin() && mapInfo->id() <= Config_TowerEnd())
					   && !(mapInfo->id() >= Config_BossBegin() && mapInfo->id() <= Config_BossEnd())
					   && (mapInfo->mapType() != MapInfo::PRACTICE && mapInfo->mapType() != MapInfo::PVP))
					   Fight_Revive(fight, movementAtt->prevNormalMap(), &movementAtt->prevCoord(), 1.0f, 1.0f, NetProto_Revive::CITY);
					   */
				} else if (fight->component->npc != NULL) {
					Fight_Revive(fight, &AI_Att(fight->component->ai)->birthCoord(), 1.0f, 1.0f);
				} else {
					assert(0);
				}
			}
		}
	}
}


void Fight_SetFightPet(struct Fight *fight, int32_t index) {
	if (!Fight_IsValid(fight)) {
		return;
	}
	fight->att->set_fightingPet(index);
}

void Fight_SetAttachPet(struct Fight *fight, int32_t index) {
	/*
	   if (!Fight_IsValid(fight)) {
	   return;
	   }
	   Fight_EnhanceByPet(fight, false);
	   fight->att->set_enhancingPet(index);
	   Fight_EnhanceByPet(fight, true);
	   */
}

void Fight_SetPetRest(struct Fight *fight, bool flag) {
	if (!Fight_IsValid(fight)) { 
		return;
	}

	if (flag) {
		fight->att->set_fightingPet(-1);
	} else {
		//	Fight_EnhanceByPet(fight, false);
		//	fight->att->set_enhancingPet(-1);
	}
}


bool Fight_RestCheckPoint(struct Fight *fight) {
	if (!Fight_IsValid(fight)) {
		return false;
	}

	int dayEvent = fight->component->playerAtt->dayEvent(27);
	//	if (	(dayEvent & 0x01)  == 0) {
	//		return false;
	//	}

	if ((dayEvent & 0x02) == 2) {
		int count = fight->component->playerAtt->dayEvent(35) & 0xff;
		const VIPInfo *vip = VIPInfoManager_VIPInfo(fight->component->playerAtt->itemPackage().vip());
		if (vip == NULL)
			return false;
		if (count >= vip->towerResetCount())
			return false;
	} 

	//		PlayerEntity_SetDayEvent(fight->component->player, 27, dayEvent | 0x02, false);
	//		dayEvent = fight->component->playerAtt->dayEvent(27);
	dayEvent = dayEvent | 0x02;
	PlayerEntity_SetDayEvent(fight->component->player, 27, dayEvent & 0xFFFFFFFE, false);
	fight->att->set_curTower(1);
	return true;
}


void Fight_TransformWar(struct Fight* fight, int id) {
	if (!Fight_IsValid(fight)) { 
		return;
	}

	fight->att->set_transformID(id);
}

int Fight_DropItem(struct Fight *fight, int npc, int index, int id) {
	if (!Fight_IsValid(fight))
		return -1;

	for (int i = 0; i < (int)fight->npcID.size(); ++i) {
		//if (fight->npcID[i] == npc) {
		if (fight->npcID[i] == id) {
			return -2;
		}else {
			int32_t map = Movement_Att(fight->component->movement)->mapID();
			const MapInfo * mapInfo = MapInfoManager_MapInfo(map);
			if (mapInfo == NULL) {
				return -2;
			}
			const MapUnit * mapUnit = MapInfoManager_MapUnit(mapInfo);
			if (mapUnit == NULL) 
				return -2;

			bool flag = true;
			for (int i = 0; i < mapUnit->npcs_size(); ++i) {
				if (mapUnit->npcs(i).id() == id) {
					flag = false;
					break;
				}
			}

			if (flag) {
				return -2;
			}
		}
	}

	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(fight->component->roleAtt->movementAtt().mapID()));
	if (mapInfo == NULL || mapInfo->mapType() == MapInfo::PEACE)
		return -2;

	const NPCAtt *npcInfo = NPCInfoManager_NPC(npc);
	if (npcInfo == NULL)
		return -3;

	const DropTable *table = DropTableManager_DropTable(npcInfo->dropID());
	if (table == NULL)
		return -4;

	if (index < 0 || index >= table->dropItems_size())
		return -5;

	const DropItem *drop = &table->dropItems(index);
	switch (drop->type()) {
		case PB_ItemInfo::MONEY:
			{
				Item_ModifyMoney(fight->component->item, drop->id());
			}
			break;
		case PB_ItemInfo::HP:
			{
				if (!MapPool_IsSingle(fight->component->roleAtt->movementAtt().mapID())) {
					float percent = (float)drop->id() / (float)Config_Divisor();
					int maxHP = Fight_FinalProperty(fight, FightAtt::MAXHP);
					int delta = (int)(maxHP * percent);
					if (delta < 0)
						delta = 0;
					Fight_ModifyHP(fight, NULL, delta);

					static NetProto_ModifyHP proto;
					int32_t player = PlayerEntity_ID(fight->component->player);
					proto.set_id(player);
					proto.set_hp(fight->att->hp());
					GCAgent_SendProtoToClients(&player, 1, &proto);
				}
			}
			break;
		default:
			break;
	}

	//fight->npcID.push_back(npc);
	fight->npcID.push_back(id);

	return 0;
}

int32_t Fight_PetPower(const PB_PlayerAtt *att, int index) {
	if (att == NULL)
		return -1;

	const std::map<int, map<int, PB_PetHaloInfo> >* info = NPCInfoManager_PetHaloInfo();
	if (info == NULL)
		return -2;

	if (index < 0 || index >= att->pets_size()) 
		return -3;

	if (att->pets(index).id() == -1) 
		return -4;

	int pet_level = att->pets(index).pet_level();

	int value[PB_FightAtt_PropertyType_PropertyType_ARRAYSIZE] = {0};
	const static int addValue[PB_FightAtt_PropertyType_PropertyType_ARRAYSIZE] = {45, 30, 450, 10, 20, 20};

	for (int i = 0; i < att->haloLevel_size() && i < att->haloValue_size(); ++i) {
		map<int, map<int, PB_PetHaloInfo> >::const_iterator itGroup = info->find(i);
		if (itGroup == info->end()) {
			continue;
		}

		map<int, PB_PetHaloInfo>::const_iterator itUnit = itGroup->second.find(att->haloLevel(i));
		if (itUnit == itGroup->second.end()) {
			continue;
		}

		value[i] = itUnit->second.propertyValue() * (1 + att->haloValue(i) / 10000.0f) + addValue[i] * (pet_level - 1);

	}

	int quality = att->pets(index).quality();
	int level = att->pets(index).level();
	const NPCAtt* pet = NPCInfoManager_Pet(att->pets(index).id(), quality, level);
	PB_NPCAtt npcAtt;
	pet->ToPB(&npcAtt);
	PB_FightAtt fightAtt = npcAtt.att().fightAtt();

	for (int i = 0; i < (int)(sizeof(value) / sizeof(int)); i++) {
		if (value[i] >= 0) {
			fightAtt.set_properties(i, fightAtt.properties(i) + value[i]);
		}
	}

	return Fight_Power(&fightAtt);
}

bool Fight_ToPetData(const PlayerAtt * playerAtt, int index, NPCAtt * att) {
	if (att == NULL || playerAtt == NULL)
		return false;

	const std::map<int, map<int, PB_PetHaloInfo> >* info = NPCInfoManager_PetHaloInfo();
	if (info == NULL)
		return false;

	if (index < 0 || index >= playerAtt->pets_size()) 
		return false;

	if (playerAtt->pets(index).id() == -1) 
		return false;

	int quality = playerAtt->pets(index).quality();
	int level = playerAtt->pets(index).level();
	*att = *NPCInfoManager_Pet(playerAtt->pets(index).id(), quality, level);

	int pet_level = playerAtt->pets(index).pet_level();
	att->mutable_att()->mutable_fightAtt()->set_level(pet_level);
	att->set_level(level);
	att->set_quality(quality);
	att->mutable_att()->mutable_fightAtt()->set_selfFaction(playerAtt->att().fightAtt().selfFaction());
	att->mutable_att()->mutable_fightAtt()->set_friendlyFaction(playerAtt->att().fightAtt().friendlyFaction());
	att->mutable_att()->mutable_baseAtt()->set_name(playerAtt->pets(index).name());

	int value[PB_FightAtt_PropertyType_PropertyType_ARRAYSIZE] = {0};
	const static int addValue[PB_FightAtt_PropertyType_PropertyType_ARRAYSIZE] = {45, 30, 450, 10, 20, 20};

	for (int i = 0; i < playerAtt->haloLevel_size() && i < playerAtt->haloValue_size(); ++i) {
		map<int, map<int, PB_PetHaloInfo> >::const_iterator itGroup = info->find(i);
		if (itGroup == info->end()) {
			continue;
		}

		map<int, PB_PetHaloInfo>::const_iterator itUnit = itGroup->second.find(playerAtt->haloLevel(i));
		if (itUnit == itGroup->second.end()) {
			continue;
		}

		value[i] = itUnit->second.propertyValue() * (1 + playerAtt->haloValue(i) / 10000.0f) + addValue[i] * (pet_level - 1);
	}

	for (int j = 0; j < att->att().fightAtt().properties_size(); j++) {
		att->mutable_att()->mutable_fightAtt()->set_properties(j, att->att().fightAtt().properties(j) + value[j]);
	}

	return true;
}





