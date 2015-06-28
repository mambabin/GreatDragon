#include "GlobalMissionManager.hpp"
#include "Movement.hpp"
#include "Mission.hpp"
#include "MissionInfo.hpp"
#include "PlayerEntity.hpp"
#include "GCAgent.hpp"
#include "NetProto.pb.h"
#include <sys/types.h>
#include <vector>

using namespace std;

static struct{
	vector<vector<const MissionInfo *> > sceneMissions;
	vector<const MissionInfo *> loginMissions;
	vector<const MissionInfo *> dailyMissions;
}package;

void GlobalMissionManager_Init() {
}

void GlobalMissionManager_Add(const MissionInfo *missionInfo) {
	if (missionInfo == NULL || missionInfo->id() == -1)
		return;

	if (missionInfo->type() == MissionInfo::DAILY) {
		package.dailyMissions.push_back(missionInfo);
	} else {
		const MissionPort *in = &missionInfo->in();
		switch(in->type()) {
			case MissionPort::CHANGE_SCENE:
				{
					int32_t dest = in->arg(0);
					if (dest >= (int32_t)package.sceneMissions.size())
						package.sceneMissions.resize(dest + 1);

					package.sceneMissions[dest].push_back(missionInfo);
				}
				break;
			case MissionPort::LOGIN:
				{
					package.loginMissions.push_back(missionInfo);
				}
				break;

			default:
				break;
		}
	}
}

void GlobalMissionManager_ApplySceneMission(struct Movement *movement) {
	if (movement == NULL || !Movement_IsValid(movement))
		return;

	struct Component *component = Movement_Component(movement);
	if (component->mission == NULL || !Mission_IsValid(component->mission))
		return;

	int32_t cur = Movement_Att(movement)->mapID();
	if (cur >= (int32_t)package.sceneMissions.size())
		return;

	vector<const MissionInfo *> &sceneMissions = package.sceneMissions[cur];
	for (size_t i = 0; i < sceneMissions.size(); i++) {
		const MissionInfo *missionInfo = sceneMissions[i];
		if (Mission_CanApply(component->mission, missionInfo->id())) {
			Mission_Add(component->mission, missionInfo->id());
			if (component->player != NULL) {
				static NetProto_ApplyMission applyMission;
				applyMission.Clear();

				applyMission.set_id(missionInfo->id());

				int id = PlayerEntity_ID(component->player);
				GCAgent_SendProtoToClients(&id, 1, &applyMission);
			}
		}
	}
}

void GlobalMissionManager_ApplyLoginMission(struct Movement *movement) {
	if (movement == NULL || !Movement_IsValid(movement))
		return;

	struct Component *component = Movement_Component(movement);
	if (component->mission == NULL || !Mission_IsValid(component->mission))
		return;

	for (size_t i = 0; i < package.loginMissions.size(); i++) {
		const MissionInfo *missionInfo = package.loginMissions[i];
		if (Mission_CanApply(component->mission, missionInfo->id())) {
			Mission_Add(component->mission, missionInfo->id());
		}
	}
}

void GlobalMissionManager_ApplyDailyMission(struct Movement *movement) {
	if (movement == NULL || !Movement_IsValid(movement))
		return;

	struct Component *component = Movement_Component(movement);
	if (component->mission == NULL || !Mission_IsValid(component->mission))
		return;

	for (size_t i = 0; i < package.dailyMissions.size(); i++) {
		const MissionInfo *missionInfo = package.dailyMissions[i];
		Mission_GiveUp(component->mission, missionInfo->id());
		Mission_Add(component->mission, missionInfo->id());
	}
}
