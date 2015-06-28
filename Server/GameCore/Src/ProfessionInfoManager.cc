#include "ProfessionInfoManager.hpp"
#include "PlayerAtt.hpp"
#include "Config.hpp"
#include "ProfessionInfo.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "EquipmentInfoManager.hpp"
#include "BloodInfoManager.hpp"
#include "SkillInfoManager.hpp"
#include "PlayerEntity.hpp"
#include "RoleAtt.hpp"
#include <vector>

using namespace std;

static struct{
	vector<PlayerAtt> pool;
}package;

static void AddSkillToALT(PlayerAtt *info, int pos) {
	for (int i = 0; i < info->alt().alt_size(); i++) {
		ItemInfo *alt = info->mutable_alt()->mutable_alt(i);
		if (alt->type() == ItemInfo::NONE) {
			alt->set_type(ItemInfo::SKILL);
			alt->set_id(pos);
			break;
		}
	}
}

static void ClearSkill(PlayerAtt *info) {
	for (int i = 0; i < info->att().fightAtt().skills_size(); i++) {
		info->mutable_att()->mutable_fightAtt()->mutable_skills(i)->set_id(-1);
		info->mutable_att()->mutable_fightAtt()->mutable_skills(i)->set_level(0);
	}
	for (int i = 0; i < info->alt().alt_size(); i++) {
		info->mutable_alt()->mutable_alt(i)->set_type(ItemInfo::NONE);
		info->mutable_alt()->mutable_alt(i)->set_id(0);
	}
}

static void AddSkill(PlayerAtt *info, int id) {
	Skill skill;
	skill.set_id(id);
	for (int i = 0; i < info->att().fightAtt().skills_size(); i++) {
		if (info->att().fightAtt().skills(i).id() == -1) {
			const SkillInfo *skillInfo = SkillInfoManager_SkillInfo(id, 1);
			assert(skillInfo != NULL);
			if (skillInfo->requireLevel() == 1) {
				AddSkillToALT(info, i);
				skill.set_level(1);
			} else {
				skill.set_level(0);
			}
			*info->mutable_att()->mutable_fightAtt()->mutable_skills(i) = skill;
			break;
		}
	}
}

void ProfessionInfoManager_Init() {
}

