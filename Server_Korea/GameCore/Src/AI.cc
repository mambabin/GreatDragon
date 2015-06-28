#include "AI.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "AIInfo.hpp"
#include "IDGenerator.hpp"
#include "Movement.hpp"
#include "MovementInfo.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "MapInfo.pb.h"
#include "PlayerEntity.hpp"
#include "NPCEntity.hpp"
#include "NetProto.pb.h"
#include "GCAgent.hpp"
#include "SkillInfoManager.hpp"
#include "Math.hpp"
#include "MathUtil.hpp"
#include "Time.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cmath>

using namespace std;

struct AI{
	int32_t id;
	AIAtt *att;
	struct Component *component;
	int64_t move;
	int64_t search;
	struct Component *enemy;
	int64_t born;
	bool cantBeAttacked;
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct AI *> pool;
}package;


void AI_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, Config_IDInterval());
	package.pool.clear();
}

struct AI * AI_Create(AIAtt *att, struct Component *component) {
	if (att == NULL || component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct AI *ai = SearchBlock(package.pool, id);
	ai->id = id;
	ai->att = att;
	ai->component = component;
	ai->born = ai->move = ai->search = Time_ElapsedTime();
	ai->enemy = NULL;
	ai->cantBeAttacked = false;

	return ai;
}

void AI_Prepare(struct AI *ai) {
	if (!AI_IsValid(ai))
		return;

	if (ai->att->status() == AIAtt::BORN) {
		Fight_SetCantBeAttacked(ai->component->fight, true);
	}
}

bool AI_IsValid(struct AI *ai) {
	return ai != NULL && IDGenerator_IsValid(package.idGenerator, ai->id);
}

void AI_Destroy(struct AI *ai) {
	if (!AI_IsValid(ai))
		return;

	IDGenerator_Release(package.idGenerator, ai->id);
}

const AIAtt * AI_Att(struct AI *ai) {
	if (!AI_IsValid(ai))
		return NULL;

	return ai->att;
}

static inline void RandomDest(const Vector2i *center, int radius, Vector2i *dest) {
	int x = Time_Random(0, radius * 2) - radius;
	int y = Time_Random(0, radius * 2) - radius;
	dest->set_x(center->x() + x);
	dest->set_y(center->y() + y);
}

static bool Move(struct AI *ai, const Vector2i *dest_, bool useAStar) {
	const MovementAtt *movementAtt = Movement_Att(ai->component->movement);
	static Vector2i dest;
	if (dest_ != NULL)
		dest = *dest_;
	else
		RandomDest(&ai->att->birthCoord(), ai->att->moveRadius(), &dest);

	if (Movement_MoveStraightly(ai->component->movement, &dest) == -1) {
		if (useAStar) {
			if (Movement_MoveByAStar(ai->component->movement, &dest) == -1)
				return false;
		}
		else {
			return false;
		}
	}

	static NetProto_Move move;
	move.Clear();
	move.set_type(NetProto_Move::NPC);
	move.set_id(NPCEntity_ID(ai->component->npc));
	movementAtt->position().ToPB(move.mutable_start());
	Vector3f end;
	MapInfoManager_PositionByLogicCoord(&dest, &end);
	end.ToPB(move.mutable_end());
	GCAgent_SendProtoToAllClientsInSameScene(movementAtt->mapID(), NULL, &move);

	return true;
}

static void DoMove(struct AI *ai) {
	if (ai->att->moveType() == AIAtt::FREE) {
		if (ai->att->moveRadius() <= 0)
			return;

		const MovementAtt *movementAtt = Movement_Att(ai->component->movement);
		if (movementAtt->status() != MovementAtt::IDLE)
			return;

		if (Time_ElapsedTime() - ai->move < ai->att->moveInterval())
			return;
		ai->move = Time_ElapsedTime();

		Move(ai, NULL, ai->att->aiType() == AIAtt::FOLLOW);
	} else if (ai->att->moveType() == AIAtt::PATH) {
		const MovementAtt *movementAtt = Movement_Att(ai->component->movement);
		if (movementAtt->status() != MovementAtt::IDLE)
			return;

		if (Time_ElapsedTime() - ai->move < ai->att->moveInterval())
			return;
		ai->move = Time_ElapsedTime();

		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(ai->component->roleAtt->movementAtt().mapID()));
		if (mapInfo == NULL)
			return;
		const MapUnit *mapUnit = MapInfoManager_MapUnit(mapInfo);
		Movement_MoveByPathNode(ai->component->movement, mapUnit->targetNode());
	}
}

