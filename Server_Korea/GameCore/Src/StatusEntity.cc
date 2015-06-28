#include "StatusEntity.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "IDGenerator.hpp"
#include "Status.hpp"
#include "StatusInfo.pb.h"
#include "StatusInfoManager.hpp"
#include "NetProto.pb.h"
#include "PlayerEntity.hpp"
#include "Movement.hpp"
#include "Equipment.hpp"
#include "NPCEntity.hpp"
#include "GCAgent.hpp"
#include "MathUtil.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "Time.hpp"
#include "Timer.hpp"
#include "Debug.hpp"
#include "SkillEntity.hpp"
#include "Item.hpp"
#include "GoodsInfoManager.hpp"
#include "GodShipInfoManager.hpp"
#include "FactionPool.hpp"
#include <sys/types.h>
#include <vector>
#include <cassert>
#include <algorithm>

using namespace std;

#define FEAR_RADIUS 5


struct StatusEntity{
	int32_t id;
	const StatusInfo *info;
	struct Status *status;
	struct Status *another;
	Vector3f skillPos;
	int64_t start;
	int count;
	vector<int32_t> delSkills;
	int64_t lastTime;
	bool first;
	const SkillInfo *skill;
	Vector2i fearCenter;
	int32_t fearTime;
	bool alive;
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct StatusEntity *> pool;
	vector<struct StatusEntity *> release;
}package;

void StatusEntity_Init() {
	package.max = Config_MaxStatuses();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
	package.release.clear();
}

static inline void DestroyReally(struct StatusEntity *entity) {
	IDGenerator_Release(package.idGenerator, entity->id);
}

static void ReleaseStatus(void *arg) {
	if (!package.release.empty()) {
		for (size_t i = 0; i < package.release.size(); i++)
			DestroyReally(package.release[i]);
		package.release.clear();
	}
}

void StatusEntity_Prepare() {
	Timer_AddEvent(0, 0, ReleaseStatus, NULL, "ReleaseStatus");
}

static int CreateGround(struct StatusEntity *entity, struct StatusEntity **statuses, size_t size) {
	if (Status_Add(entity->status, entity) == -1) {
		assert(0);
	}

	statuses[0] = entity;
	return 1;
}

static int CreateStandup(struct StatusEntity *entity, struct StatusEntity **statuses, size_t size) {
	return CreateGround(entity, statuses, size);
}

static int CreateDefault(struct StatusEntity *entity, struct Status *attacker, struct Status *defender, const Vector3f *skillPos, const StatusInfo *info, const SkillInfo *skill, struct StatusEntity **statuses, size_t size, int count, bool *resistControl) {
	if (Status_Add(entity->status, entity) == -1) {
		IDGenerator_Release(package.idGenerator, entity->id);
		DEBUG_LOGERROR("Failed to add status");
		return -1;
	}

	statuses[count++] = entity;

	StatusEntity_Update(entity);
	// SendAddStatus(entity);

	if (info->triggerType() == StatusInfo::HIT) {
		const StatusInfo *child = StatusInfoManager_StatusInfo(info->next());
		int res = StatusEntity_Create(attacker, defender, skillPos, info->targetType(), child, skill, statuses, size, count, resistControl);
		if (res != -1)
			count = res;
	}

	return count;
}

