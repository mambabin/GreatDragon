#ifndef _FIGHT_HPP_
#define _FIGHT_HPP_

#include "Component.hpp"
#include "FightInfo.hpp"
#include "Movement.hpp"
#include "PlayerEntity.hpp"
#include "SkillInfo.pb.h"
#include "SkillEntity.hpp"
#include "Math.hpp"
#include "ItemInfo.hpp"
#include "NetProto.pb.h"
#include "MapInfo.pb.h"
#include <sys/types.h>


struct Fight;

void Fight_Init();

struct Fight * Fight_Create(FightAtt *att, struct Component *component);
bool Fight_IsValid(struct Fight *fight);
void Fight_Destroy(struct Fight *fight);
void Fight_Finalize(struct Fight *fight);

void Fight_Prepare(struct Fight *fight);

const FightAtt * Fight_Att(struct Fight *fight);

struct Component * Fight_Component(struct Fight *fight);

// Only used while changing scene.
void Fight_Idle(struct Fight *fight);

void Fight_Cancel(struct Fight *fight);

void Fight_EnhanceByPet(struct Fight *fight, bool add);

int32_t Fight_Power(const PB_FightAtt *att);
int32_t Fight_Power(struct Fight *fight);
int32_t Fight_PetPower(const PB_PlayerAtt *att, int index);

void Fight_ModifyHP(struct Fight *fight, struct Fight *maker, int32_t delta);
void Fight_ModifyMana(struct Fight *fight, struct Fight *maker, int32_t delta);
void Fight_ModifyEnergy(struct Fight *fight, struct Fight *maker, int32_t delta);

void Fight_ModifyExp(struct Fight *fight, int64_t delta);

void Fight_SetFaction(struct Fight *fight, int32_t self, int32_t friendly);

void Fight_SetGodTarget(struct Fight *fight, int64_t target, int32_t power, int32_t score);
int32_t Fight_GodTargetScore(struct Fight *fight);

void Fight_SetFightPet(struct Fight *fight, int32_t index);
void Fight_SetAttachPet(struct Fight *fight, int32_t index);
void Fight_SetPetRest(struct Fight *fight, bool flag);

bool Fight_ResetCount(struct Fight *fight, NetProto_ResetCount::Type type, int arg);

void Fight_AddTower(struct Fight *fight);
//void Fight_DropTower(struct Fight *fight);

void Fight_AddSurvive(struct Fight *fight, int group);

int Fight_SetPKTarget(struct Fight *fight, int64_t target);
int64_t Fight_PKTarget(struct Fight *fight);
bool Fight_DoPK(struct Fight *fight);

// -1: Failure.
// >=0: Used money.
int64_t Fight_SkillLevelUp(struct Fight *fight, int32_t index, int32_t delta);
// -1: Failure.
int Fight_ClearSkill(struct Fight *fight, int index);

bool Fight_IsEnemy(struct Fight *fight, struct Fight *defender);
void Fight_AddEnemy(struct Fight *fight, struct Fight *enemy);
void Fight_DelEnemy(struct Fight *fight, struct Fight *enemy);
void Fight_ClearEnemy(struct Fight *fight);

bool Fight_IsCDOver(struct Fight *fight, const SkillInfo *skill, int64_t cur);

int Fight_Skills(struct Fight *fight, struct SkillEntity **skills, size_t size);
int Fight_BindSkill(struct Fight *fight, struct SkillEntity *skill, int32_t id);
void Fight_DelSkill(struct Fight *fight, struct SkillEntity *skill);
int32_t Fight_SkillClientID(struct Fight *fight, struct SkillEntity *skill);
bool Fight_HasSkillLaunchType(struct Fight *fight, SkillInfo::LaunchType launchType);
bool Fight_HasSkillLaunchTypes(struct Fight *fight, SkillInfo::LaunchType launchTypes[], size_t size);
void Fight_ClearSkills(struct Fight *fight);

bool Fight_HasSkill(struct Fight *fight, const SkillInfo *skill);
const SkillInfo * Fight_NextSkill(struct Fight *fight, const SkillInfo *root);

