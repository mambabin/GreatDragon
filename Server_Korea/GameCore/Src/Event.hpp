#ifndef _EVENT_HPP_
#define _EVENT_HPP_

#include <sys/types.h>
#include <vector>
#include <string>
#include "PlayerEntity.hpp"
#include "DCProto.pb.h"
#include "NetProto.pb.h"
using namespace std;

struct stNotice {
	int id;
	int startTime;
	int endTime;
	int hz;
	string content;
	string GM;
	bool flag;
	int recentTime;
};

struct stForbid {
	int id;
	int64_t roleID;
	string name;
	int level;
	int profession;
	int startTime;
	int endTime;
	string GM;
	bool flag;
};

bool Event_NoticeOperator(NetProto_GMNoticeMgr* info, int32_t id);
bool Event_AddGMInfo(stForbid& st, bool flag = false);
bool Event_DelForbid(int id);
bool Event_DelForbid(int roleID, bool flag);
int Event_DailyMissionAward();

int Event_Goods(int32_t *goods, size_t size);

void Event_Notice(void *arg);

int Event_CreateGMID();

bool Event_GMRequest(NetProto_GMRequest& gmRequest);
bool Event_IsNoTalking(int64_t roleID);
bool Event_IsFreeze(int64_t roleID);

void Event_ResetDoubleRecharge(struct PlayerEntity *entity);
void Event_RecieveRoses(struct PlayerEntity *entity);
void Event_RealGod(struct PlayerEntity *entity, int event);

int Event_ExtraWorldBoss(time_t cur, int begin[]);
void Event_CheckFactionWarWinnerDesignation(struct PlayerEntity *entity, bool notice = true);
bool Event_AwardFromSky(struct PlayerEntity *entity);
void Event_AwardFromSky_Cost(struct Item *item, int64_t delta);

//void Event_NewYearGift(struct Item *item, int64_t delta);
void Event_AwardFromRecharge(struct Item *item, int64_t delta);
void Event_AwardFromSpend(struct Item *item, int64_t delta);

void Event_AwardStrongWeapon(struct PlayerEntity* entity);
void Event_AwardConsume(struct Item* item, int delta);

bool Event_ForbidMessage(int group, int unit);

int Event_AddGMData(DCProto_LoadAllDataFromGMDataTable * info, int op);

bool Event_IsGM(NetProto_GMLogin * info);
int Event_GMRegister(NetProto_GMRegister * info, const char * str);
int Event_GMShutDownMessage(NetProto_GMShutDownMessage * info, const char * str);
int Event_GMOpenMessage(NetProto_GMOpenMessage * info, const char * str);
int Event_GMModifyVIP(NetProto_GMModifyVIP * info, const char * str);
int Event_GMAddExchange(NetProto_GMAddExchange * info, const char * str);
bool Event_GMPermission(const char * str);
int Event_GMQueryFaction(NetProto_GMQueryFaction * info, const char * str);
int Event_GMChangeFactionMem(NetProto_GMChangeFactionMem * info, const char * str);
int Event_GMAddRekooRole(NetProto_GMAddRekooRole * info, const char * str);

int Event_DayEnterRoom(Component *component, int id);

#endif