static bool CheckDependence(struct Status *attacker, struct Status *defender, StatusInfo::TargetType targetType, const StatusInfo *info, bool *resistControl) {
	struct Status *status = NULL;
	if (targetType == StatusInfo::ATTACKER)
		status = attacker;
	else if (targetType == StatusInfo::DEFENDER)
		status = defender;
	else
		assert(0);

	struct Component *component = Status_Component(status);

	if (Fight_CantBeAttacked(component->fight))
		return false;

	if (info->statusType() != StatusInfo::HURT) {
		if (Status_HasType(status, StatusInfo::GROUND)
				|| Status_HasType(status, StatusInfo::STANDUP))
			return false;
	}

	switch(info->statusType()) {
		case StatusInfo::SKY:
		case StatusInfo::FLYAWAY:
		case StatusInfo::STATIC:
			{
				/*
				if (component->npc != NULL) {
					const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(component->roleAtt->movementAtt().mapID()));
					const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
					if (mapUnit != NULL && NPCEntity_ID(component->npc) == mapUnit->boss()) {
						const FightAtt *att = Fight_Att(component->fight);
						if (att != NULL && att->status() == FightAtt::ATTACK)
							return false;
					}
				}
				*/
			}
			break;
		default:
			break;
	}

	switch(info->statusType()) {
		case StatusInfo::FLYAWAY:
		case StatusInfo::SKY:
		case StatusInfo::STATIC:
		case StatusInfo::SPEED:
		case StatusInfo::CANTMOVE:
		case StatusInfo::GIDDY:
		case StatusInfo::FREEZE:
		case StatusInfo::FEAR:
			{
				if (Fight_IsSpecial(component->fight))
					return false;
				static SkillInfo::LaunchType types[] = {SkillInfo::GOSTRAIGHT, SkillInfo::LEAP, SkillInfo::ROTATION, SkillInfo::SEPARATION, SkillInfo::BACKSTAB, SkillInfo::BLINK};
				if (Fight_HasSkillLaunchTypes(component->fight, types, sizeof(types) / sizeof(types[0])))
					return false;
			}
			break;

		case StatusInfo::HURT:
			{
				static SkillInfo::LaunchType types[] = {SkillInfo::SEPARATION, SkillInfo::BACKSTAB, SkillInfo::BLINK};
				if (Fight_HasSkillLaunchTypes(component->fight, types, sizeof(types) / sizeof(types[0])))
					return false;
			}
			break;

		default:
			break;
	}

	switch(info->statusType()) {
		case StatusInfo::FLYAWAY:
			{
				if (Status_HasType(status, StatusInfo::FLYAWAY))
					return false;

				struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
				int count = Status_GetType(status, StatusInfo::STATIC, statuses, CONFIG_FIXEDARRAY);
				if (count > 0) {
					assert(count == 1);
					if (statuses[0]->another != attacker)
						return false;

					StatusEntity_Destroy(statuses[0]);
				}
				count = Status_GetType(status, StatusInfo::SKY, statuses, CONFIG_FIXEDARRAY);
				if (count > 0) {
					assert(count == 1);
					if (statuses[0]->another != attacker)
						return false;

					StatusEntity_Destroy(statuses[0]);
				}

				static StatusInfo::StatusType types[] = {StatusInfo::GROUND, StatusInfo::STANDUP, StatusInfo::CANTMOVE, StatusInfo::GIDDY, StatusInfo::FREEZE, StatusInfo::FEAR};
				Status_DelTypes(status, types, sizeof(types) / sizeof(types[0]));
			}
			break;

		case StatusInfo::SKY:
			{
				if (Status_HasType(status, StatusInfo::FLYAWAY) || Status_HasType(status, StatusInfo::SKY))
					return false;

				struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
				int count = Status_GetType(status, StatusInfo::STATIC, statuses, CONFIG_FIXEDARRAY);
				if (count > 0) {
					assert(count == 1);
					if (statuses[0]->another != attacker)
						return false;

					StatusEntity_Destroy(statuses[0]);
				}

				static StatusInfo::StatusType types[] = {StatusInfo::GROUND, StatusInfo::STANDUP, StatusInfo::CANTMOVE, StatusInfo::GIDDY, StatusInfo::FREEZE, StatusInfo::FEAR};
				Status_DelTypes(status, types, sizeof(types) / sizeof(types[0]));
			}
			break;

		case StatusInfo::STATIC:
			{
				if (Status_HasType(status, StatusInfo::FLYAWAY))
					return false;

				{
					static StatusInfo::StatusType types[] = {StatusInfo::CANTMOVE, StatusInfo::GIDDY, StatusInfo::FREEZE, StatusInfo::FEAR};
					if (Status_HasTypes(status, types, sizeof(types) / sizeof(types[0])))
						return false;
				}

				struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
				int count = Status_GetType(status, StatusInfo::STATIC, statuses, CONFIG_FIXEDARRAY);
				if (count > 0) {
					assert(count == 1);
					if (statuses[0]->another != attacker)
						return false;

					StatusEntity_Destroy(statuses[0]);
				}

				count = Status_GetType(status, StatusInfo::SKY, statuses, CONFIG_FIXEDARRAY);
				if (count > 0) {
					assert(count == 1);
					if (statuses[0]->another != attacker)
						return false;

					statuses[0]->start = Time_ElapsedTime();
					statuses[0]->lastTime = Config_StatusStaticSkyTime();
				} else {
					/*
					   if (Status_Component(attacker)->player != NULL && Status_Component(defender)->player != NULL)
					   return false;
					   */
				}

				{
					static StatusInfo::StatusType types[] = {StatusInfo::GROUND, StatusInfo::STANDUP};
					Status_DelTypes(status, types, sizeof(types) / sizeof(types[0]));
				}
			}
			break;

		case StatusInfo::GROUND:
		case StatusInfo::STANDUP:
			assert(!Status_HasType(status, StatusInfo::SKY) && !Status_HasType(status, StatusInfo::FLYAWAY));
			break;

		case StatusInfo::SPEED:
			{
				if (Status_HasType(status, StatusInfo::SKY)
						|| Status_HasType(status, StatusInfo::FLYAWAY))
					return false;

				struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
				int count = Status_GetType(status, info->statusType(), statuses, CONFIG_FIXEDARRAY);
				for (int i = 0; i < count; i++) {
					struct StatusEntity *entity = statuses[i];
					if (info->percent() >= -1.0f && info->percent() <= 0.0f) {
						if (entity->info->percent() >= -1.0f && entity->info->percent() <= 0.0f) {
							if (info->percent() < entity->info->percent()) {
								StatusEntity_Destroy(entity);
								return true;
							}
							else {
								return false;
							}
						}
					}
					else if (info->percent() > 0.0f) {
						if (entity->info->percent() > 0.0f) {
							if (info->percent() > entity->info->percent()) {
								StatusEntity_Destroy(entity);
								return true;
							}
							else {
								return false;
							}
						}
					}
					else {
						assert(0);
					}
				}
			}
			break;

		case StatusInfo::CANTMOVE:
		case StatusInfo::GIDDY:
		case StatusInfo::FREEZE:
		case StatusInfo::FEAR:
			{
				if (Status_HasType(status, StatusInfo::SKY) || Status_HasType(status, StatusInfo::FLYAWAY))
					return false;
				if (!Status_IsControlOver(status)) {
					*resistControl = true;
					return false;
				}
				static StatusInfo::StatusType types[] = {StatusInfo::CANTMOVE, StatusInfo::GIDDY, StatusInfo::FREEZE, StatusInfo::FEAR};
				if (Status_HasTypes(status, types, sizeof(types) / sizeof(types[0])))
					return false;

				Status_DelType(status, StatusInfo::STATIC);

				assert(!Status_HasType(status, info->statusType()));
			}
			break;

		default:
			break;
	}

	return true;
}