static void Reset(struct AI *ai, bool cantBeAttacked) {
	if (ai->att->moveType() == AIAtt::PATH)
		ai->att->set_status(AIAtt::IDLE);
	else if (ai->att->moveType() != AIAtt::DONTMOVE)
		ai->att->set_status(AIAtt::RESET);
	else
		ai->att->set_status(AIAtt::IDLE);
	ai->enemy = NULL;
	/*
	if (cantBeAttacked && ai->att->moveRadius() > 0) {
		Fight_SetCantBeAttacked(ai->component->fight, cantBeAttacked);
		ai->cantBeAttacked = cantBeAttacked;
	}
	*/
	Fight_Cancel(ai->component->fight);
	// DEBUG_LOG("miss enemy");
}

static void DoAttack(struct AI *ai) {
	if (Fight_HasSkillLaunchType(ai->component->fight, SkillInfo::ROTATION))
		return;

	if (!Movement_SameScene(ai->component->movement, ai->enemy->movement)) {
		Reset(ai, false);
		return;
	}

	if (Fight_CantBeAttacked(ai->enemy->fight)) {
		Reset(ai, false);
		return;
	}

	/*
	const Vector2i *myCoord = &Movement_Att(ai->component->movement)->logicCoord();
	if (abs(myCoord->x() - ai->att->birthCoord().x()) > ai->att->followRadius()
			|| abs(myCoord->y() - ai->att->birthCoord().y()) > ai->att->followRadius()) {
		Reset(ai, true);
		return;
	}
	*/

	if (Fight_Att(ai->component->fight)->status() != FightAtt::IDLE)
		return;

	if (Fight_CantAttack(ai->component->fight))
		return;

	int64_t cur = Time_ElapsedTime();

	const FightAtt *att = &NPCEntity_Att(ai->component->npc)->att().fightAtt();
	for (int i = 0; i < att->skills_size(); i++) {
		if (att->skills(i).id() == -1)
			continue;
		if (att->skills(i).level() <= 0)
			continue;

		const SkillInfo *skill = SkillInfoManager_SkillInfo(att->skills(i).id(), att->skills(i).level());
		skill = Fight_NextSkill(ai->component->fight, skill);
		if (skill == NULL)
			continue;

		if (!Fight_IsCDOver(ai->component->fight, skill, cur))
			continue;
		// if (skill->mp() > 0 && att->mana() < skill->mp())
		// 	continue;

		// skill = Fight_NextSkill(ai->component->fight, skill);
		// assert(skill != NULL);

		// int res = Fight_Attack(ai->component->fight, skill, NULL, &Movement_Att(ai->enemy->movement)->position());
		int res = Fight_Attack(ai->component->fight, skill, ai->enemy->fight, NULL, cur);
		if (res < 0) {
			// DEBUG_LOGERROR("Failed to attack by npc, errno: %d", res);
			Reset(ai, false);
			return;
		}
		else if (res == 1 || res == 0) {
			/*
			   static NetProto_Attack attack;
			   attack.Clear();
			   attack.set_aType(NetProto_Attack::NPC);
			   attack.set_aID(NPCEntity_ID(ai->component->npc));
			 *attack.mutable_aSkill() = att->skills(0);
			// attack.set_dID(-1);
			// *attack.mutable_tPos() = Movement_Att(ai->enemy->movement)->position();
			if (ai->enemy->player != NULL) {
			attack.set_dType(NetProto_Attack::PLAYER);
			attack.set_dID(PlayerEntity_ID(ai->enemy->player));
			}
			else if (ai->enemy->npc != NULL) {
			attack.set_dType(NetProto_Attack::NPC);
			attack.set_dID(NPCEntity_ID(ai->enemy->npc));
			}
			GCAgent_SendProtoToAllClientsInSameScene(Movement_Att(ai->enemy->movement)->mapID(), &attack);
			*/

			if (res == 1) {
				Fight_DoAttack(ai->component->fight, NULL, 0, cur);
			}

			ai->att->set_status(AIAtt::BUSY);
			return;
		}
	}
}

