#include "SkillEntity.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "IDGenerator.hpp"
#include "Fight.hpp"
#include "SkillPool.hpp"
#include "PlayerEntity.hpp"
#include "Time.hpp"
#include "SkillInfo.pb.h"
#include "Math.hpp"
#include "MathUtil.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "NPCEntity.hpp"
#include "StatusInfoManager.hpp"
#include "NetProto.pb.h"
#include "GCAgent.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <set>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace std;

#define HORLINE_HIT_INTERVAL 300

#define MANA_BE_HIT_INCREASE 3

struct SkillEntity{
	int32_t id;
	const SkillInfo *skill;
	SkillInfo::StatusType status;
	int32_t moveTime;
	Vector3f position;
	Vector2i prevCoord;
	Vector3f dir;
	struct Fight *owner;
	int64_t start;
	int32_t delay;
	int32_t mapID;
	struct Fight *tFight;
	Vector3f tPos;
	int count;
	int64_t beginIdle;
	map<struct Component *, int64_t> hitEntities;
	int hitNum;
};

static struct{
	vector<struct SkillEntity *> pool;
	int max;
	struct IDGenerator *idGenerator;
}package;


void SkillEntity_Init() {
	package.max = Config_MaxSkills();
	package.idGenerator = IDGenerator_Create(package.max, 0);
	package.pool.clear();
}

static int32_t MoveTime(const SkillInfo *skill, const Vector3f *pos, const Vector3f *tPos) {
	assert(tPos != NULL);

	int32_t moveTime = skill->moveTime();
	switch(skill->launchType()) {
		case SkillInfo::GOSTRAIGHT:
			{
				float dist = Vector3f_Distance(pos, tPos);
				moveTime = (int32_t)(dist / skill->speed() * 1000);
				if (moveTime > skill->moveTime())
					moveTime = skill->moveTime();
			}
			break;

		default:
			break;
	}

	return moveTime;
}

static struct SkillEntity * CreateEntity(const SkillInfo *skill, const Vector3f *pos, const Vector3f *dir, int32_t delay, struct Fight *owner, struct Fight *tFight, const Vector3f *tPos) {
	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct SkillEntity *entity = SearchBlock(package.pool, id);
	entity->id = id;
	entity->skill = skill;
	entity->status = SkillInfo::BEGIN;
	entity->position = *pos;
	entity->prevCoord.set_x(-1);
	entity->prevCoord.set_y(-1);
	entity->dir = *dir;
	entity->delay = delay;
	entity->owner = owner;
	entity->start = Time_ElapsedTime();
	entity->tFight = tFight;
	entity->tPos = *tPos;
	entity->mapID = Movement_Att(Fight_Component(owner)->movement)->mapID();
	entity->count = skill->unitCount();
	entity->hitEntities.clear();
	entity->hitNum = 0;
	entity->moveTime = MoveTime(skill, pos, tPos);

	SkillPool_Add(entity);

	return entity;
}

static inline void GenDir(const Vector3f *begin, const Vector3f *tPos, Vector3f *dir) {
	dir->set_x(tPos->x() - begin->x());
	dir->set_z(tPos->z() - begin->z());
	Vector3f_Normalize(dir);
}