static int64_t LastTime(struct StatusEntity *entity) {
	switch(entity->info->statusType()) {
		case StatusInfo::STATIC:
			return Status_HasType(entity->status, StatusInfo::SKY) ? Config_StatusStaticSkyTime() : Config_StatusStaticTime();
		case StatusInfo::FLYAWAY:
			return Config_StatusFlyAwayTime();
		case StatusInfo::SKY:
			return Config_StatusSkyTime();
		case StatusInfo::GROUND:
			return Config_StatusGroundTime();
		case StatusInfo::STANDUP:
			return Config_StatusStandupTime();
		default:
			break;
	}

	return (int64_t)entity->info->interval();
}

int StatusEntity_Create(struct Status *attacker, struct Status *defender, const Vector3f *skillPos, StatusInfo::TargetType targetType, const StatusInfo *info, const SkillInfo *skill, struct StatusEntity **statuses, size_t size, int zero, bool *resistControl) {
	if (attacker == NULL || defender == NULL || info == NULL || statuses == NULL || resistControl == NULL)
		return -1;

	if (zero >= (int)size)
		return -1;

	if (info->statusType() != StatusInfo::ROOM_EXP)
		if (!Time_CanPass(info->rate()))
			return -1;

	if (!CheckDependence(attacker, defender, targetType, info, resistControl))
		return -1;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1) {
		DEBUG_LOGERROR("Failed to create status");
		return -1;
	}

	struct StatusEntity *entity = SearchBlock(package.pool, id);
	entity->id = id;
	entity->info = info;
	if (targetType == StatusInfo::ATTACKER) {
		entity->status = attacker;
		entity->another = defender;
	}
	else if (targetType == StatusInfo::DEFENDER) {
		entity->status = defender;
		entity->another = attacker;
	}
	else {
		assert(0);
	}
	if (skillPos != NULL)
		entity->skillPos = *skillPos;
	entity->start = Time_ElapsedTime();
	entity->count = 0;
	entity->delSkills.clear();
	entity->lastTime = LastTime(entity);
	entity->first = true;
	entity->skill = skill;
	entity->alive = true;

	switch(info->statusType()) {
		case StatusInfo::GROUND:
			return CreateGround(entity, statuses, size);

		case StatusInfo::STANDUP:
			return CreateStandup(entity, statuses, size);

		default:
			return CreateDefault(entity, attacker, defender, skillPos, info, skill, statuses, size, zero, resistControl);
	}

	return -1;
}

static inline bool IsValid(struct StatusEntity *entity) {
	return entity != NULL && entity->alive && IDGenerator_IsValid(package.idGenerator, entity->id);
}

bool StatusEntity_ToAddStatus(struct StatusEntity *entity, NetProto_AddStatus *addStatus) {
	if (!IsValid(entity) || addStatus == NULL)
		return false;

	addStatus->Clear();
	addStatus->set_statusID(entity->info->id());
	struct Component *component = Status_Component(entity->status);
	if (component->player != NULL) {
		addStatus->set_type(NetProto_AddStatus::PLAYER);
		addStatus->set_targetID(PlayerEntity_ID(component->player));
	}
	else if (component->npc != NULL) {
		addStatus->set_type(NetProto_AddStatus::NPC);
		addStatus->set_targetID(NPCEntity_ID(component->npc));
	}
	else {
		assert(0);
	}

	struct Component *aComponent = Status_Component(entity->another);
	if (aComponent->player != NULL)
		addStatus->set_aID(PlayerEntity_ID(aComponent->player));
	else
		addStatus->set_aID(-1);

	addStatus->set_time((int32_t)entity->lastTime);
	Movement_Att(component->movement)->position().ToPB(addStatus->mutable_dest());
	for (size_t i = 0; i < entity->delSkills.size(); i++) {
		int32_t id = entity->delSkills[i];
		if (id == -1) {
			DEBUG_LOGERROR("Failed to del launched skill.");
			continue;
		}

		struct SkillEntity *skill = SkillEntity_Skill(id);
		if (skill == NULL) {
			DEBUG_LOGERROR("Failed to del launched skill.");
			continue;
		}

		struct Fight *owner = SkillEntity_Owner(skill);
		if (!Fight_IsValid(owner)) {
			DEBUG_LOGERROR("Owenr of del skill is not valid");
			SkillEntity_Destroy(skill);
			continue;
		}

		struct Component *component = Fight_Component(owner);
		if (component->player != NULL) {
			int32_t clientID = Fight_SkillClientID(owner, skill);
			if (clientID == -1) {
				DEBUG_LOG("Client id of del skill is not valid");
				SkillEntity_Destroy(skill);
				continue;
			}

			addStatus->add_delSkills(clientID);
		}
		else if (component->npc != NULL) {
			addStatus->add_delSkills(id);
		}
		else {
			assert(0);
		}

		SkillEntity_Destroy(skill);
	}

	return true;
}