static bool DoSearch(struct AI *ai) {
	if (ai->att->searchType() == AIAtt::MINDIST) {
		if (ai->att->searchRadius() <= 0)
			return false;

		if (Time_ElapsedTime() - ai->search < ai->att->searchInterval())
			return false;
		ai->search = Time_ElapsedTime();

		const MovementAtt *att = Movement_Att(ai->component->movement);
		struct Movement *entities[CONFIG_FIXEDARRAY];
		int count = MapPool_SearchWithMinDist(&att->logicCoord(), ai->att->searchRadius(), att->mapID(), NULL, entities, CONFIG_FIXEDARRAY, CONFIG_FIXEDARRAY);
		for (int i = 0; i < count; i++) {
			struct Component *component = Movement_Component(entities[i]);
			if (Fight_CantBeAttacked(component->fight))
				continue;
			if (!Fight_IsEnemy(ai->component->fight, component->fight))
				continue;

			ai->enemy = component;
			DoAttack(ai);
			// DEBUG_LOG("find enemy");
			return true;
		}
	}

	return false;
}

void AI_Idle(struct AI *ai) {
	if (!AI_IsValid(ai))
		return;

	ai->enemy = NULL;
	ai->att->set_status(AIAtt::IDLE);
}

static void UpdateFlee(struct AI *ai) {
	const MovementAtt *movementAtt = Movement_Att(ai->component->movement);
	if (movementAtt->status() != MovementAtt::IDLE)
		return;

	if (!Movement_SameScene(ai->component->movement, ai->enemy->movement)) {
		AI_Idle(ai);
		return;
	}

	const Vector2i *myCoord = &movementAtt->logicCoord();
	const Vector2i *enemyCoord = &Movement_Att(ai->enemy->movement)->logicCoord();
	if (abs(myCoord->x() - enemyCoord->x()) > ai->att->searchRadius()
			|| abs(myCoord->y() - enemyCoord->y()) > ai->att->searchRadius()) {
		AI_Idle(ai);
		return;
	}

	Move(ai, NULL, false);
}

static bool DoFlee(struct AI *ai) {
	if (ai->att->fleeType() == AIAtt::SEARCH) {
		if (ai->att->status() == AIAtt::IDLE) {
			if (Time_ElapsedTime() - ai->search < ai->att->searchInterval())
				return false;
			ai->search = Time_ElapsedTime();

			const MovementAtt *att = Movement_Att(ai->component->movement);
			struct Movement *entities[CONFIG_FIXEDARRAY];
			int count = MapPool_SearchWithMinDist(&att->logicCoord(), ai->att->searchRadius(), att->mapID(), NULL, entities, CONFIG_FIXEDARRAY, CONFIG_FIXEDARRAY);
			for (int i = 0; i < count; i++) {
				struct Component *component = Movement_Component(entities[i]);
				if (Fight_IsEnemy(ai->component->fight, component->fight)) {
					Movement_Stop(ai->component->movement);
					ai->enemy = component;
					ai->att->set_status(AIAtt::FLEE);
					UpdateFlee(ai);
					return true;
				}
			}
		}
	}

	return false;
}