void ProfessionInfoManager_Prepare() {
	PlayerAtt info;
	PlayerEntity_FilterData(&info);

	info.mutable_att()->mutable_movementAtt()->set_status(MovementAtt::IDLE);
	info.mutable_att()->mutable_movementAtt()->set_mapID(MAP_CREATE);
	int32_t firstMap = MapInfoManager_Next(MAP_CREATE, 0, info.mutable_att()->mutable_movementAtt()->mutable_prevCoord());
	info.mutable_att()->mutable_movementAtt()->set_prevNormalMap(firstMap);
	info.mutable_att()->mutable_fightAtt()->set_status(FightAtt::IDLE);
	info.mutable_att()->mutable_fightAtt()->set_selfFaction(1 << 0);
	info.mutable_att()->mutable_fightAtt()->set_friendlyFaction((1 << 0) | (1 << 30));
	info.mutable_att()->mutable_fightAtt()->set_reviveTime(Config_PlayerReviveTime());
	info.mutable_att()->mutable_fightAtt()->set_level(1);
	// info.mutable_itemPackage()->set_vip(1);
	info.mutable_itemPackage()->set_validNumEquipment(21);
	//info.mutable_itemPackage()->set_validNumGoods(21);
	info.mutable_itemPackage()->set_validNumGoods(42);
	info.mutable_itemPackage()->set_validNumGem(21);
	info.mutable_itemPackage()->set_durability(Config_MaxDurability(info.itemPackage().vip()));
	const BaseWing *wing = EquipmentInfoManager_BaseWing(info.att().fightAtt().baseWingLevel(), info.att().fightAtt().baseWingDegree());
	assert(wing != NULL);
	for (int i = 0; i < wing->att_size(); i++)
		info.mutable_att()->mutable_fightAtt()->mutable_propertiesDelta(i)->set_delta(wing->att(i));
	info.mutable_att()->mutable_fightAtt()->set_transformID(0);
	TransformAsset *asset = info.mutable_itemPackage()->mutable_transforms(0);
	asset->set_id(0);
	asset->set_quality(0);
	asset->set_level(0);

	{
		info.mutable_att()->mutable_baseAtt()->set_professionType(ProfessionInfo::KNIGHT);
		info.mutable_att()->mutable_baseAtt()->set_male(true);
		info.mutable_att()->mutable_baseAtt()->set_height(2.5f);
		info.mutable_att()->mutable_movementAtt()->set_moveSpeed(6.0f);
		info.mutable_att()->mutable_movementAtt()->set_radius(1);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ATK, 51);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DEF, 26);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::MAXHP, 430);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ACCURACY, 24);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::CRIT, 34);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DODGE, 25);
		info.mutable_att()->mutable_fightAtt()->set_hp(info.mutable_att()->fightAtt().properties(FightAtt::MAXHP));
		info.mutable_att()->mutable_fightAtt()->set_mana(Config_MaxMana());
		ClearSkill(&info);
		AddSkill(&info, 42);
		AddSkill(&info, 3);
		AddSkill(&info, 4);
		AddSkill(&info, 5);
		AddSkill(&info, 6);
		AddSkill(&info, 7);
		package.pool.push_back(info);
	}
	{
		info.mutable_att()->mutable_baseAtt()->set_professionType(ProfessionInfo::RANGER);
		info.mutable_att()->mutable_baseAtt()->set_male(false);
		info.mutable_att()->mutable_baseAtt()->set_height(2.2f);
		info.mutable_att()->mutable_movementAtt()->set_moveSpeed(6.0f);
		info.mutable_att()->mutable_movementAtt()->set_radius(1);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ATK, 49);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DEF, 23);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::MAXHP, 395);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ACCURACY, 32);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::CRIT, 46);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DODGE, 32);
		info.mutable_att()->mutable_fightAtt()->set_hp(info.mutable_att()->fightAtt().properties(FightAtt::MAXHP));
		info.mutable_att()->mutable_fightAtt()->set_mana(Config_MaxMana());
		ClearSkill(&info);
		AddSkill(&info, 8);
		AddSkill(&info, 11);
		AddSkill(&info, 12);
		AddSkill(&info, 13);
		AddSkill(&info, 14);
		AddSkill(&info, 15);
		package.pool.push_back(info);
	}
	{
		info.mutable_att()->mutable_baseAtt()->set_professionType(ProfessionInfo::MAGICIAN);
		info.mutable_att()->mutable_baseAtt()->set_male(false);
		info.mutable_att()->mutable_baseAtt()->set_height(1.9f);
		info.mutable_att()->mutable_movementAtt()->set_moveSpeed(6.0f);
		info.mutable_att()->mutable_movementAtt()->set_radius(1);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ATK, 49);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DEF, 22);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::MAXHP, 385);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ACCURACY, 30);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::CRIT, 39);
		info.mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DODGE, 29);
		info.mutable_att()->mutable_fightAtt()->set_hp(info.mutable_att()->fightAtt().properties(FightAtt::MAXHP));
		info.mutable_att()->mutable_fightAtt()->set_mana(Config_MaxMana());
		ClearSkill(&info);
		AddSkill(&info, 16);
		AddSkill(&info, 19);
		AddSkill(&info, 20);
		AddSkill(&info, 21);
		AddSkill(&info, 22);
		AddSkill(&info, 23);
		package.pool.push_back(info);
	}
}

const PlayerAtt * ProfessionInfoManager_ProfessionInfo(ProfessionInfo::Type type, bool male) {
	for (size_t i = 0; i < package.pool.size(); i++) {
		if (type == package.pool[i].att().baseAtt().professionType() && male == package.pool[i].att().baseAtt().male())
			return &package.pool[i];
	}

	return NULL;
}