static void DestroyStatic(struct StatusEntity *entity) {
	struct Component *component = Status_Component(entity->status);
	const MovementAtt *att = Movement_Att(component->movement);

	static NetProto_StaticOver staticOver;
	staticOver.Clear();

	if (component->player != NULL) {
		staticOver.set_type(NetProto_StaticOver::PLAYER);
		staticOver.set_id(PlayerEntity_ID(component->player));
	}
	else if (component->npc != NULL) {
		staticOver.set_type(NetProto_StaticOver::NPC);
		staticOver.set_id(NPCEntity_ID(component->npc));
	}
	else {
		assert(0);
	}

	GCAgent_SendProtoToAllClientsInSameScene(att->mapID(), component->player, &staticOver);
}

static void AddGround(struct StatusEntity *entity) {
	if (!Status_IsValid(entity->status))
		return;
	if (Fight_Att(Status_Component(entity->status)->fight)->status() == FightAtt::DEAD)
		return;

	static StatusInfo info;
	info.set_statusType(StatusInfo::GROUND);
	info.set_rate(1);

	StatusEntity *status;
	bool resistControl = false;
	if (StatusEntity_Create(entity->status, entity->status, &entity->skillPos, StatusInfo::DEFENDER, &info, entity->skill, &status, 1, 0, &resistControl) == -1) {
		// assert(0);
		return;
	}
}

static void AddStandup(struct StatusEntity *entity) {
	if (!Status_IsValid(entity->status))
		return;
	if (Fight_Att(Status_Component(entity->status)->fight)->status() == FightAtt::DEAD)
		return;

	static StatusInfo info;
	info.set_statusType(StatusInfo::STANDUP);
	info.set_rate(1);

	StatusEntity *status;
	bool resistControl = false;
	if (StatusEntity_Create(entity->status, entity->status, &entity->skillPos, StatusInfo::DEFENDER, &info, entity->skill, &status, 1, 0, &resistControl) == -1) {
		// assert(0);
		return;
	}

	static NetProto_Standup standup;
	standup.Clear();

	struct Component *component = Status_Component(entity->status);
	const MovementAtt *att = Movement_Att(component->movement);
	if (component->player != NULL) {
		standup.set_type(NetProto_Standup::PLAYER);
		standup.set_id(PlayerEntity_ID(component->player));
	}
	else if (component->npc != NULL) {
		standup.set_type(NetProto_Standup::NPC);
		standup.set_id(NPCEntity_ID(component->npc));
	}
	else {
		assert(0);
	}

	GCAgent_SendProtoToAllClientsInSameScene(att->mapID(), component->player, &standup);
}

void StatusEntity_Destroy(struct StatusEntity *entity) {
	if (!IsValid(entity))
		return;

	Status_Del(entity->status, entity);
	entity->alive = false;
	package.release.push_back(entity);
}

const StatusInfo * StatusEntity_Info(struct StatusEntity *entity) {
	if (!IsValid(entity))
		return NULL;

	return entity->info;
}

static bool IsCriting(struct StatusEntity *entity, double percent) {
	int critdef = 0;
	critdef = StatusEntity_Critdef(entity); 
	DEBUG_LOG("Critdef: %d", critdef);

	int32_t crit = Fight_FinalProperty(Status_Component(entity->another)->fight, FightAtt::CRIT) * (1 + percent);
	DEBUG_LOG("CritBefore: %d", crit);
	crit = crit - critdef;
	DEBUG_LOG("CritAfter: %d", crit);
	int32_t tLevel = Fight_Att(Status_Component(entity->status)->fight)->level();
	return Time_CanPass(crit / (tLevel * 200.0f));
}

static bool IsDodging(struct StatusEntity *entity) {
	int32_t dodge = Fight_FinalProperty(Status_Component(entity->status)->fight, FightAtt::DODGE);
	int32_t accuracy = Fight_FinalProperty(Status_Component(entity->another)->fight, FightAtt::ACCURACY);
	int32_t aLevel = Fight_Att(Status_Component(entity->another)->fight)->level();
	int32_t dLevel = Fight_Att(Status_Component(entity->status)->fight)->level();
	float rate = (dodge / aLevel - accuracy / dLevel) / 100.0f + 0.05f;
	if (rate < 0)
		rate = 0;
	else if (rate > 0.85f)
		rate = 0.85f;
	return Time_CanPass(rate);
}