static void UpdateReset(struct AI *ai) {
	if (ai->cantBeAttacked)
		Fight_SetCantBeAttacked(ai->component->fight, true);
	if (Movement_Att(ai->component->movement)->status() == MovementAtt::IDLE) {
		const Vector2i *myCoord = &Movement_Att(ai->component->movement)->logicCoord();
		if (abs(myCoord->x() - ai->att->birthCoord().x()) > ai->att->moveRadius()
				|| abs(myCoord->y() - ai->att->birthCoord().y()) > ai->att->moveRadius()) {
			if (!Move(ai, NULL, true))
				return;
		}
		else {
			if (!Move(ai, NULL, true))
				return;
			ai->att->set_status(AIAtt::IDLE);
			if (Fight_CantBeAttacked(ai->component->fight))
				Fight_SetCantBeAttacked(ai->component->fight, false);
			// DEBUG_LOG("idle");
		}
	}
}

void AI_BeAttacked(struct AI *ai, struct Fight *attacker) {
	if (!AI_IsValid(ai) || attacker == NULL)
		return;

	if (Fight_CantBeAttacked(ai->component->fight))
		return;

	if (ai->att->status() != AIAtt::IDLE)
		return;

	if (!ai->att->canAttackBack())
		return;

	ai->enemy = Fight_Component(attacker);
	ai->att->set_status(AIAtt::BUSY);
	DoAttack(ai);
}

void AI_Update(struct AI *ai) {
	assert(AI_IsValid(ai));

	if (Fight_CantAttack(ai->component->fight))
		return;

	if (ai->att->status() == AIAtt::BORN) {
		if (ai->component->npc != NULL) {
			const NPCAtt *npcAtt = NPCEntity_Att(ai->component->npc);
			int64_t cur = Time_ElapsedTime();
			if (cur - ai->born >= (int64_t)npcAtt->bornTime()) {
				ai->att->set_status(AIAtt::IDLE);
				Fight_SetCantBeAttacked(ai->component->fight, false);
			}
		} else {
			ai->att->set_status(AIAtt::IDLE);
			Fight_SetCantBeAttacked(ai->component->fight, false);
		}
	} else {
		if (ai->att->aiType() == AIAtt::NORMAL
				|| ai->att->aiType() == AIAtt::BOX
				|| ai->att->aiType() == AIAtt::FOLLOW
				|| (ai->att->aiType() == AIAtt::TIME && ai->att->arg(0) == 1)) {
			if (ai->att->aiType() == AIAtt::FOLLOW) {
				// Only for pet
				if (ai->component->npc != NULL) {
					struct Component *master = NPCEntity_Master(ai->component->npc);
					if (Component_IsValid(master))
						*ai->att->mutable_birthCoord() = master->roleAtt->movementAtt().logicCoord();
				}
			}
			if (ai->att->status() == AIAtt::IDLE) {
				if (ai->att->aiType() == AIAtt::FOLLOW) {
					// Only for pet
					if (ai->component->npc != NULL) {
						struct Component *master = NPCEntity_Master(ai->component->npc);
						if (Component_IsValid(master)) {
							const Vector2i *masterCoord = &master->roleAtt->movementAtt().logicCoord();
							const Vector2i *myCoord = &ai->component->roleAtt->movementAtt().logicCoord();
							if (fabs((myCoord->x() + myCoord->y()) - (masterCoord->x() + masterCoord->y())) > Config_MaxPetDistance()) {
								Vector3f pos;
								MapInfoManager_PositionByLogicCoord(masterCoord, &pos);
								Movement_SetPosition(ai->component->movement, &pos);
							}
						}
					}
				}
				if (DoFlee(ai))
					return;
				if (DoSearch(ai))
					return;
				DoMove(ai);
			} else if (ai->att->status() == AIAtt::BUSY) {
				DoAttack(ai);
			} else if (ai->att->status() == AIAtt::FLEE) {
				UpdateFlee(ai);
			} else if (ai->att->status() == AIAtt::RESET) {
				UpdateReset(ai);
			}
		} else if (ai->att->aiType() == AIAtt::TIME && ai->att->arg(0) == 0) {
		} else {
			assert(0);
		}
	}
}