bool Fight_CantAttack(struct Fight *fight);
void Fight_SetCantAttack(struct Fight *fight, bool cantAttack);

bool Fight_CantBeAttacked(struct Fight *fight);
void Fight_SetCantBeAttacked(struct Fight *fight, bool cantBeAttacked);

bool Fight_IsSpecial(struct Fight *fight);
void Fight_SetSpecial(struct Fight *fight, bool special);
void Fight_AddSpecialProtect(struct Fight *fight, int v);

int32_t Fight_FinalProperty(const PB_FightAtt *att, PB_FightAtt::PropertyType type);
int32_t Fight_FinalProperty(const FightAtt *att, FightAtt::PropertyType type);
int32_t Fight_FinalProperty(struct Fight *fight, FightAtt::PropertyType att);
void Fight_ModifyProperty(struct Fight *fight, FightAtt::PropertyType att, int point, bool adjust = true);
void Fight_ModifyPropertyDelta(struct Fight *fight, FightAtt::PropertyType att, int32_t delta, float percent, bool adjust = true);

// 0: Succeed.
// 1: Is near, call Fight_Update.
// -1: Failure.
int Fight_Attack(struct Fight *fight, const SkillInfo *skill, struct Fight *tFight, Vector3f *tPos, int64_t cur);

int Fight_DoAttack(struct Fight *fight, const int32_t *skills, size_t size, int64_t cur);
void Fight_DestroyClientSkills(struct Fight *fight, const int32_t *skills, size_t size);

void Fight_Die(struct Fight *fight, struct Fight *killer);

void Fight_Revive(struct Fight *fight, const Vector2i *coord, float hp, float mana, bool free = false);
void Fight_RevivePetInSingle(struct Fight *fight);

int Fight_Transform(struct Fight *fight, bool transform);
bool Fight_IsTransforming(struct Fight *fight);
int Fight_AddBloodNode(struct Fight *fight, NetProto_AddBloodNode::Type type);
int32_t Fight_AddBloodEffect(struct Fight *fight, int soul, bool all, int *count);
int32_t Fight_UnlockBlood(struct Fight *fight);
int32_t Fight_SkillAreaDelta(struct Fight *fight, int32_t id);
int32_t Fight_SkillIncMana(struct Fight *fight);
int32_t Fight_SkillEnergyDelta(struct Fight *fight, int32_t id);

void Fight_EnterScene(struct Fight *fight, const MapInfo *info);

// <0: error
// 0: success, need to send NetProto_ClearRoom
// 1: success, no need to send NetProto_ClearRoom
int32_t Fight_ClearRoom(struct Fight *fight, bool win, int group, NetProto_ClearRoom *award = NULL);
bool Fight_HasClearedRoom(struct Fight *fight);
int Fight_OpenRoomBox(struct Fight *fight, bool free, int32_t *results, size_t size);

int Fight_Recover(struct Fight *fight);

int Fight_QuickFight(struct Fight *fight, int32_t room, int32_t count);

int Fight_Timeout(struct Fight *fight);

// -1: error
// 0: enqueue
// 1: done
int Fight_BeginWaitPVP(struct Fight *fight);
void Fight_EndWaitPVP(struct Fight *fight);
int Fight_PVPWaitCount();

// -1: error
// 0: enqueue
// 1: done
int Fight_BeginWaitHell(struct Fight *fight);
void Fight_EndWaitHell(struct Fight *fight);
int Fight_HellWaitCount();

void Fight_SetIgnorePK(struct Fight *fight, bool value);

int Fight_StrongBaseWing(struct Fight *fight, bool useRMB);

void Fight_Update(struct Fight *fight);

bool Fight_RestCheckPoint(struct Fight *fight);
void Fight_TransformWar(struct Fight* fight, int id);

int Fight_DropItem(struct Fight *fight, int npc, int index, int id);
bool Fight_ToPetData(const PlayerAtt * playerAtt, int index, NPCAtt * att);

#endif