static void GenStatus(struct StatusEntity *entity) {
	struct Component *attacker = Status_Component(entity->another);
	struct Component *defender = Status_Component(entity->status);

	if (entity->info->statusType() == StatusInfo::HURT && entity->info->count() <= 1) {
		if (attacker->player != NULL) {
			int32_t fashion = attacker->roleAtt->equipmentAtt().fashion();
			if (fashion != -1) {
				Equipment_AddStatusToBody(attacker->equipment, fashion, StatusInfo::SELF_ATT);
				Equipment_AddStatusToBody(attacker->equipment, fashion, StatusInfo::IGNORE_DEF);
			}
		}
		if (defender->player != NULL) {
			int32_t fashion = defender->roleAtt->equipmentAtt().fashion();
			if (fashion != -1) {
				Equipment_AddStatusToBody(defender->equipment, fashion, StatusInfo::DEC_HURT);
			}
		}
	}
}

static int Hurt(struct StatusEntity *entity, bool *isCriting) {
	struct Component *attacker = Status_Component(entity->another);
	struct Component *defender = Status_Component(entity->status);
	float attack = 0.0f;
	float defense = 0.0f;

	GenStatus(entity);

	switch(entity->info->hurtType()) {
		case StatusInfo::NORMAL:
			{
				attack = Fight_FinalProperty(attacker->fight, FightAtt::ATK);
				defense = Fight_FinalProperty(defender->fight, FightAtt::DEF);
			}
			break;
		case StatusInfo::ALWAYS:
			{
				attack = Fight_FinalProperty(attacker->fight, FightAtt::ATK);
				defense = 0;
			}
			break;
		default:
			assert(0);
	}

	struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
	int count = Status_GetType(attacker->status, StatusInfo::IGNORE_DEF, statuses, CONFIG_FIXEDARRAY);
	if (count > 0) {
		int v = 0;
		for (int i = 0; i < count; i++) {
			struct StatusEntity *status = statuses[i];
			const StatusInfo *info = StatusEntity_Info(status);
			v += (int)(defense * info->percent()) + info->value();
		}
		defense -= v;
		if (defense < 0)
			defense = 0;
	}

	float factor = entity->info->percent();
	int32_t extra = entity->info->value();
	// attack = attack * factor + extra;

	int32_t aLevel = Fight_Att(attacker->fight)->level();
	int32_t dLevel = Fight_Att(defender->fight)->level();

	float hurt = (Time_Random(95, 106) / 100.0f) * max(attack - defense, 0.0f) * max(1 + (aLevel - dLevel) / 100.0f, 0.0f);
	if (hurt < attack * 0.05f)
		hurt = attack * 0.05f;

	hurt = hurt * factor + extra;

	count = Status_GetType(defender->status, StatusInfo::DEC_HURT, statuses, CONFIG_FIXEDARRAY);
	if (count > 0) {
		int v = 0;
		for (int i = 0; i < count; i++) {
			struct StatusEntity *status = statuses[i];
			const StatusInfo *info = StatusEntity_Info(status);
			v += (int)(hurt * info->percent()) + info->value();
		}
		hurt -= v;
		if (hurt < 0)
			hurt = 0;
	}

	int inspireATK = 0;
	if (attacker->playerAtt != NULL) {
		int32_t map = attacker->roleAtt->movementAtt().mapID();
		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
		if (mapInfo != NULL) {
			int index = -1;
			if (mapInfo->mapType() == MapInfo::PVP)
				index = 17;
			else if (mapInfo->mapType() == MapInfo::HELL)
				index = 18;
			if (index != -1)
				inspireATK = attacker->playerAtt->fixedEvent(index) & 0xff;
		}
	}
	int inspireDEF = 0;
	if (defender->playerAtt != NULL) {
		int32_t map = defender->roleAtt->movementAtt().mapID();
		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
		if (mapInfo != NULL) {
			int index = -1;
			if (mapInfo->mapType() == MapInfo::PVP)
				index = 17;
			else if (mapInfo->mapType() == MapInfo::HELL)
				index = 18;
			if (index != -1)
				inspireDEF = (defender->playerAtt->fixedEvent(index) & 0xff00) >> 8;
		}
	}

	static int vData1[2];
	static double vData2[2];
	for (int i = 0; i < 2; ++i) {
		vData1[i] = 0;
		vData2[i] = 0;
	}

	if (attacker != NULL && attacker->player != NULL && attacker->playerAtt != NULL) {
		FactionPool_GetFactionGurdianData(attacker->playerAtt->faction(), vData1, vData2, true);
	}
	if (defender != NULL && defender->player != NULL && defender->playerAtt != NULL) {
		FactionPool_GetFactionGurdianData(defender->playerAtt->faction(), vData1, vData2, false);
	}
	
	hurt = hurt * (1.0f + inspireATK * 0.1f - inspireDEF * 0.05f);
	hurt = hurt * (1.0f + vData2[0]) + vData1[0];

	hurt = hurt * (1 + vData2[0]) + vData1[0];

	DEBUG_LOG("hurt: %d", hurt);
	int critdamage = 0;
	critdamage = StatusEntity_Critdamage(entity);
	DEBUG_LOG("critdamage: %d", critdamage);
	hurt = hurt + critdamage / (aLevel * 100);
	DEBUG_LOG("hurt: %d", hurt);

	*isCriting = IsCriting(entity, vData2[1]);
	if (*isCriting) {
		hurt = hurt * 2 - vData1[1];
	}

	if (defender->npc != NULL) {
		const NPCAtt *npcAtt = NPCEntity_Att(defender->npc);
		if (npcAtt != NULL) {
			if (npcAtt->newSpecial()) {
				if (Fight_IsSpecial(defender->fight)) {
					float protect = hurt * 0.8f;
					hurt *= 0.2f;
					Fight_AddSpecialProtect(defender->fight, (int)protect);
				}
			}
		}
	}

	return -max((int)hurt, 1);
}