static int CreateStatusSkill(const SkillInfo *skill, struct Fight *owner, struct Fight *tFight, const Vector3f *tPos, int32_t *ids, size_t size) {
	if (skill == NULL || owner == NULL)
		return -1;

	switch(skill->launchType()) {
		case SkillInfo::SINGLELINE:
			{
				if (size < 1)
					return -1;

				if (tPos == NULL) {
					if (tFight == NULL)
						return -1;

					tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				}
				tFight = NULL;
				/*
				   if (tFight != NULL || tPos == NULL)
				   return -1;
				 */

				static Vector3f dir;
				GenDir(&Movement_Att(Fight_Component(owner)->movement)->position(), tPos, &dir);

				const Vector3f *pos = &Movement_Att(Fight_Component(owner)->movement)->position();

				struct SkillEntity *entity = CreateEntity(skill, pos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;

				return 1;
			}
			break;

		case SkillInfo::GOSTRAIGHT:
			{
				if (size < 1)
					return -1;

				if (tPos == NULL)
					return -1;
				tFight = NULL;
				/*
				   if (tFight != NULL || tPos == NULL)
				   return -1;
				 */

				static Vector3f dir;
				GenDir(&Movement_Att(Fight_Component(owner)->movement)->position(), tPos, &dir);

				const Vector3f *pos = &Movement_Att(Fight_Component(owner)->movement)->position();

				struct SkillEntity *entity = CreateEntity(skill, pos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;

				return 1;
			}
			break;

		case SkillInfo::MULLINE:
			{
				if ((int)size < skill->unitCount())
					return -1;

				if (tPos == NULL) {
					if (tFight == NULL)
						return -1;

					tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				}
				tFight = NULL;
				/*
				if (tFight != NULL || tPos == NULL)
					return -1;
					*/

				static Vector3f dir;
				GenDir(&Movement_Att(Fight_Component(owner)->movement)->position(), tPos, &dir);

				static Vector3f dir2;
				dir2.set_x(-dir.z());
				dir2.set_y(0.0f);
				dir2.set_z(dir.x());

				Vector3f pos = Movement_Att(Fight_Component(owner)->movement)->position();
				Vector3f begin = pos;

				for (int i = 0; i < skill->unitCount(); i++) {
					if (i == 0) {
						struct SkillEntity *entity = CreateEntity(skill, &pos, &dir, skill->prepareTime(), owner, tFight, tPos);
						if (entity == NULL)
							return -1;

						ids[i] = entity->id;

						continue;
					}

					int factor = (i - 1) / 2 + 1;
					dir2.set_x(dir2.x() * factor);
					dir2.set_z(dir2.z() * factor);

					pos.set_x(pos.x() + dir2.x());
					pos.set_z(pos.z() + dir2.z());
					struct SkillEntity *entity = CreateEntity(skill, &pos, &dir, skill->prepareTime(), owner, tFight, tPos);
					if (entity == NULL)
						return -1;

					ids[i] = entity->id;

					pos = begin;
					dir2.set_x(-dir2.x());
					dir2.set_z(-dir2.z());
				}

				return skill->unitCount();
			}
			break;

		case SkillInfo::SECTORLINE:
			{
				if ((int)size < skill->unitCount())
					return -1;

				if (tPos == NULL) {
					if (tFight == NULL)
						return -1;

					tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				}
				tFight = NULL;
				/*
				if (tFight != NULL || tPos == NULL)
					return -1;
					*/

				static Vector3f pos;
				pos = Movement_Att(Fight_Component(owner)->movement)->position();

				static Vector3f dir;
				GenDir(&pos, tPos, &dir);

				static Vector3f dir2;
				dir2.set_x(-dir.z() * 2 / 3);
				dir2.set_y(0.0f);
				dir2.set_z(dir.x() * 2 / 3);

				static Vector3f front;
				front.set_x(pos.x() + dir.x());
				front.set_z(pos.z() + dir.z());
				static Vector3f begin;
				begin = front;

				for (int i = 0; i < skill->unitCount(); i++) {
					if (i == 0) {
						struct SkillEntity *entity = CreateEntity(skill, &pos, &dir, skill->prepareTime(), owner, tFight, tPos);
						if (entity == NULL)
							return -1;

						ids[i] = entity->id;
						continue;
					}

					int factor = (i - 1) / 2 + 1;
					dir2.set_x(dir2.x() * factor);
					dir2.set_z(dir2.z() * factor);

					front.set_x(front.x() + dir2.x());
					front.set_z(front.z() + dir2.z());
					GenDir(&pos, &front, &dir);

					struct SkillEntity *entity = CreateEntity(skill, &pos, &dir, skill->prepareTime(), owner, tFight, tPos);
					if (entity == NULL)
						return -1;

					ids[i] = entity->id;

					front = begin;
					dir2.set_x(-dir2.x());
					dir2.set_z(-dir2.z());
				}

				return skill->unitCount();
			}
			break;

		case SkillInfo::CONTINUOUSLINE:
			{
				if ((int)size < skill->unitCount())
					return -1;

				if (tPos == NULL) {
					if (tFight == NULL)
						return -1;

					tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				}
				tFight = NULL;
				/*
				if (tFight != NULL || tPos == NULL)
					return -1;
					*/

				static Vector3f dir;
				GenDir(&Movement_Att(Fight_Component(owner)->movement)->position(), tPos, &dir);

				const Vector3f *pos = &Movement_Att(Fight_Component(owner)->movement)->position();

				int delay = 300;
				for (int i = 0; i < skill->unitCount(); i++) {
					struct SkillEntity *entity = CreateEntity(skill, pos, &dir, skill->prepareTime() + delay * i, owner, tFight, tPos);
					if (entity == NULL)
						return -1;

					ids[i] = entity->id;
				}

				return skill->unitCount();
			}
			break;

		case SkillInfo::FALLDOWN:
		case SkillInfo::JUMP:
			{
				if (size < 1)
					return -1;

				if (tPos == NULL) {
					if (tFight == NULL)
						return -1;

					tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				}
				// tFight = NULL;
				/*
				   if (tFight != NULL || tPos == NULL)
				   return -1;
				 */

				static Vector3f dir;

				/*
				const Vector3f *pos = NULL;
				if (tFight != NULL)
					pos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				else if (tPos != NULL)
					pos = tPos;
				else
					return -1;
					*/

				struct SkillEntity *entity = CreateEntity(skill, /*pos*/tPos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;

				return 1;
			}
			break;

		case SkillInfo::BLINK:
			{
				if (size < 1)
					return -1;

				if (tPos == NULL)
					return -1;
				tFight = NULL;

				static Vector3f dir;
				static Vector3f pos;
				pos = Movement_Att(Fight_Component(owner)->movement)->position();

				struct SkillEntity *entity = CreateEntity(skill, &pos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;
				Fight_SetCantBeAttacked(owner, true);
				return 1;
			}
			break;

		case SkillInfo::LEAP:
			{
				if (size < 1)
					return -1;

				if (tPos == NULL) {
					if (tFight == NULL)
						return -1;

					tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				}
				// tFight = NULL;

				static Vector3f dir;
				struct SkillEntity *entity = CreateEntity(skill, /*pos*/tPos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;

				return 1;
			}
			break;

		case SkillInfo::SEPARATION:
			{
				if (size < 1)
					return -1;

				if (tFight == NULL)
					return -1;

				tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				tFight = NULL;

				static Vector3f dir;
				struct SkillEntity *entity = CreateEntity(skill, tPos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;
				Fight_SetCantBeAttacked(owner, true);
				return 1;
			}
			break;

		case SkillInfo::BACKSTAB:
			{
				if (size < 1)
					return -1;

				if (tFight == NULL)
					return -1;

				tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();

				static Vector3f dir;
				struct SkillEntity *entity = CreateEntity(skill, tPos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;
				Fight_SetCantBeAttacked(owner, true);
				return 1;
			}
			break;

		case SkillInfo::MELEE:
			{
				if (size < 1)
					return -1;

				if (tPos == NULL) {
					if (tFight == NULL)
						return -1;

					tPos = &Movement_Att(Fight_Component(tFight)->movement)->position();
				}
				// tFight = NULL;

				static Vector3f dir;
				static Vector3f pos;

				/*
				   if (tFight != NULL) {
				   pos = Movement_Att(Fight_Component(tFight)->movement)->position();
				   GenDir(&Movement_Att(Fight_Component(owner)->movement)->position(), &pos, &dir);
				   }
				   else if (tPos != NULL) {
				   */
				GenDir(&Movement_Att(Fight_Component(owner)->movement)->position(), tPos, &dir);
				// dir.set_x(dir.x() * skill->dist());
				// dir.set_z(dir.z() * skill->dist());

				// float extra = Movement_Att(Fight_Component(owner)->movement)->radius() * MAP_BLOCKSIZE;
				// const Vector3f *base = &Movement_Att(Fight_Component(owner)->movement)->position();
				// pos.set_x(base->x() + dir.x() * (skill->dist() + extra));
				// pos.set_x(base->x() + dir.x() * (1.0f + extra));
				// pos.set_y(0.0f);
				// pos.set_z(base->z() + dir.z() * (skill->dist() + extra));
				// pos.set_z(base->z() + dir.z() * (1.0f + extra));
				pos = *tPos;
				/*
				   }
				   else {
				   return -1;
				   }
				   */

				struct SkillEntity *entity = CreateEntity(skill, &pos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;

				return 1;
			}
			break;

		case SkillInfo::AROUNDSELF:
		case SkillInfo::ROTATION:
			{
				if (size < 1)
					return -1;

				// if (tFight != NULL)
				//	return -1;

				tFight = NULL;

				static Vector3f dir;
				tPos = &Movement_Att(Fight_Component(owner)->movement)->position();

				struct SkillEntity *entity = CreateEntity(skill, tPos, &dir, skill->prepareTime(), owner, tFight, tPos);
				if (entity == NULL)
					return -1;

				ids[0] = entity->id;

				return 1;
			}
			break;

		default:
			break;
	}

	return -1;
}

static void DestroyReally(struct SkillEntity *entity) {
	SkillPool_Del(entity);
	IDGenerator_Release(package.idGenerator, entity->id);
}

int SkillEntity_Create(const SkillInfo *skill, struct Fight *owner, struct Fight *tFight, const Vector3f *tPos, int32_t *ids, size_t size, const int32_t *skills, size_t count) {
	if (skill == NULL || owner == NULL || (tFight == NULL && tPos == NULL) || ids == NULL)
		return -1;

	if (skills == NULL) {
		if (Fight_Component(owner)->npc == NULL) {
			DEBUG_LOGERROR("NPC has allocated skill ids");
			return -2;
		}
	} else if (count > 0) {
		if (Fight_Component(owner)->player == NULL) {
			DEBUG_LOGERROR("Player has no allocated skill ids");
			return -3;
		}
	} else {
		DEBUG_LOGERROR("Skill ids from client are wrong");
		return -4;
	}

	if (skill->skillType() == SkillInfo::STATUS) {
		int res = CreateStatusSkill(skill, owner, tFight, tPos, ids, size);
		if (skills == NULL) {
			for (int i = 0; i < res; i++) {
				struct SkillEntity *skill = SkillEntity_Skill(ids[i]);
				Fight_BindSkill(owner, skill, -1);
			}
		} else {
			if (res != (int)count) {
				if (res != -1)
					DEBUG_LOG("The skill ids of client and server are diff, client: %d, server: %d, skillid: %d", count, res, skill->id());
				for (int i = 0; i < res; i++) {
					struct SkillEntity *skill = SkillEntity_Skill(ids[i]);
					DestroyReally(skill);
				}
				return -5;
			}
			for (int i = 0; i < res; i++) {
				struct SkillEntity *skill = SkillEntity_Skill(ids[i]);
				// DEBUG_LOG("bind id: %d to clientID: %d", skill->id, skills[i]);
				int ret = Fight_BindSkill(owner, skill, skills[i]);
				if (ret < 0) {
					DEBUG_LOG("Failed to bind skill, error: %d", ret);
					for (int j = 0; j < i; j++) {
						skill = SkillEntity_Skill(ids[j]);
						Fight_DelSkill(owner, skill);
					}
					for (int j = 0; j < res; j++) {
						skill = SkillEntity_Skill(ids[j]);
						DestroyReally(skill);
					}
					return -6;
				}
			}
		}

		return res;
	} else {
		assert(0);
	}

	return -7;
}

static inline bool IsValid(struct SkillEntity *entity) {
	return entity != NULL && IDGenerator_IsValid(package.idGenerator, entity->id);
}

int32_t SkillEntity_ID(struct SkillEntity *entity) {
	if (!IsValid(entity))
		return -1;

	return entity->id;
}

SkillInfo::StatusType SkillEntity_Status(struct SkillEntity *entity) {
	if (!IsValid(entity))
		return SkillInfo::IDLE;

	return entity->status;
}

struct Fight * SkillEntity_Owner(struct SkillEntity *entity) {
	if (!IsValid(entity))
		return NULL;

	return entity->owner;
}

int32_t SkillEntity_MapID(struct SkillEntity *entity) {
	if (!IsValid(entity))
		return -1;

	return entity->mapID;
}

const SkillInfo * SkillEntity_Info(struct SkillEntity *entity) {
	if (!IsValid(entity))
		return NULL;

	return entity->skill;
}

void SkillEntity_Destroy(struct SkillEntity *entity) {
	if (!IsValid(entity) || entity->status == SkillInfo::IDLE)
		return;

	if (Fight_IsValid(entity->owner)) {
		struct Component *component = Fight_Component(entity->owner);
		if (component->player != NULL) {
			int32_t id = Fight_SkillClientID(entity->owner, entity);
			assert(id != -1);

			static NetProto_DestroySkill destroySkill;
			destroySkill.Clear();
			destroySkill.add_skill(id);
			int playerID = PlayerEntity_ID(component->player);
			destroySkill.set_id(playerID);
			destroySkill.set_immediately(false);
			GCAgent_SendProtoToClients(&playerID, 1, &destroySkill);
		}

		if (entity->skill->launchType() == SkillInfo::SEPARATION
				|| entity->skill->launchType() == SkillInfo::BACKSTAB
				|| entity->skill->launchType() == SkillInfo::BLINK)
			Fight_SetCantBeAttacked(entity->owner, false);

		Fight_DelSkill(entity->owner, entity);
	}

	entity->status = SkillInfo::IDLE;
	entity->beginIdle = Time_ElapsedTime();
}

struct SkillEntity * SkillEntity_Skill(int32_t id) {
	if (!IDGenerator_IsValid(package.idGenerator, id))
		return NULL;

	return SearchBlock(package.pool, id);
}

static inline bool IsNear(const Vector3f *v1, const Vector3f *v2, int dist) {
	return Vector3f_Distance(v1, v2) < dist;
}

static bool CheckEdge(struct SkillEntity *entity) {
	const MapInfo * map = MapInfoManager_MapInfo(MapPool_MapInfo(entity->mapID));
	assert(map != NULL);
	static Vector2i coord;
	MapInfoManager_LogicCoordByPosition(&entity->position, &coord);
	return !MapInfoManager_Unskill(map, &coord);
}

static inline void DoMove(struct SkillEntity *entity) {
	entity->position.set_x(entity->position.x() + entity->skill->speed() * entity->dir.x() * ((float)Time_DeltaTime() / 1000));
	entity->position.set_z(entity->position.z() + entity->skill->speed() * entity->dir.z() * ((float)Time_DeltaTime() / 1000));

	if (entity->skill->launchType() == SkillInfo::GOSTRAIGHT) {
		if (CheckEdge(entity)) {
			struct Component *component = Fight_Component(entity->owner);
			Movement_SetPosition(component->movement, &entity->position);
		}
	}
}

static int32_t FinalSkillIncMana(struct SkillEntity *entity) {
	return -entity->skill->mp() + Fight_SkillIncMana(entity->owner);
}

static int32_t FinalSkillEnergy(struct SkillEntity *entity) {
	return entity->skill->energy() + Fight_SkillEnergyDelta(entity->owner, entity->skill->id());
}

static void SendHit(struct Component **components, size_t size, struct SkillEntity *entity, struct StatusEntity **statuses, int count, bool resistControl) {
	if (size <= 0 && !resistControl)
		return;

	static NetProto_Hit hit;
	hit.Clear();
	struct Component *owner = Fight_Component(SkillEntity_Owner(entity));
	if (owner->player != NULL) {
		int32_t id = Fight_SkillClientID(owner->fight, entity);
		assert(id != -1);
		hit.set_aSkillID(id);
		hit.set_aType(NetProto_Hit::PLAYER);
		hit.set_aID(PlayerEntity_ID(owner->player));
	}
	else if (owner->npc != NULL) {
		hit.set_aSkillID(entity->id);
		hit.set_aType(NetProto_Hit::NPC);
		hit.set_aID(NPCEntity_ID(owner->npc));
	}
	else {
		assert(0);
	}

	for (size_t i = 0; i < size; i++) {
		if (components[i]->player != NULL) {
			hit.add_dType(NetProto_Hit::PLAYER);
			hit.add_dID(PlayerEntity_ID(components[i]->player));
		}
		else if (components[i]->npc != NULL) {
			hit.add_dType(NetProto_Hit::NPC);
			hit.add_dID(NPCEntity_ID(components[i]->npc));
		}
		else {
			assert(0);
		}
		Fight_ModifyMana(components[i]->fight, NULL, MANA_BE_HIT_INCREASE);
	}
	if (size > 0) {
		if (entity->skill->mp() < 0)
			Fight_ModifyMana(owner->fight, NULL, FinalSkillIncMana(entity));
		if (!Fight_IsTransforming(owner->fight))
			Fight_ModifyEnergy(owner->fight, NULL, FinalSkillEnergy(entity));
	}

	for (int i = 0; i < count; i++) {
		if (!StatusEntity_ToAddStatus(statuses[i], hit.add_statuses())) {
			// DEBUG_LOGERROR("Failed to add status to proto");
			hit.mutable_statuses()->RemoveLast();
			continue;
		}
	}

	hit.set_resistControl(resistControl);
	GCAgent_SendProtoToAllClientsInSameScene(entity->mapID, Fight_Component(entity->owner)->player, &hit);
}

static int DoHit(struct Component *component, struct SkillEntity *entity, struct StatusEntity **statuses, size_t size, bool *resistControl) {
	if (entity->skill->maxHit() != -1) {
		if (entity->hitNum >= entity->skill->maxHit())
			return -1;
	}

	const MovementAtt *tAtt = Movement_Att(component->movement);
	const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(tAtt->mapID()));
	assert(mapInfo != NULL);
	const Vector2i *tCoord = &tAtt->logicCoord();
	if (MapInfoManager_Unwalkable(mapInfo, tCoord, tAtt->mapID()))
		return -1;

	if (entity->skill->trackType() == SkillInfo::HORLINE) {
		map<struct Component *, int64_t>::iterator it = entity->hitEntities.find(component);
		if (it != entity->hitEntities.end()) {
			if (Time_ElapsedTime() - it->second < HORLINE_HIT_INTERVAL)
				return -1;
		}
	}
	else if (entity->skill->trackType() == SkillInfo::VERLINE) {
		if (entity->hitEntities.find(component) != entity->hitEntities.end())
			return -1;
	}
	else {
		assert(0);
	}

	if (Fight_CantBeAttacked(component->fight))
		return -1;
	if (Status_HasType(component->status, StatusInfo::FLYAWAY))
		return -1;

	/*
	   const MovementAtt *myAtt = Movement_Att(Fight_Component(entity->owner)->movement);

	   static Vector3f dir;
	   dir.set_x(tAtt->position().x() - myAtt->position().x());
	   dir.set_z(tAtt->position().z() - myAtt->position().z());

	   float mul = dir.x() * entity->dir.x() + dir.z() * entity->dir.z();
	   if (mul < 0.0f)
	   return -1;
	   */

	if (entity->skill->targetType() & SkillInfo::ENEMY) {
		if (Fight_IsEnemy(entity->owner, component->fight)) {
			// God can do anything...
			/*
			   if (Status_HasType(component->status, StatusInfo::GOD)) {
			   entity->hitNum++;
			   return 0;
			   }
			   */

			entity->hitEntities[component] = Time_ElapsedTime();
			int count = 0;
			for (int i = 0; i < entity->skill->dStatusID_size(); i++) {
				if (entity->skill->dStatusID(i) == -1)
					break;

				const StatusInfo *statusInfo = StatusInfoManager_StatusInfo(entity->skill->dStatusID(i));
				int cur = StatusEntity_Create(Fight_Component(entity->owner)->status, component->status, &entity->position, StatusInfo::DEFENDER, statusInfo, entity->skill, statuses, size, count, resistControl);
				if (cur <= 0)
					continue;

				count += cur - count;
			}
			if (count <= 0)
				return -1;

			Fight_AddEnemy(component->fight, entity->owner);
			Fight_AddEnemy(entity->owner, component->fight);

			if (component->npc != NULL) {
				AI_BeAttacked(component->ai, entity->owner);
			}

			entity->hitNum++;
			return count;
		}
	}

	if (entity->skill->targetType() & SkillInfo::FRIEND) {
		if (!Fight_IsEnemy(entity->owner, component->fight) && component->fight != entity->owner) {
			entity->hitEntities[component] = Time_ElapsedTime();
			const StatusInfo * statusInfo = StatusInfoManager_StatusInfo(entity->skill->fStatusID());
			int count = StatusEntity_Create(Fight_Component(entity->owner)->status, component->status, &entity->position, StatusInfo::DEFENDER, statusInfo, entity->skill, statuses, size, 0, resistControl);
			if (count <= 0)
				return 0;

			entity->hitNum++;
			return count;
		}
	}

	if (entity->skill->targetType() & SkillInfo::SELF) {
		if (component->fight == entity->owner) {
			entity->hitEntities[component] = Time_ElapsedTime();
			const StatusInfo * statusInfo = StatusInfoManager_StatusInfo(entity->skill->aStatusID());
			int count = StatusEntity_Create(Fight_Component(entity->owner)->status, component->status, &entity->position, StatusInfo::ATTACKER, statusInfo, entity->skill, statuses, size, 0, resistControl);
			if (count <= 0)
				return 0;

			entity->hitNum++;
			return count;
		}
	}

	return -1;
}

static int32_t FinalSkillArea(struct SkillEntity *entity) {
	return entity->skill->radius() + Fight_SkillAreaDelta(entity->owner, entity->skill->id());
}

static struct PlayerEntity * WorldBossTypePlayer(struct SkillEntity *entity) {
	int map = entity->mapID;
	if (map == MapPool_WorldBoss()
		|| map == MapPool_FactionWar()
		|| MapPool_FactionOfBoss(map) != NULL) {
		struct Component *owner = Fight_Component(entity->owner);
		if (owner != NULL) {
			if (owner->player != NULL) {
				return owner->player;
			} else if (owner->npc != NULL) {
				struct Component *master = NPCEntity_Master(owner->npc);
				if (master != NULL)
					return master->player;
			}
		}
	}
	return NULL;
}

static bool ComputeHorlineHit(struct SkillEntity *entity) {
	static Vector2i coord;
	MapInfoManager_LogicCoordByPosition(&entity->position, &coord);

	if (entity->skill->areaType() == SkillInfo::ONE || entity->skill->areaType() == SkillInfo::SQUARE) {
		struct Movement *movements[CONFIG_FIXEDARRAY];
		int total = MapPool_SearchSquareArea(&coord, FinalSkillArea(entity), entity->mapID, WorldBossTypePlayer(entity), movements, CONFIG_FIXEDARRAY, CONFIG_FIXEDARRAY);
		struct Component *components[CONFIG_FIXEDARRAY];
		int j = 0;
		struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
		int all = 0;
		bool resistControl = false;
		for (int i = 0; i < total; i++) {
			struct Component *component = Movement_Component(movements[i]);
			int res = DoHit(component, entity, statuses + all, CONFIG_FIXEDARRAY - all, &resistControl);
			if (res >= 0) {
				components[j++] = component;
				all += res;
				if (entity->skill->areaType() == SkillInfo::ONE)
					break;
			}
		}
		if (j > 0) {
			SendHit(components, j, entity, statuses, all, resistControl);
			if (!entity->skill->canPenetrate()) {
				SkillEntity_Destroy(entity);
				return true;
			}
		}
	} else {
		assert(0);
	}

	return false;
}

static bool ComputeVerlineHit(struct SkillEntity *entity) {
	static Vector2i coord;
	MapInfoManager_LogicCoordByPosition(&entity->position, &coord);

	if (entity->tFight != NULL) {
		if (entity->skill->areaType() == SkillInfo::ONE) {
			if (!Fight_IsValid(entity->tFight) || !Movement_SameScene(Fight_Component(entity->owner)->movement, Fight_Component(entity->tFight)->movement)) {
				SkillEntity_Destroy(entity);
				return true;
			}
			struct Component *component = Fight_Component(entity->tFight);
			// if (IsNear(&entity->position, tPos, 1)) {
			struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
			bool resistControl = false;
			int res = DoHit(component, entity, statuses, CONFIG_FIXEDARRAY, &resistControl);
			if (res >= 0)
				SendHit(&component, 1, entity, statuses, res, resistControl);
			// }
		}
		else if (entity->skill->areaType() == SkillInfo::SQUARE) {
			static Vector2i coord;
			MapInfoManager_LogicCoordByPosition(&entity->position, &coord);

			struct Movement *movements[CONFIG_FIXEDARRAY];
			int total = MapPool_SearchSquareArea(&coord, FinalSkillArea(entity), entity->mapID, WorldBossTypePlayer(entity), movements, CONFIG_FIXEDARRAY, CONFIG_FIXEDARRAY);

			struct Component *components[CONFIG_FIXEDARRAY];
			int j = 0;
			struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
			int all = 0;
			bool resistControl = false;
			for (int i = 0; i < total; i++) {
				struct Component *component = Movement_Component(movements[i]);
				bool resistControl = false;
				int res = DoHit(component, entity, statuses + all, CONFIG_FIXEDARRAY - all, &resistControl);
				if (res >= 0) {
					components[j++] = component;
					all += res;
				}
			}
			if (j > 0)
				SendHit(components, j, entity, statuses, all, resistControl);
		}
		else {
			assert(0);
		}
	}
	else {
		static Vector2i coord;
		MapInfoManager_LogicCoordByPosition(&entity->position, &coord);

		struct Movement *movements[CONFIG_FIXEDARRAY];
		if (entity->skill->areaType() == SkillInfo::ONE) {
			int count = MapPool_Entities(entity->mapID, &coord, WorldBossTypePlayer(entity), movements, CONFIG_FIXEDARRAY);
			if (count < 0)
				DEBUG_LOGERROR("Failed to get all entities");
			for (int i = 0; i < count; i++) {
				struct Component *component = Movement_Component(movements[i]);
				struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
				bool resistControl = false;
				int res = DoHit(component, entity, statuses, CONFIG_FIXEDARRAY, &resistControl);
				if (res >= 0) {
					SendHit(&component, 1, entity, statuses, res, resistControl);
					break;
				}
			}
		}
		else if (entity->skill->areaType() == SkillInfo::SQUARE) {
			int total = MapPool_SearchSquareArea(&coord, FinalSkillArea(entity), entity->mapID, WorldBossTypePlayer(entity), movements, CONFIG_FIXEDARRAY, CONFIG_FIXEDARRAY);
			struct Component *components[CONFIG_FIXEDARRAY];
			int j = 0;
			struct StatusEntity *statuses[CONFIG_FIXEDARRAY];
			int all = 0;
			bool resistControl = false;
			for (int i = 0; i < total; i++) {
				struct Component *component = Movement_Component(movements[i]);
				int res = DoHit(component, entity, statuses + all, CONFIG_FIXEDARRAY - all, &resistControl);
				if (res >= 0) {
					components[j++] = component;
					all += res;
				}
			}
			if (j > 0)
				SendHit(components, j, entity, statuses, all, resistControl);
		}
		else {
			assert(0);
		}
	}

	return false;
}

static void AfterBegin(struct SkillEntity *entity) {
	if (entity->skill->skillType() == SkillInfo::STATUS) {
		if (entity->skill->trackType() == SkillInfo::HORLINE) {
		}
		else if (entity->skill->trackType() == SkillInfo::VERLINE) {
			switch(entity->skill->launchType()) {
				case SkillInfo::BLINK:
					{
						if (entity->skill->unitCount() == entity->count)
							Movement_SetPosition(Fight_Component(entity->owner)->movement, &entity->tPos);
					}
					break;
				default:
					break;
			}
		}
	}

	if (!Fight_IsValid(entity->owner))
		return;
	struct Component *component = Fight_Component(entity->owner);
	const Vector3f *myPos = &Movement_Att(component->movement)->position();
	Movement_SetOriginalOffsetPos(component->movement, myPos);
	if (entity->skill->aOffsetType() == SkillInfo::FRONT) {
		const Vector3f *endPos = &entity->tPos;
		if (entity->tFight != NULL) {
			if (!Fight_IsValid(entity->tFight))
				return;
			endPos = &Movement_Att(Fight_Component(entity->tFight)->movement)->position();
		}

		static Vector3f dir;
		dir.set_x(endPos->x() - myPos->x());
		dir.set_y(0);
		dir.set_z(endPos->z() - myPos->z());

		Vector3f_Normalize(&dir);
		float dist = entity->skill->offsetDist();
		dir.set_x(dir.x() * dist);
		dir.set_z(dir.z() * dist);

		static Vector3f dest;
		dest.set_x(myPos->x() + dir.x());
		dest.set_z(myPos->z() + dir.z());
		Movement_SetPosition(component->movement, &dest);
	}
}

static void MoveStatusSkill(struct SkillEntity *entity) {
	if (entity->skill->trackType() == SkillInfo::HORLINE) {
		int64_t delta = Time_ElapsedTime() - entity->start - entity->delay;
		if ((int32_t)delta >= entity->moveTime) {
			SkillEntity_Destroy(entity);
			return;
		}

		if (entity->tFight != NULL) {
			// TODO
			assert(0);
			if (!Fight_IsValid(entity->tFight)) {
				SkillEntity_Destroy(entity);
				return;
			}
		}
		else {
			static Vector2i coord;
			MapInfoManager_LogicCoordByPosition(&entity->position, &coord);

			if (entity->prevCoord.x() != coord.x() || entity->prevCoord.y() != coord.y()) {
				if (ComputeHorlineHit(entity))
					return;
				entity->prevCoord = coord;
			}

			DoMove(entity);
			/*
			   if (!CheckEdge(entity)) {
			   SkillEntity_Destroy(entity);
			   return;
			   }
			   */
		}
	}
	else if (entity->skill->trackType() == SkillInfo::VERLINE) {
		int64_t delta = Time_ElapsedTime() - entity->start - entity->delay;
		/*
		   if (delta >= (int64_t)entity->skill->moveActionTime()) {
		   SkillEntity_Destroy(entity);
		   return;
		   }
		   */

		if (entity->count <= 0) {
			SkillEntity_Destroy(entity);
			return;
		}

		if (entity->skill->launchType() == SkillInfo::SEPARATION) {
			if ((int32_t)delta < entity->moveTime / 2 + entity->moveTime * (entity->skill->unitCount() - entity->count))
				return;
		}
		else {
			if ((int32_t)delta < entity->moveTime * (entity->skill->unitCount() - entity->count + 1)) {
				if (entity->skill->unitCount() != entity->count || entity->skill->speed() != 1)
					return;
			}
		}

		switch(entity->skill->launchType()) {
			case SkillInfo::ROTATION:
				entity->position = Movement_Att(Fight_Component(entity->owner)->movement)->position();
				break;
			case SkillInfo::BACKSTAB:
			case SkillInfo::LEAP:
				{
					if (entity->tFight != NULL) {
						if (Fight_IsValid(entity->tFight))
							Movement_SetPosition(Fight_Component(entity->owner)->movement, &Movement_Att(Fight_Component(entity->tFight)->movement)->position());
					}
					else {
						Movement_SetPosition(Fight_Component(entity->owner)->movement, &entity->tPos);
					}
				}
				break;
			default:
				break;
		}

		if (ComputeVerlineHit(entity))
			return;
		entity->hitEntities.clear();

		if (entity->skill->launchType() == SkillInfo::BLINK) {
			Vector3f origin = entity->position;
			entity->position = entity->tPos;

			if (ComputeVerlineHit(entity))
				return;
			entity->hitEntities.clear();
			entity->position = origin;
		}

		// entity->count--;
		if (--entity->count <= 0) {
			SkillEntity_Destroy(entity);
			return;
		}
	}
}

static inline bool IsDelayOver(struct SkillEntity *entity) {
	return (int32_t)(Time_ElapsedTime() - entity->start) >= entity->delay;
}

void SkillEntity_Update(struct SkillEntity *entity) {
	assert(IsValid(entity));

	if (!Fight_IsValid(entity->owner)) {
		SkillEntity_Destroy(entity);
		return;
	}

	if (entity->status == SkillInfo::BEGIN) {
		if (!IsDelayOver(entity))
			return;

		entity->status = SkillInfo::MOVE;
		AfterBegin(entity);
	}

	if (entity->status == SkillInfo::MOVE) {
		if (entity->skill->skillType() == SkillInfo::STATUS) {
			MoveStatusSkill(entity);
		}
		else {
			assert(0);
		}
	}

	if (entity->status == SkillInfo::IDLE) {
		if (Time_ElapsedTime() - entity->beginIdle >= (int64_t)entity->skill->hitTime())
			DestroyReally(entity);
	}
}
