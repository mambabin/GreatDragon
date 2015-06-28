#ifndef _FACTION_POOL_HPP_
#define _FACTION_POOL_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include "PlayerAtt.hpp"
#include "DCProto.pb.h"
#include "NetProto.pb.h"
#include "GCAgent.hpp"


struct FactionMem {
	FactionMem():roleID(-1),name(""),level(0),contribute(0),lastLoginTime(0),type(0){}
	int64_t roleID;
	std::string name;
	int level;
	int contribute;
	int lastLoginTime;
	int type;
};

struct FactionMemComp {
	bool operator()(const FactionMem& value1, const FactionMem& value2) {
		return value1.roleID < value2.roleID;
	}	
};

struct Applicant {
	Applicant():roleID(-1),level(0),power(0),vip(0),name("") {factionName.clear();}
	int64_t roleID;
	int level;
	int power;
	int vip;
	std::string name;
	std::set<std::string> factionName;	
};

struct FactionCache {
	FactionCache():bits(0) {changePlayer.clear();}
	int bits;
	std::set<FactionMem, FactionMemComp> changePlayer;
	//set<Applicant, FactionMemComp> changeApplicant; 
};

struct Faction {
	Faction():name(""),factionName(""),num(0),exp(0),lastLevelUpTime(0),item(0),notice(""),guardian(""){factionMem.clear();}
	std::string name;
	std::string factionName;
	int num;
	int64_t exp;
	int lastLevelUpTime;
	int item;
	std::string notice;
	std::string guardian;
	std::set<FactionMem, FactionMemComp> factionMem;
};

int FactionPool_CreateFaction(const PlayerAtt* att, const char* str);

int FactionPool_DelFaction(const PlayerAtt* att);

int FactionPool_Donate(const PlayerAtt* att, int type, int contribute = -1);

int FactionPool_Notice(const PlayerAtt* att, const char* str);

int FactionPool_Designate(const PlayerAtt* att, int64_t roleID, int type);

int FactionPool_AddMem(const PlayerAtt* att, int64_t roleID, bool flag);

int FactionPool_DelMem(const PlayerAtt* att, int64_t roleID);

int FactionPool_Applicant(const PlayerAtt* att, const char* str, int power, int vip);

int FactionPool_Guardian(const PlayerAtt* att, int type);

int FactionPool_FactionGuardian(const PlayerAtt* att, NetProto_FactionGuardian* proto);

void FactionPool_Init(const DCProto_FactionLoadData* data);

void FactionPool_Signin(int64_t roleID, bool flag);

void FactionPool_SetBits(int64_t roleID, int index);

int FactionPool_FactionInfo(const PlayerAtt* att, NetProto_FactionInfo* info);

int FactionPool_ChangeMem(const PlayerAtt* att, NetProto_FactionChangeMem* mem);

int FactionPool_ApplicantListRequest(const PlayerAtt* att, NetProto_FactionAllApplicant* proto);

int FactionPool_FactionListRequest(const PlayerAtt* att, NetProto_FactionList* proto);

void FactionPool_AddRecord(DCProto_FactionAddRecord* data, Applicant* applicant, Faction* faction);

void FactionPool_DelRecord(DCProto_FactionDelRecord* data, Applicant* applicant, Faction* faction);

void FactionPool_UpdateRecord(DCProto_FactionUpdateRecord* data, Applicant* applicant, Faction* faction);

int FactionPool_Check(int64_t roleID, const char* str);

void FactionPool_InitSociaty();

void FactionPool_GetFactionGurdianData(std::string str, int* vData1, double* vData2, bool flag);

void FactionPool_SynFactionLevel(const PlayerAtt* att);

int FactionPool_GetFactionMem(const PlayerAtt* att, int* id);

int FactionPool_GetRoleContribute(const PlayerAtt* att);

void FactionPool_UpateRoleContribute(const PlayerAtt* att, int detail);

void FactionPool_AddExpAndItem(const char* str, int exp, int item);

void FactionPool_AddMemContribute(const char* str, int num, int64_t* roleIDs, int count);

// bit
// 1: president
// 2: group leader
// 4: teacher
// 8: student
void FactionPool_GetRolesFromFaction(const char* str, int bit, std::vector<int64_t>* vec);

void FactionPool_GMQueryFaction(NetProto_GMQueryFaction * info);
void FactionPool_GMChangeFactionMem(NetProto_GMChangeFactionMem * info);
int FactionPool_GetMemOffic(const char* str, int64_t roleID);
#endif