static void UpdateHurt(struct StatusEntity *entity) {
	if (entity->first) {
		entity->first = false;
		return;
	}

	if (entity->count >= entity->info->count()) {
		StatusEntity_Destroy(entity);
		return;
	}

	if (!Status_IsValid(entity->status) || !Status_IsValid(entity->another)) {
		StatusEntity_Destroy(entity);
		return;
	}

	if (entity->info->count() > 1) {
		int64_t cur = Time_ElapsedTime();
		if (cur - entity->start < entity->lastTime * entity->count)
			return;
	}

	static NetProto_StatusEffect statusEffect;
	statusEffect.Clear();
	statusEffect.set_statusType((StatusInfo::StatusType)entity->info->statusType());
	statusEffect.set_skillID(entity->skill == NULL ? -1 : entity->skill->id());

	struct Component *component = Status_Component(entity->status);
	if (component->player != NULL) {
		statusEffect.set_type(NetProto_StatusEffect::PLAYER);
		statusEffect.set_id(PlayerEntity_ID(component->player));
	}
	else if (component->npc != NULL) {
		statusEffect.set_type(NetProto_StatusEffect::NPC);
		statusEffect.set_id(NPCEntity_ID(component->npc));
	}
	else {
		assert(0);
	}

	int32_t delta = 0;
	bool isCriting = false, isDodging = false;

	if (entity->count == 0) {
		isDodging = IsDodging(entity);
		if (isDodging) {
			delta = 0;
		}
		else {
			delta = Hurt(entity, &isCriting);
			assert(delta < 0);
		}
	}
	else {
		delta = Hurt(entity, &isCriting);
		assert(delta < 0);
	}

	if (delta == 0 && entity->count == 0) {
		entity->count = entity->info->count();
	}
	else {
		entity->count++;
	}

	int32_t mapID = Movement_Att(component->movement)->mapID();

	Fight_ModifyHP(component->fight, Status_Component(entity->another)->fight, delta);
	statusEffect.set_value(delta);
	if (isDodging)
		statusEffect.set_effect(NetProto_StatusEffect::DODGE);
	else if (isCriting)
		statusEffect.set_effect(NetProto_StatusEffect::CRIT);
	else
		statusEffect.set_effect(NetProto_StatusEffect::NONE);
	GCAgent_SendProtoToAllClientsInSameScene(mapID, component->player, &statusEffect);
}

static void UpdateRecover(struct StatusEntity *entity) {
	/*
	   if (entity->first) {
	   entity->first = false;
	   return;
	   }
	   */

	if (entity->count >= entity->info->count()) {
		StatusEntity_Destroy(entity);
		return;
	}

	if (entity->info->count() > 1) {
		int64_t cur = Time_ElapsedTime();
		if (cur - entity->start < entity->lastTime * entity->count)
			return;
	}

	static NetProto_StatusEffect statusEffect;
	statusEffect.Clear();
	statusEffect.set_statusType((StatusInfo::StatusType)entity->info->statusType());

	struct Component *component = Status_Component(entity->status);
	if (component->player != NULL) {
		statusEffect.set_type(NetProto_StatusEffect::PLAYER);
		statusEffect.set_id(PlayerEntity_ID(component->player));
	}
	else if (component->npc != NULL) {
		statusEffect.set_type(NetProto_StatusEffect::NPC);
		statusEffect.set_id(NPCEntity_ID(component->npc));
	}
	else {
		assert(0);
	}

	Fight_ModifyHP(component->fight, Status_Component(entity->another)->fight, entity->info->value());

	statusEffect.set_value(entity->info->value());
	GCAgent_SendProtoToAllClientsInSameScene(Movement_Att(component->movement)->mapID(), component->player, &statusEffect);

	entity->count++;
}

