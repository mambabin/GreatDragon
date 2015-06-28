#include "Status.hpp"
#include "Config.hpp"
#include "Component.hpp"
#include "StatusInfo.pb.h"
#include "IDGenerator.hpp"
#include "StatusEntity.hpp"
#include "NetProto.pb.h"
#include "PlayerEntity.hpp"
#include "Movement.hpp"
#include "Time.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <vector>

using namespace std;

#define STATUS_MAX 32

struct Status{
	int32_t id;
	struct Component *component;
	struct StatusEntity *statuses[STATUS_MAX];
	int cantMoveCount;
	int cantAttackCount;
	int cantBeAttackedCount;
	int64_t controlOverTime;
};

static struct{
	int max;
	struct IDGenerator *idGenerator;
	vector<struct Status *> pool;
}package;


void Status_Init() {
	package.max = Config_MaxComponents();
	package.idGenerator = IDGenerator_Create(package.max, 60 * 1000);
	package.pool.clear();
}

struct Status * Status_Create(struct Component *component) {
	if (component == NULL)
		return NULL;

	int32_t id = IDGenerator_Gen(package.idGenerator);
	if (id == -1)
		return NULL;

	struct Status *status = SearchBlock(package.pool, id);
	status->id = id;
	status->component = component;
	for (int i = 0; i < STATUS_MAX; i++)
		status->statuses[i] = NULL;
	status->cantMoveCount = 0;
	status->cantAttackCount = 0;
	status->cantBeAttackedCount = 0;
	status->controlOverTime = CONFIG_INIT_USE_TIME;

	return status;
}

bool Status_IsValid(struct Status *status) {
	return status != NULL && IDGenerator_IsValid(package.idGenerator, status->id);
}

void Status_Destroy(struct Status *status) {
	if (!Status_IsValid(status))
		return;
	IDGenerator_Release(package.idGenerator, status->id);
}

void Status_Finalize(struct Status *status) {
	if (!Status_IsValid(status))
		return;
	Status_Clear(status);
}

struct Component * Status_Component(struct Status *status) {
	if (!Status_IsValid(status))
		return NULL;

	return status->component;
}

int Status_GetType(struct Status *status, StatusInfo::StatusType type, struct StatusEntity **statuses, size_t size) {
	if (!Status_IsValid(status) || statuses == NULL)
		return -1;

	int count = 0;
	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL)
			continue;

		if (StatusEntity_Info(status->statuses[i])->statusType() == type) {
			if (count >= (int)size)
				return -1;
			statuses[count++] = status->statuses[i];
		}
	}

	return count;
}

int Status_Add(struct Status *status, struct StatusEntity *entity) {
	if (!Status_IsValid(status) || entity == NULL)
		return -1;

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL) {
			status->statuses[i] = entity;

			const StatusInfo *info = StatusEntity_Info(entity);
			switch(info->statusType()) {
				case StatusInfo::STATIC:
				case StatusInfo::SKY:
				case StatusInfo::FLYAWAY:
					Fight_Cancel(status->component->fight);
					status->cantMoveCount++;
					status->cantAttackCount++;
					break;
				case StatusInfo::GROUND:
				case StatusInfo::STANDUP:
					Fight_Cancel(status->component->fight);
					status->cantMoveCount++;
					status->cantAttackCount++;
					// status->cantBeAttackedCount++;
					break;

				case StatusInfo::SPEED:
					Movement_SetSpeedFactor(status->component->movement, Movement_SpeedFactor(status->component->movement) + (StatusEntity_Info(entity)->percent()));
					break;

				case StatusInfo::CANTMOVE:
					Fight_Cancel(status->component->fight);
					status->cantMoveCount++;
					break;

				case StatusInfo::GIDDY:
				case StatusInfo::FREEZE:
					Fight_Cancel(status->component->fight);
					status->cantMoveCount++;
					status->cantAttackCount++;
					break;

				case StatusInfo::FEAR:
					status->cantAttackCount++;
					break;

				case StatusInfo::SELF_ATT:
					Fight_ModifyProperty(status->component->fight, (FightAtt::PropertyType)info->value(), info->count());
					break;

				default:
					break;
			}

			if (status->cantMoveCount > 0)
				Movement_SetCantMove(status->component->movement, true);
			if (status->cantAttackCount > 0)
				Fight_SetCantAttack(status->component->fight, true);
			if (status->cantBeAttackedCount > 0)
				Fight_SetCantBeAttacked(status->component->fight, true);

			switch(info->statusType()) {
				case StatusInfo::CANTMOVE:
				case StatusInfo::GIDDY:
				case StatusInfo::FREEZE:
				case StatusInfo::FEAR:
					status->controlOverTime = Time_ElapsedTime() + info->interval() * 4;
					break;
				default:
					break;
			}

			return 0;
		}
	}

	return -1;
}