static void ComputeBreak(struct StatusEntity *entity) {
	struct Component *component = Status_Component(entity->status);
	if (component != NULL) {
		// if (component->player != NULL) {
		struct SkillEntity *skills[CONFIG_FIXEDARRAY];
		struct Fight *fight = component->fight;

		if (component->npc != NULL) {
			int32_t id = NPCEntity_ID(component->npc);
			const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(component->roleAtt->movementAtt().mapID()));
			const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
			if (mapUnit != NULL && id == mapUnit->boss())
				return;
		}

		int count = Fight_Skills(fight, skills, CONFIG_FIXEDARRAY);
		for (int i = 0; i < count; i++) {
			bool canBreak = false;
			if (SkillEntity_Status(skills[i]) == SkillInfo::BEGIN) {
				canBreak = true;
			}

			if (canBreak) {
				entity->delSkills.push_back(SkillEntity_ID(skills[i]));
				SkillEntity_Destroy(skills[i]);
			}
		}
		// }
	}
}

static void UpdateFlyAway(struct StatusEntity *entity, const Vector3f *origin) {
	if (entity->first) {
		const MovementAtt *myAtt = Movement_Att(Status_Component(entity->status)->movement);

		static Vector3f dir;
		dir.set_x(myAtt->position().x() - origin->x());
		dir.set_y(0.0f);
		dir.set_z(myAtt->position().z() - origin->z());
		Vector3f_Normalize(&dir);

		dir.set_x(dir.x() / 2);
		dir.set_z(dir.z() / 2);

		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(myAtt->mapID()));
		assert(mapInfo != NULL);
		static Vector3f delta;
		int dist = 1;
		for (; dist <= Config_StatusFlyAwayDist() * 2; dist++) {
			delta.set_x(myAtt->position().x() + dir.x() * dist);
			delta.set_z(myAtt->position().z() + dir.z() * dist);

			static Vector2i deltaCoord;
			MapInfoManager_LogicCoordByPosition(&delta, &deltaCoord);
			if (MapInfoManager_Unwalkable(mapInfo, &deltaCoord, myAtt->mapID()))
				break;
		}

		dir.set_x(dir.x() * (dist - 1));
		dir.set_z(dir.z() * (dist - 1));

		static Vector3f newPos;
		newPos.set_x(myAtt->position().x() + dir.x());
		newPos.set_y(myAtt->position().y());
		newPos.set_z(myAtt->position().z() + dir.z());

		struct Component *component = Status_Component(entity->status);
		Movement_SetPosition(component->movement, &newPos);

		ComputeBreak(entity);
		entity->first = false;
		return;
	}

	if (Time_ElapsedTime() - entity->start >= entity->lastTime) {
		StatusEntity_Destroy(entity);
		AddGround(entity);
	}
}

static void UpdateStatic(struct StatusEntity *entity) {
	if (entity->first) {
		ComputeBreak(entity);
		if (entity->skill != NULL) {
			if (entity->skill->dOffsetType() == SkillInfo::FRONT) {
				if (Status_IsValid(entity->status) && Status_IsValid(entity->another)) {
					struct Component *component = Status_Component(entity->status);
					const Vector3f *endPos = &Movement_Att(component->movement)->position();
					struct Component *aComponent = Status_Component(entity->another);
					const Vector3f *beginPos = Movement_OriginalOffsetPos(aComponent->movement);

					const MovementAtt *myAtt = Movement_Att(Status_Component(entity->status)->movement);
					const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(myAtt->mapID()));
					assert(mapInfo != NULL);

					Vector3f dir;
					dir.set_x(endPos->x() - beginPos->x());
					dir.set_z(endPos->z() - beginPos->z());
					Vector3f_Normalize(&dir);

					dir.set_x(dir.x() / 2);
					dir.set_z(dir.z() / 2);

					float dist = entity->skill->offsetDist();
					Vector3f dest = *endPos;
					for (int i = 1; i <= dist * 2; i++) {
						dest.set_x(endPos->x() + dir.x() * i);
						dest.set_z(endPos->z() + dir.z() * i);

						static Vector2i deltaCoord;
						MapInfoManager_LogicCoordByPosition(&dest, &deltaCoord);
						if (MapInfoManager_Unwalkable(mapInfo, &deltaCoord, myAtt->mapID()))
							break;
					}
					Movement_SetPosition(component->movement, &dest);
				}
			}
		}
		entity->first = false;
		return;
	}

	int64_t cur = Time_ElapsedTime();
	if (cur - entity->start >= entity->lastTime) {
		DestroyStatic(entity);
		StatusEntity_Destroy(entity);
	}
}

static void UpdateSky(struct StatusEntity *entity) {
	if (entity->first) {
		ComputeBreak(entity);
		entity->first = false;
		return;
	}

	int64_t cur = Time_ElapsedTime();
	if (cur - entity->start >= entity->lastTime) {
		StatusEntity_Destroy(entity);
		AddGround(entity);
	}
}