void Status_Del(struct Status *status, struct StatusEntity *entity) {
	if (!Status_IsValid(status) || entity == NULL)
		return;

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == entity) {
			status->statuses[i] = NULL;

			const StatusInfo *info = StatusEntity_Info(entity);
			switch(info->statusType()) {
				case StatusInfo::STATIC:
				case StatusInfo::SKY:
				case StatusInfo::FLYAWAY:
					status->cantMoveCount--;
					status->cantAttackCount--;
					break;
				case StatusInfo::GROUND:
				case StatusInfo::STANDUP:
					status->cantMoveCount--;
					status->cantAttackCount--;
					// status->cantBeAttackedCount--;
					break;

				case StatusInfo::SPEED:
					Movement_SetSpeedFactor(status->component->movement, Movement_SpeedFactor(status->component->movement) - StatusEntity_Info(entity)->percent());
					break;

				case StatusInfo::CANTMOVE:
					status->cantMoveCount--;
					break;

				case StatusInfo::GIDDY:
				case StatusInfo::FREEZE:
					status->cantMoveCount--;
					status->cantAttackCount--;
					break;

				case StatusInfo::FEAR:
					status->cantAttackCount--;
					break;

				case StatusInfo::SELF_ATT:
					Fight_ModifyProperty(status->component->fight, (FightAtt::PropertyType)info->value(), -info->count());
					break;

				default:
					break;
			}

			if (status->cantMoveCount <= 0) {
				Movement_SetCantMove(status->component->movement, false);
				if (status->cantMoveCount < 0)
					status->cantMoveCount = 0;
			}
			if (status->cantAttackCount <= 0) {
				Fight_SetCantAttack(status->component->fight, false);
				if (status->cantAttackCount < 0)
					status->cantAttackCount = 0;
			}
			if (status->cantBeAttackedCount <= 0) {
				Fight_SetCantBeAttacked(status->component->fight, false);
				if (status->cantBeAttackedCount < 0)
					status->cantBeAttackedCount = 0;
			}

			return;
		}
	}
}

void Status_DelType(struct Status *status, StatusInfo::StatusType type) {
	if (!Status_IsValid(status))
		return;

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL)
			continue;

		if (StatusEntity_Info(status->statuses[i])->statusType() == type) {
			StatusEntity_Destroy(status->statuses[i]);
		}
	}
}

void Status_DelTypes(struct Status *status, StatusInfo::StatusType types[], size_t size) {
	if (!Status_IsValid(status))
		return;

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL)
			continue;

		StatusInfo::StatusType type = StatusEntity_Info(status->statuses[i])->statusType();
		for (size_t j = 0; j < size; j++) {
			if (type == types[j])
				StatusEntity_Destroy(status->statuses[i]);
		}
	}
}

void Status_Clear(struct Status *status) {
	if (!Status_IsValid(status))
		return;

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL)
			continue;

		StatusEntity_Destroy(status->statuses[i]);
	}

	status->cantMoveCount = 0;
	status->cantAttackCount = 0;
	status->cantBeAttackedCount = 0;
	status->controlOverTime = CONFIG_INIT_USE_TIME;
}

bool Status_HasType(struct Status *status, StatusInfo::StatusType type) {
	if (!Status_IsValid(status))
		return false;

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL)
			continue;

		if (StatusEntity_Info(status->statuses[i])->statusType() == type)
			return true;
	}

	return false;
}

bool Status_HasTypes(struct Status *status, StatusInfo::StatusType types[], size_t size) {
	if (!Status_IsValid(status))
		return false;

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL)
			continue;

		StatusInfo::StatusType type = StatusEntity_Info(status->statuses[i])->statusType();
		for (size_t j = 0; j < size; j++) {
			if (type == types[j])
				return true;
		}
	}

	return false;
}

bool Status_IsControlOver(struct Status *status) {
	if (!Status_IsValid(status))
		return false;

	return true;
	// return Time_ElapsedTime() > status->controlOverTime;
}

void Status_Update(struct Status *status) {
	assert(Status_IsValid(status));

	for (int i = 0; i < STATUS_MAX; i++) {
		if (status->statuses[i] == NULL)
			continue;

		StatusEntity_Update(status->statuses[i]);
	}
}