static void SendMove(struct Movement *movement, const Vector3f *dest) {
	struct Component *component = Movement_Component(movement);
	const MovementAtt *att = Movement_Att(component->movement);

	static NetProto_Move move;
	move.Clear();

	if (component->player != NULL) {
		move.set_type(NetProto_Move::PLAYER);
		move.set_id(PlayerEntity_ID(component->player));
	}
	else if (component->npc != NULL) {
		move.set_type(NetProto_Move::NPC);
		move.set_id(NPCEntity_ID(component->npc));
	}
	att->position().ToPB(move.mutable_start());
	dest->ToPB(move.mutable_end());
	GCAgent_SendProtoToAllClientsInSameScene(att->mapID(), component->player, &move);
}

static inline void RandomDest(const Vector2i *center, int radius, Vector2i *dest) {
	int x = Time_Random(0, radius * 2) - radius;
	int y = Time_Random(0, radius * 2) - radius;
	dest->set_x(center->x() + x);
	dest->set_y(center->y() + y);
}

static void UpdateFear(struct StatusEntity *entity) {
	struct Component *component = Status_Component(entity->status);

	if (entity->first) {
		entity->fearCenter = Movement_Att(component->movement)->logicCoord();
		entity->fearTime = entity->lastTime;
		entity->first = false;
	}
	else {
		int64_t cur = Time_ElapsedTime();
		if (cur - entity->start >= entity->lastTime) {
			StatusEntity_Destroy(entity);
			return;
		}
	}

	if (!Movement_IsValid(component->movement))
		return;
	if (Fight_CantBeAttacked(component->fight))
		return;
	if (Movement_Att(component->movement)->status() != MovementAtt::IDLE)
		return;

	Vector2i destCoord;
	RandomDest(&entity->fearCenter, FEAR_RADIUS, &destCoord);
	if (Movement_MoveStraightly(component->movement, &destCoord) == 0) {
		Vector3f destPos;
		MapInfoManager_PositionByLogicCoord(&destCoord, &destPos);
		SendMove(component->movement, &destPos);
	}
}

static void UpdateGround(struct StatusEntity *entity) {
	if (entity->first) {
		entity->first = false;
		return;
	}

	int64_t cur = Time_ElapsedTime();
	if (cur - entity->start >= entity->lastTime) {
		StatusEntity_Destroy(entity);
		AddStandup(entity);
	}
}

static void UpdateDefault(struct StatusEntity *entity) {
	if (entity->first) {
		entity->first = false;
		return;
	}

	int64_t cur = Time_ElapsedTime();
	if (cur - entity->start >= entity->lastTime) {
		StatusEntity_Destroy(entity);
	}
}

void StatusEntity_Update(struct StatusEntity *entity) {
	assert(IsValid(entity));

	switch(entity->info->statusType()) {
		case StatusInfo::HURT:
			UpdateHurt(entity);
			break;

		case StatusInfo::STATIC:
			UpdateStatic(entity);
			break;

		case StatusInfo::SKY:
			UpdateSky(entity);
			break;

		case StatusInfo::FLYAWAY:
			{
				UpdateFlyAway(entity, entity->first ? &Movement_Att(Status_Component(entity->another)->movement)->position() : NULL);
			}
			break;

		case StatusInfo::FEAR:
			UpdateFear(entity);
			break;

		case StatusInfo::RECOVER:
			UpdateRecover(entity);
			break;

		case StatusInfo::GROUND:
			UpdateGround(entity);
			break;

		case StatusInfo::ROOM_EXP:
			break;

		default:
			UpdateDefault(entity);
			break;
	}
}

int StatusEntity_Critdef(struct StatusEntity *entity) {
	if (entity == NULL)
		return 0;

	struct Component *component = Status_Component(entity->status);
	if (component == NULL)
		return 0;

	const ItemPackage *package = Item_Package(component->item);
	if (package == NULL)
		return 0;

	const EquipmentAtt *att =  Equipment_Att(component->equipment);
	if (att == NULL)
		return 0;

	int critdef = 0;

	for (int i = 0; i < att->godShips_size(); i++) {

		int index = att->godShips(i);
		if (index == -1)
			continue;

		const GodShipAsset *asset = &package->godShips(index);
		if (asset == NULL)
			continue;

		const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
		if (info == NULL)
			continue;

		critdef += info->CRITDEF();
	} 
	return critdef;
}

int StatusEntity_Critdamage(struct StatusEntity *entity) {
	if (entity == NULL)
		return 0;

	struct Component *component = Status_Component(entity->status);
	if (component == NULL)
		return 0;

	const ItemPackage *package = Item_Package(component->item);
	if (package == NULL)
		return 0;

	const EquipmentAtt *att =  Equipment_Att(component->equipment);
	if (att == NULL)
		return 0;

	int critdamage = 0;

	for (int i = 0; i < att->godShips_size(); i++) {

		int index = att->godShips(i);
		if (index == -1)
			continue;

		const GodShipAsset *asset = &package->godShips(index);
		if (asset == NULL)
			continue;

		const GodShip *info = GodShipInfoManager_GodShipInfo(asset->id());
		if (info == NULL)
			continue;

		critdamage += info->CRITDamage();
	} 
	return critdamage;
}
