#include "FactionPool.hpp"
#include "FactionInfo.pb.h"
#include <fstream>
#include <algorithm>
#include "PlayerEntity.hpp"
#include "ScribeClient.hpp"
#include "AccountPool.hpp"
using namespace std;

static struct {
	map<string, Faction> factionPool;
	map<int64_t, Applicant> applicantPool;
	map<int64_t, FactionCache> cachePool;

	FactionBaseInfo factionInfo;
	SociatySkillBaseInfo sociatySkillInfo;
}pool;

void CreateFactionMem(const PlayerAtt* att, FactionMem& factionMem, int type) {
	factionMem.roleID = att->att().baseAtt().roleID();
	factionMem.name = att->att().baseAtt().name();
	factionMem.level = att->att().fightAtt().level();
	factionMem.contribute = 0;
	factionMem.lastLoginTime = Time_TimeStamp();
	factionMem.type = type;
}

void CreateFaction(const PlayerAtt* att, Faction& faction, const char* str) {
	faction.name = att->att().baseAtt().name();
	faction.factionName = str;
	faction.num = 1;
	faction.exp = 0;
	faction.lastLevelUpTime = Time_TimeStamp();
	faction.item = 0;
	faction.notice = "";
	faction.guardian = "";

	FactionMem factionMem;
	CreateFactionMem(att, factionMem, 1);
	faction.factionMem.insert(factionMem);
}

int FactionPool_CreateFaction(const PlayerAtt* att, const char* str) {
	map<string, Faction>::iterator itor = pool.factionPool.find(str);
	if (itor != pool.factionPool.end()) {
		DEBUG_LOG("faction name has already exit!");
		return -1;
	}

	itor = pool.factionPool.find(att->faction());
	if (itor != pool.factionPool.end()) {
		DEBUG_LOG("you have faction!");
		return -2;
	}
	
	if (strlen(str) < 2 || strlen(str) > PlayerAtt::FACTION_SIZE - 1) {
		return -4;
	}

	for (size_t i = 0; i < strlen(str); ++i) {
		if (str[i] == ',') {
			return -5;
		}
	}

	Faction faction;
	CreateFaction(att, faction, str);
	pool.factionPool[string(str)] = faction;
	
	pool.applicantPool.erase(att->att().baseAtt().roleID());

	DCProto_FactionDelRecord delData;
	Applicant delApplicant;
	delApplicant.roleID = att->att().baseAtt().roleID();
	FactionPool_DelRecord(&delData, &delApplicant, NULL);

	DCProto_FactionAddRecord data;
	FactionPool_AddRecord(&data, NULL, &faction);
	
	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)att->att().baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", att->att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"factionName\":\"%s", str);
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(att->att().baseAtt().roleID());
		if (entity != NULL) {
			int32_t id  = PlayerEntity_ID(entity);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "CreateFaction", buf));
		}
	}
	return 0;
}

int FactionPool_DelFaction(const PlayerAtt* att) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}
	
	if ((itor->second.name).compare(att->att().baseAtt().name()) != 0) {
		DEBUG_LOG("%s != %s", (itor->second.name).c_str(), att->att().baseAtt().name());
		return -2;
	}	

	DCProto_FactionDelRecord data;
	FactionPool_DelRecord(&data, NULL, &(itor->second));

	for (set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.begin(); it != itor->second.factionMem.end(); ++it) {
		PlayerEntity *target = PlayerEntity_PlayerByRoleID(it->roleID);
		if (target != NULL) {
			PlayerEntity_SysFaction(target, "");
			int32_t targetID = PlayerEntity_ID(target);
			static NetProto_AcceptToFaction accept;
			accept.set_factionName("");
			GCAgent_SendProtoToClients(&targetID, 1, &accept);
		}
	}

	pool.factionPool.erase(itor);
	FactionPool_Signin(att->att().baseAtt().roleID(), true);
	
	return 0;
}

int FactionPool_Notice(const PlayerAtt* att, const char* str) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}
	
	FactionMem factionMem;
	factionMem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
	if (it == itor->second.factionMem.end()) {
		return -2;
	}
	
	if (!(it->type == 1 || it->type == 2)) {
		DEBUG_LOG("type = %d", it->type);
		return -3;
	}

	itor->second.notice = str;
	
	DCProto_FactionUpdateRecord data;
	FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(att->faction())->second));
	
	return 0;
}

int FactionPool_Designate(const PlayerAtt* att, int64_t roleID, int type) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}
	
	FactionMem factionMem;
	factionMem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
	if (it == itor->second.factionMem.end()) {
		return -2;
	}

	int ownType = it->type;
	if (ownType == 1 && type == 1) {
		if (roleID == att->att().baseAtt().roleID()) {
			return -7;
		}
		factionMem.roleID = roleID;
		it = itor->second.factionMem.find(factionMem);
		if (it == itor->second.factionMem.end()) {
			return -7;
		}
		itor->second.name = it->name;
		
		factionMem = *it;
		factionMem.type = type;
		itor->second.factionMem.erase(it);
		itor->second.factionMem.insert(factionMem);
		{
			set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
			map<int64_t, FactionCache>::iterator it1;
			for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
				it1 = pool.cachePool.find(it2->roleID);
				if (it1 != pool.cachePool.end()) {
					it1->second.changePlayer.erase(factionMem);
					it1->second.changePlayer.insert(factionMem);
				}
			}
		}
	
		factionMem.roleID = att->att().baseAtt().roleID();
		it = itor->second.factionMem.find(factionMem);

		factionMem = *it;
		factionMem.type = 4;
		itor->second.factionMem.erase(it);
		itor->second.factionMem.insert(factionMem);
		{
			set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
			map<int64_t, FactionCache>::iterator it1;
			for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
				it1 = pool.cachePool.find(it2->roleID);
				if (it1 != pool.cachePool.end()) {
					it1->second.changePlayer.erase(factionMem);
					it1->second.changePlayer.insert(factionMem);
				}
			}
		}

		DCProto_FactionUpdateRecord data;
		FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(att->faction())->second));
		
		return 0;
	}


	if (ownType >= type) {
		return -3;
	}

	factionMem.roleID = roleID;
	it = itor->second.factionMem.find(factionMem);
	if (it == itor->second.factionMem.end()) {
		return -4;
	}
	
	if (ownType >= it->type) {
		return -5;
	}

	if (type == 2) {
		int num = 0;
		for (set<FactionMem, FactionMemComp>::iterator it1 = itor->second.factionMem.begin(); it1 != itor->second.factionMem.end(); ++it1) {
			if (it1->type == 2) {
				num++;
			}
			if (num >= 3) {
				return -6;
			}
		}
	}

	factionMem = *it;
	factionMem.type = type;
	itor->second.factionMem.erase(it);
	itor->second.factionMem.insert(factionMem);

	DCProto_FactionUpdateRecord data;
	FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(att->faction())->second));

	{
		set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
		map<int64_t, FactionCache>::iterator it1;
		for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
			it1 = pool.cachePool.find(it2->roleID);
			if (it1 != pool.cachePool.end()) {
				it1->second.changePlayer.erase(factionMem);
				it1->second.changePlayer.insert(factionMem);
			}
		}
	}

	return 0;
}

int FactionPool_AddMem(const PlayerAtt* att, int64_t roleID, bool flag) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}
	
	FactionMem factionMem;
	factionMem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
	if (it == itor->second.factionMem.end()) {
		return -2;
	}

	if (!(it->type == 1 || it->type == 2 || it->type == 3)) {
		DEBUG_LOG("insufficient permissions!");
		return -3;
	}

	map<int64_t, Applicant>::iterator itorApplicant = pool.applicantPool.find(roleID);
	if (itorApplicant == pool.applicantPool.end()) {
		return -4;
	}

	if (!flag) {
		set<string>::iterator itorName = itorApplicant->second.factionName.find(att->faction());
		if (itorName == itorApplicant->second.factionName.end()) {
			return -5;
		}
		if (itorApplicant->second.factionName.size() == 1) {
			itorApplicant->second.factionName.clear();
			pool.applicantPool.erase(itorApplicant);
			
			DCProto_FactionDelRecord delData;
			Applicant delApplicant;
			delApplicant.roleID = roleID;
			FactionPool_DelRecord(&delData, &delApplicant, NULL);
			return 1;
		}
		itorApplicant->second.factionName.erase(itorName);

		DCProto_FactionUpdateRecord upApplicantData;
		FactionPool_UpdateRecord(&upApplicantData, &(pool.applicantPool.find(roleID)->second), NULL);
		
		return 1;
	}

	int64_t exp = itor->second.exp;
	int maxNum = 0;
	for (int i = 0; i < (int)pool.factionInfo.faction_size(); ++i) {
		if (exp >= pool.factionInfo.faction(i).exp()) {
			maxNum = pool.factionInfo.faction(i).upperLimit();
		}
	}
	if (itor->second.num >= maxNum) {
		return -6;
	}


	factionMem.roleID = itorApplicant->second.roleID;
	factionMem.name = itorApplicant->second.name;
	factionMem.level = itorApplicant->second.level;
	factionMem.contribute = 0;
	factionMem.lastLoginTime = Time_TimeStamp();
	factionMem.type = 4;
	itor->second.factionMem.insert(factionMem);
	itor->second.num++;
	pool.applicantPool.erase(itorApplicant);

	{
		set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
		map<int64_t, FactionCache>::iterator it1;
		if (flag) {
			for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
				it1 = pool.cachePool.find(it2->roleID);
				if (it1 != pool.cachePool.end()) {
					it1->second.changePlayer.insert(factionMem);
				}
			}
		}
	}

	DCProto_FactionDelRecord delData;
	Applicant delApplicant;
	delApplicant.roleID = roleID;
	FactionPool_DelRecord(&delData, &delApplicant, NULL);

	
	DCProto_FactionUpdateRecord upData;
	FactionPool_UpdateRecord(&upData, NULL, &(pool.factionPool.find(att->faction())->second));

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"factionName\":\"%s", att->faction());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"factionNum\":\"%d", itor->second.num);
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(att->att().baseAtt().roleID());
		if (entity != NULL) {
			int32_t id  = PlayerEntity_ID(entity);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "AddMem", buf));
		}
	}
	return 0;
}

int FactionPool_DelMem(const PlayerAtt* att, int64_t roleID) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}
	
	FactionMem factionMem;
	factionMem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
	if (it == itor->second.factionMem.end()) {
		return -2;
	}
	int type1 = it->type;

	if (roleID == att->att().baseAtt().roleID()) {
		if (itor->second.num == 1) {
			FactionPool_DelFaction(att);
			return 0;
		}

		itor->second.num--;
		FactionMem factionChange = *it;
		itor->second.factionMem.erase(it);
		{
			set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
			map<int64_t, FactionCache>::iterator it1;
			for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
				it1 = pool.cachePool.find(it2->roleID);
				if (it1 != pool.cachePool.end()) {
					if (roleID == it1->first) {
						it1->second.changePlayer.clear();
					} else {
						it1->second.changePlayer.erase(factionChange);
						it1->second.changePlayer.insert(factionChange);
					}
				}
			}

			if (type1 == 1) {
				FactionMem mem;
				mem.contribute = -1;
				for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
					if (mem.contribute < it2->contribute) {
						mem = *it2;
					} else if (mem.contribute == it2->contribute) {
						if (mem.type > it2->type) {
							mem = *it2;
						}
					}
				}

				mem.type = 1;
				itor->second.name = mem.name;
				itor->second.factionMem.erase(mem);
				itor->second.factionMem.insert(mem);

				for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
					it1 = pool.cachePool.find(it2->roleID);
					if (it1 != pool.cachePool.end()) {
						it1->second.changePlayer.erase(mem);
						it1->second.changePlayer.insert(mem);
					}
				}
			}
			FactionPool_Signin(roleID, true);

			DCProto_FactionUpdateRecord data;
			FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(att->faction())->second));
		}
		return 0;
	}

	factionMem.roleID = roleID;
	it = itor->second.factionMem.find(factionMem);
	if (it == itor->second.factionMem.end()) {
		return -3;
	}
	int type2 = it->type;

	if (type1 >= type2) {
		DEBUG_LOG("insufficient permissions!");
		return -4;
	}

	{
		set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
		map<int64_t, FactionCache>::iterator it1;
		for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
			it1 = pool.cachePool.find(it2->roleID);
			if (it1 != pool.cachePool.end()) {
				if (roleID == it1->first) {
					it1->second.changePlayer.clear();
				} else {
					it1->second.changePlayer.erase(*it);
					it1->second.changePlayer.insert(*it);
				}
			}
		}
	}


	itor->second.num--;
	itor->second.factionMem.erase(it);

	if (itor->second.num != (int)itor->second.factionMem.size()) {
		DEBUG_LOG("KKKKKKKKK");
	}

	DCProto_FactionUpdateRecord data;
	FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(att->faction())->second));
			
	FactionPool_Signin(roleID, true);

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"factionName\":\"%s", att->faction());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"factionNum\":\"%d", itor->second.num);
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(att->att().baseAtt().roleID());
		if (entity != NULL) {
			int32_t id  = PlayerEntity_ID(entity);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "DelMem", buf));
		}
	}
	return 0;
}

int FactionPool_Applicant(const PlayerAtt* att, const char* str, int power, int vip) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor != pool.factionPool.end()) {
		return -1;
	}
	
	itor = pool.factionPool.find(str);
	if (itor == pool.factionPool.end()) {
		return -2;
	}

	map<int64_t, Applicant>::iterator itorApplicant = pool.applicantPool.find(att->att().baseAtt().roleID());
	if (itorApplicant != pool.applicantPool.end()) {
		itorApplicant->second.power = power;
		itorApplicant->second.vip = vip;
		itorApplicant->second.factionName.insert(str);
	
		for (std::set<std::string>::iterator itStr = itorApplicant->second.factionName.begin(); itStr != itorApplicant->second.factionName.end();) {
			map<string, Faction>::iterator itStrFactionName = pool.factionPool.find(*itStr);
			if (itStrFactionName == pool.factionPool.end()) {
				itorApplicant->second.factionName.erase(itStr++);
			}else {
				++itStr;
			}
		}


		DCProto_FactionUpdateRecord data;
		FactionPool_UpdateRecord(&data, &(pool.applicantPool.find(att->att().baseAtt().roleID())->second), NULL);
		return 0;
	}
	
	Applicant applicant;
	applicant.roleID = att->att().baseAtt().roleID();
	applicant.level = att->att().fightAtt().level();
	applicant.power = power;
	applicant.vip = vip;
	applicant.name = att->att().baseAtt().name();
	applicant.factionName.insert(str);
	pool.applicantPool[applicant.roleID] = applicant;
	
	DCProto_FactionAddRecord data;
	FactionPool_AddRecord(&data, &applicant, NULL);
	return 0;
}

void FactionPool_AnalyticalApplicant(string& str, Applicant& applicant) {
	int begin = 0;
	int end = 0;	
	
	end = str.find(',');
	applicant.roleID = 0;
	for (; begin < end; ++begin) {
		applicant.roleID = 10 * applicant.roleID + str[begin] - 48;
	}
DEBUG_LOG("DDDDDD:%lld", applicant.roleID);
	end = str.find(',', ++begin);
	applicant.level = 0;
	for (; begin < end; ++begin) {
		applicant.level = 10 * applicant.level + str[begin] - 48;
	}
	
DEBUG_LOG("DDDDDD:%d", applicant.level);
	end = str.find(',', ++begin);
	applicant.power = 0;
	for (; begin < end; ++begin) {
		applicant.power = 10 * applicant.power + str[begin] - 48;
	}
DEBUG_LOG("DDDDDD:%d", applicant.power);
	
	end = str.find(',', ++begin);
	applicant.vip = 0;
	for (; begin < end; ++begin) {
		applicant.vip = 10 * applicant.vip + str[begin] - 48;
	}
DEBUG_LOG("DDDDDD:%d", applicant.vip);

	end = str.find(',', ++begin);
	applicant.name = "";
	for (; begin < end; ++begin) {
		applicant.name = applicant.name + str[begin];
	}
DEBUG_LOG("DDDDDD:%s", applicant.name.c_str());

	string value = "";
	for (;(begin + 1) < str.length(); ++begin) {
		value.clear();
		end = str.find(',', ++begin);
		if (end > ((int)str.length() - 1)) {
			return;
		}
		for (; begin < end; ++begin) {
			value = value + str[begin];
		}
DEBUG_LOG("DDDDDD:%s", value.c_str());
		applicant.factionName.insert(value);
	}
}

void FactionPool_AnalyticalFactionMem(const string& str, Faction& faction) {
	int begin = 0;
	int end = 0;
	FactionMem factionMem;

	DEBUG_LOG("MEM = %s", str.c_str());
	for (;begin < ((int)str.length() - 1);) {
		factionMem.roleID = 0;
		end = str.find(',', begin);
		for (; begin < end; ++begin) {
			factionMem.roleID = 10 * factionMem.roleID + str[begin] - 48;
		}
		
	DEBUG_LOG("MEM2 = %lld", factionMem.roleID);
		end = str.find(',', ++begin);
		factionMem.name = "";
		for (; begin < end; ++begin) {
			factionMem.name = factionMem.name + str[begin];
		}

	DEBUG_LOG("MEM2 = %s", factionMem.name.c_str());
		end = str.find(',', ++begin);
		factionMem.level = 0;
		for (; begin < end; ++begin) {
			factionMem.level = 10 * factionMem.level + str[begin] - 48;
		}

	DEBUG_LOG("MEM2 = %d", factionMem.level);
		end = str.find(',', ++begin);
		factionMem.contribute = 0;
		for (; begin < end; ++begin) {
			factionMem.contribute = 10 * factionMem.contribute + str[begin] - 48;
		}
	DEBUG_LOG("MEM2 = %d", factionMem.contribute);

		end = str.find(',', ++begin);
		factionMem.lastLoginTime = 0;
		for (; begin < end; ++begin) {
			factionMem.lastLoginTime = 10 * factionMem.lastLoginTime + str[begin] - 48;
		}

	DEBUG_LOG("MEM2 = %d", factionMem.lastLoginTime);
		end = str.find(',', ++begin);
		factionMem.type = 0;
		for (; begin < end; ++begin) {
			factionMem.type = 10 * factionMem.type + str[begin] - 48;
		}
	DEBUG_LOG("MEM2 = %d", factionMem.type);
		++begin;
		faction.factionMem.insert(factionMem);
	}	
}

void FactionPool_Init(const DCProto_FactionLoadData* data) {
	if (data == NULL) {
		return;
	}

	for (int i = 0; i < data->data_size(); ++i) {
		if (data->data(i).factionName() == "") {
			string str = data->data(i).team();
			Applicant applicant;
			FactionPool_AnalyticalApplicant(str, applicant);
			pool.applicantPool[data->data(i).exp()] = applicant;
		} else {
			Faction faction;
			faction.name = data->data(i).name();
			faction.factionName = data->data(i).factionName();
			faction.num = data->data(i).num();
			faction.exp = data->data(i).exp();
			faction.lastLevelUpTime = data->data(i).exp_time();
			faction.item = data->data(i).item();
			faction.notice = data->data(i).notice();
			faction.guardian = data->data(i).guardian();
			FactionPool_AnalyticalFactionMem(data->data(i).team(), faction);
			pool.factionPool[data->data(i).factionName()] = faction;
		}
	}

}

void FactionPool_Signin(int64_t roleID, bool flag) {
	if (flag) {
		FactionCache cache;
		pool.cachePool[roleID] = cache;
		
		PlayerEntity *target = PlayerEntity_PlayerByRoleID(roleID);
		if (target == NULL) {
			return;
		}

		const PlayerAtt *att = PlayerEntity_Att(target);
		if (att == NULL) {
			return;
		}

		map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
		if (itor != pool.factionPool.end()) {
			FactionMem factionMem;
			factionMem.roleID = att->att().baseAtt().roleID();
			set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
			if (it != itor->second.factionMem.end()) {
				factionMem = *it;
				factionMem.lastLoginTime = Time_TimeStamp();
				itor->second.factionMem.erase(factionMem);
				itor->second.factionMem.insert(factionMem);
			}
		}
	} else {
		pool.cachePool.erase(roleID);
	}
}

void FactionPool_SetBits(int64_t roleID, int index) {
	map<int64_t, FactionCache>::iterator it = pool.cachePool.find(roleID);
	if (it == pool.cachePool.end()) {
		return;
	}
	if (index == 1) {
		it->second.bits |= 0x01;
	}
}

int FactionPool_FactionInfo(const PlayerAtt* att, NetProto_FactionInfo* info) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}

	info->set_name(itor->second.factionName);
	info->set_exp(itor->second.exp);
	info->set_num(itor->second.num);
	info->set_str(itor->second.notice);
	int rank = 0;
	for (map<string, Faction>::iterator it = pool.factionPool.begin(); it != pool.factionPool.end(); ++it) {
		if (info->exp() < it->second.exp)
		{
			info->set_rank(++rank);
		}
	}

	return 0;
}

int FactionPool_ChangeMem(const PlayerAtt* att, NetProto_FactionChangeMem* mem) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}

	int64_t roleID = att->att().baseAtt().roleID();
	map<int64_t, FactionCache>::iterator it = pool.cachePool.find(roleID);
	if (it == pool.cachePool.end()) {
		return -1;
	}

	if (it->second.bits & 0x01) {
		for (set<FactionMem, FactionMemComp>::iterator it2 = it->second.changePlayer.begin(); it2 != it->second.changePlayer.end(); ++it2) {
			FactionMem tmp;
			tmp.roleID = it2->roleID;
			NetProto_FactionMem* change = mem->add_data();
			set<FactionMem, FactionMemComp>::iterator it3 = itor->second.factionMem.find(tmp);
			if (it3 != itor->second.factionMem.end()) {
				mem->add_flag(true);
				change->set_roleID(it2->roleID);
				change->set_contribute(it2->contribute);
				change->set_name(it2->name);
				change->set_office(it2->type);
				change->set_lastLoginTime(it2->lastLoginTime);
				if (pool.cachePool.find(it2->roleID) != pool.cachePool.end()) {
					change->set_type(0);
				} else {
					change->set_type(it2->lastLoginTime);
				}
			} else {
				change->set_roleID(it2->roleID);
				mem->add_flag(false);
			}
		}
	} else {
		for (set<FactionMem, FactionMemComp>::iterator it4 = itor->second.factionMem.begin(); it4 != itor->second.factionMem.end(); ++it4) {
			mem->add_flag(true);
			NetProto_FactionMem* change = mem->add_data();
			change->set_roleID(it4->roleID);
			change->set_contribute(it4->contribute);
			change->set_name(it4->name);
			change->set_office(it4->type);
			change->set_lastLoginTime(it4->lastLoginTime);
			if (pool.cachePool.find(it4->roleID) != pool.cachePool.end()) {
				change->set_type(0);
			} else {
				change->set_type(it4->lastLoginTime);
			}
		}
		FactionPool_SetBits(roleID, 1);
	}

	it->second.changePlayer.clear();

	return 0;
}

int FactionPool_ApplicantListRequest(const PlayerAtt* att, NetProto_FactionAllApplicant* proto) {
	for (map<int64_t, Applicant>::iterator itor = pool.applicantPool.begin(); itor != pool.applicantPool.end(); ++itor) {
		if (itor->second.factionName.find(att->faction()) != itor->second.factionName.end()) {
			NetProto_FactionApplicant* tmp = proto->add_data();
			tmp->set_level(itor->second.level);	
			tmp->set_power(itor->second.power);	
			tmp->set_vip(itor->second.vip);	
			tmp->set_name(itor->second.name);	
			tmp->set_roleID(itor->second.roleID);	
		}
	}
	return 0;	
}

bool Sort_Faction(const Faction& tmp1, const Faction& tmp2) {
	if (tmp1.exp > tmp2.exp) {
		return true;
	} else if (tmp1.exp == tmp2.exp) {
		if (tmp1.lastLevelUpTime > tmp2.lastLevelUpTime) {
			return true;
		}
	}

	return false;
}

int FactionPool_FactionListRequest(const PlayerAtt* att, NetProto_FactionList* proto) {
	if (att == NULL) 
		return -1;

	map<string, Faction> factionPool;
	vector<Faction> vec;
	for (map<string, Faction>::iterator itor = pool.factionPool.begin(); itor != pool.factionPool.end(); ++itor) {
		vec.push_back(itor->second);
	}

	sort(vec.begin(), vec.end(), Sort_Faction);
	int rank = 1;
	for (vector<Faction>::iterator it = vec.begin(); it != vec.end() /*&& rank <= 20*/; ++it) {
		NetProto_FactionInfo* info = proto->add_info();
		info->set_name(it->factionName);
		info->set_rank(rank++);
		info->set_exp(it->exp);
		info->set_num(it->num);
		info->set_str(it->name);
	}


	map<int64_t, Applicant>::iterator itApplicant = pool.applicantPool.find(att->att().baseAtt().roleID());
	if (itApplicant != pool.applicantPool.end()) {
		for (set<std::string>::iterator it1 = itApplicant->second.factionName.begin(); it1 != itApplicant->second.factionName.end(); ++it1) {
			proto->add_str(*it1);	
		}	
	}
	return 0;	
}


void FactionPool_AddRecord(DCProto_FactionAddRecord* data, Applicant* applicant, Faction* faction) {
	DCProto_FactionData* tmp = data->mutable_data();
	char value[CONFIG_FIXEDARRAY * 10];
	memset(value, 0, sizeof(value));
	char *p = value;
	char *pp;
	int index = 0;

	if (applicant != NULL) {
		tmp->set_factionName("");
		tmp->set_exp(applicant->roleID);
		SNPRINTF1(value, "%lld,%d,%d,%d,%s,", (long long)applicant->roleID, applicant->level, applicant->power, applicant->vip, applicant->name.c_str());

		for (set<string>::iterator itor = applicant->factionName.begin(); itor != applicant->factionName.end(); ++itor) {
			index = strlen(value);
			pp = p + index;
			SNPRINTF2(value, pp, "%s,", (*itor).c_str());
		}

		tmp->set_team(value);	
	}

	if (faction != NULL) {
		tmp->set_factionName(faction->factionName);
		tmp->set_name(faction->name);
		tmp->set_num(faction->num);
		tmp->set_exp(faction->exp);
		tmp->set_exp_time(faction->lastLevelUpTime);
		tmp->set_item(faction->item);
		tmp->set_notice(faction->notice);
		tmp->set_guardian(faction->guardian);

		for (set<FactionMem, FactionMemComp>::iterator itor = faction->factionMem.begin(); itor != faction->factionMem.end(); itor++) {
			index = strlen(value);
			pp = p + index;
			SNPRINTF2(value, pp, "%lld,%s,%d,%d,%d,%d,", (long long)itor->roleID, itor->name.c_str(), itor->level, itor->contribute, itor->lastLoginTime, itor->type);
		}
		tmp->set_team(value);
	}

	DEBUG_LOG("NNNNNNNNNNNNNNNNNNN");
	GCAgent_SendProtoToDCAgent(data);
}

void FactionPool_DelRecord(DCProto_FactionDelRecord* data, Applicant* applicant, Faction* faction) {
	DCProto_FactionData* tmp = data->mutable_data();

	if (applicant != NULL) {
		tmp->set_factionName("");
		tmp->set_exp(applicant->roleID);
	}

	if (faction != NULL) {
		tmp->set_factionName(faction->factionName);
	}

	GCAgent_SendProtoToDCAgent(data);

}

void FactionPool_UpdateRecord(DCProto_FactionUpdateRecord* data, Applicant* applicant, Faction* faction) {
	DCProto_FactionData* tmp = data->mutable_data();
	char value[CONFIG_FIXEDARRAY * 10];
	memset(value, 0, sizeof(value));
	char *p = value;
	char *pp;
	int index = 0;

	if (applicant != NULL) {
		tmp->set_factionName("");
		tmp->set_exp(applicant->roleID);
		SNPRINTF1(value, "%lld,%d,%d,%d,%s,", (long long)applicant->roleID, applicant->level, applicant->power, applicant->vip, applicant->name.c_str());

		for (set<string>::iterator itor = applicant->factionName.begin(); itor != applicant->factionName.end(); ++itor) {
			index = strlen(value);
			pp = p + index;
			SNPRINTF2(value, pp, "%s,", (*itor).c_str());
		}

		tmp->set_team(value);	
	}

	if (faction != NULL) {
		tmp->set_factionName(faction->factionName);
		tmp->set_name(faction->name);
		tmp->set_num(faction->num);
		tmp->set_exp(faction->exp);
		tmp->set_exp_time(faction->lastLevelUpTime);
		tmp->set_item(faction->item);
		tmp->set_notice(faction->notice);
		tmp->set_guardian(faction->guardian);

		for (set<FactionMem, FactionMemComp>::iterator itor = faction->factionMem.begin(); itor != faction->factionMem.end(); itor++) {
			index = strlen(value);
			pp = p + index;
			SNPRINTF2(value, pp, "%lld,%s,%d,%d,%d,%d,", (long long)itor->roleID, itor->name.c_str(), itor->level, itor->contribute, itor->lastLoginTime, itor->type);
		}
		tmp->set_team(value);
	}

	GCAgent_SendProtoToDCAgent(data);

}


int FactionPool_Donate(const PlayerAtt* att, int type, int contribute) {
	int expend = 0;
	int dayEvent = 0;
	int factionExp = 0;
	int exp = 0;
	int item = 0;
	if (contribute == -1) {
		Config_FactionDoante(type, &expend, &dayEvent, &factionExp, &exp, &item);
	}

	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		DEBUG_LOG("you have no faction!");
		return -1;
	}

	if (contribute == -1) {
		itor->second.exp += factionExp;
		itor->second.item += item;
	}

	FactionMem mem;
	mem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(mem);

	if (it == itor->second.factionMem.end()) {
		return -2;
	}

	mem = *it;

	if (contribute == -1) {
		mem.contribute += exp;
	}else {
		mem.contribute += contribute;
	}
	itor->second.factionMem.erase(it);
	itor->second.factionMem.insert(mem);

	{
		set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
		map<int64_t, FactionCache>::iterator it1;
		for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
			it1 = pool.cachePool.find(it2->roleID);
			if (it1 != pool.cachePool.end()) {
				it1->second.changePlayer.erase(mem);
				it1->second.changePlayer.insert(mem);
			}
		}
	}

	DCProto_FactionUpdateRecord upData;
	FactionPool_UpdateRecord(&upData, NULL, &(pool.factionPool.find(att->faction())->second));

	if (Config_ScribeHost() != NULL) {
		static char buf[CONFIG_FIXEDARRAY];
		memset(buf, 0, sizeof(buf) / sizeof(char));
		char *index = buf;

		SNPRINTF2(buf, index, "{\"factionName\":\"%s", att->faction());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"factionExp\":\"%lld", (long long int)itor->second.exp);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleID\":\"%lld", (long long int)att->att().baseAtt().roleID());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleName\":\"%s", att->att().baseAtt().name());
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleLevel\":\"%d", mem.level);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleContribute\":\"%d", mem.contribute);
		index += strlen(index);
		SNPRINTF2(buf, index, "\",\"roleType\":\"%d", mem.type);
		index += strlen(index);
		SNPRINTF2(buf, index, "\"}");
		struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(att->att().baseAtt().roleID());
		if (entity != NULL) {
			int32_t id  = PlayerEntity_ID(entity);
			ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "FactionExp", buf));
		}
	}
	return 0;
}

int FactionPool_Guardian(const PlayerAtt* att, int type) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		return -1;
	}

	string str = itor->second.guardian;
	if (str.length() == 0) {
		str = "0,0,0,0,";
	}

	FactionMem factionMem;
	factionMem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
	if (it == itor->second.factionMem.end()) {
		return -2;
	}
	
	if (!(it->type == 1 || it->type == 2)) {
		return -3;
	}

	int64_t exp = itor->second.exp;
	int curLevel = 0;
	for (int i = 0; i < (int)pool.factionInfo.faction_size(); ++i) {
		if (exp >= pool.factionInfo.faction(i).exp()) {
			curLevel = pool.factionInfo.faction(i).level();
		}
	}

	basic_string<char>::iterator iterF0, iterL0;
	string::size_type pos;
	int tmp = 0;
	char ch[8];
	bool flag = false;

	if (type == 0) {
		pos = str.find(',');
		for (string::size_type sz = 0; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}

		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == type && pool.sociatySkillInfo.info(i).level() == (tmp + 1) && pool.sociatySkillInfo.info(i).requiredSociatyLevel() <= curLevel && pool.sociatySkillInfo.info(i).needBless() <= itor->second.item) {
				flag = true;
				itor->second.item -= pool.sociatySkillInfo.info(i).needBless();
				break;
			}
		}

		if (!flag) {
			return -4;
		}

		iterF0 = str.begin();
		iterL0 = str.begin() + pos;
		SNPRINTF1(ch, "%d", ++tmp);
		str.replace(iterF0, iterL0, string(ch));
	} else if (type == 1) {
		pos = str.find(',');
		string::size_type sz = pos + 1;
		
		iterF0 = str.begin() + sz;
		pos = str.find(',', sz);
		
		for (; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}

		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == type && pool.sociatySkillInfo.info(i).level() == (tmp + 1) && pool.sociatySkillInfo.info(i).requiredSociatyLevel() <= curLevel && pool.sociatySkillInfo.info(i).needBless() <= itor->second.item) {
				flag = true;
				itor->second.item -= pool.sociatySkillInfo.info(i).needBless();
				break;
			}
		}

		if (!flag) {
			return -4;
		}

		iterL0 = str.begin() + pos;
		SNPRINTF1(ch, "%d", ++tmp);
		str.replace(iterF0, iterL0, string(ch));
	} else if (type == 2) {
		pos = str.find(',');
		pos = str.find(',', pos + 1);
		string::size_type sz = pos + 1;
		
		iterF0 = str.begin() + sz;

		pos = str.find(',', sz);
		for (; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}

		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == type && pool.sociatySkillInfo.info(i).level() == (tmp + 1) && pool.sociatySkillInfo.info(i).requiredSociatyLevel() <= curLevel && pool.sociatySkillInfo.info(i).needBless() <= itor->second.item) {
				flag = true;
				itor->second.item -= pool.sociatySkillInfo.info(i).needBless();
				break;
			}
		}

		if (!flag) {
			return -4;
		}

		iterL0 = str.begin() + pos;
		SNPRINTF1(ch, "%d", ++tmp);
		str.replace(iterF0, iterL0, string(ch));
	
	} else if (type == 3) {
		pos = str.find(',');
		pos = str.find(',', pos + 1);
		pos = str.find(',', pos + 1);
		string::size_type sz = pos + 1;
		
		iterF0 = str.begin() + sz;

		pos = str.find(',', sz);
		for (; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}

		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == type && pool.sociatySkillInfo.info(i).level() == (tmp + 1) && pool.sociatySkillInfo.info(i).requiredSociatyLevel() <= curLevel && pool.sociatySkillInfo.info(i).needBless() <= itor->second.item) {
				flag = true;
				itor->second.item -= pool.sociatySkillInfo.info(i).needBless();
				break;
			}
		}

		if (!flag) {
			return -4;
		}

		iterL0 = str.begin() + pos;
		SNPRINTF1(ch, "%d", ++tmp);
		str.replace(iterF0, iterL0, string(ch));
	} else {
		return -1;
	}

	itor->second.guardian = str;
	DCProto_FactionUpdateRecord upData;
	FactionPool_UpdateRecord(&upData, NULL, &(pool.factionPool.find(att->faction())->second));
	return 0;
}

int FactionPool_FactionGuardian(const PlayerAtt* att, NetProto_FactionGuardian* proto) {
	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		return -1;
	}

	string str = itor->second.guardian;
	if (str.length() == 0) {
		str = "0,0,0,0,";
	}

	proto->set_item(itor->second.item);
	proto->set_str(str);

	return 0;
}

int FactionPool_Check(int64_t roleID, const char* str) {
	if (str == NULL) {
		return -1;
	}
	map<string, Faction>::iterator itor = pool.factionPool.find(str); 
	if (itor != pool.factionPool.end()) {
		FactionMem factionMem;
		factionMem.roleID = roleID;
		set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
		if (it != itor->second.factionMem.end()) {
			return -2;
		}
	}
	
	return 0;
}

void FactionPool_GetFactionGurdianData(string str, int* vData1, double* vData2, bool flag) {
	if (str == "") {
		return;
	}
	map<string, Faction>::iterator itor = pool.factionPool.find(str);
	if (itor == pool.factionPool.end()) {
		return;
	}
	
	string strValue = itor->second.guardian;
	if (strValue == "") {
		strValue = "0,0,0,0,";
	}

	static string::size_type pos;
	static string::size_type sz;
	static int tmp = 0;
	if (flag) {
		pos  = 0;
		sz = 0;
		tmp = 0;
		pos = strValue.find(',');
		for (sz = 0; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}
		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == 0 && pool.sociatySkillInfo.info(i).level() == tmp) {
				*vData1 = pool.sociatySkillInfo.info(i).effectValue();
				*vData2 = pool.sociatySkillInfo.info(i).effectPercent();
				break;
			}
		}
		
		pos = strValue.find(',', pos + 1);
		pos = strValue.find(',', pos + 1);
		sz = pos + 1;
		pos = strValue.find(',', pos + 1);

		tmp = 0;
		for (; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}
		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == 3 && pool.sociatySkillInfo.info(i).level() == tmp) {
				*(++vData1) = pool.sociatySkillInfo.info(i).effectValue();
				*(++vData2) = pool.sociatySkillInfo.info(i).effectPercent();
				break;
			}
		}
	} else {
		pos  = 0;
		sz = 0;
		tmp = 0;

		pos = strValue.find(',');
		sz = pos + 1;
		pos = strValue.find(',', pos + 1);
		
		for (sz = 0; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}
		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == 1 && pool.sociatySkillInfo.info(i).level() == tmp) {
				*vData1 = *vData1 - pool.sociatySkillInfo.info(i).effectValue();
				if (*vData1 < 0) {
					*vData1 = 0;
				}
				*vData2 = *vData2 - pool.sociatySkillInfo.info(i).effectPercent();
				if (vData2 < 0) {
					vData2 = 0;
				}
				break;
			}
		}
		
		sz = pos + 1;
		pos = strValue.find(',', pos + 1);
		tmp = 0;
		for (; sz < pos; sz++) {
			tmp = 10 * tmp + str[sz] - 48;
		}
		for (int i = 0; i < pool.sociatySkillInfo.info_size(); ++i) {
			if (pool.sociatySkillInfo.info(i).effectType() == 2 && pool.sociatySkillInfo.info(i).level() == tmp) {
				*(++vData1) -= pool.sociatySkillInfo.info(i).effectValue();
				if (*(++vData1) < 0) {
					double dm = -(*(++vData1));
					*(++vData1) = dm;
				}
				*(++vData2) -= pool.sociatySkillInfo.info(i).effectPercent();
				if (*(++vData2) < 0) {
					*(++vData2) = 0;
				}
				break;
			}
		}
	}
}

void FactionPool_SynFactionLevel(const PlayerAtt* att) {
	if (att == NULL) {
		return;
	}

	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		return;
	}

	{
		set<FactionMem, FactionMemComp>* tmp = &itor->second.factionMem;
		FactionMem factionMem;
		factionMem.roleID = att->att().baseAtt().roleID();
		set<FactionMem, FactionMemComp>::iterator it = tmp->find(factionMem);
		if (it != tmp->end()) {
			factionMem = *it;

			factionMem.level = att->att().fightAtt().level();
			map<int64_t, FactionCache>::iterator it1;
			for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
				it1 = pool.cachePool.find(it2->roleID);
				if (it1 != pool.cachePool.end()) {
					it1->second.changePlayer.erase(factionMem);
					it1->second.changePlayer.insert(factionMem);
				}
			}

			DCProto_FactionUpdateRecord upData;
			FactionPool_UpdateRecord(&upData, NULL, &(pool.factionPool.find(att->faction())->second));
		}
	}
}

void FactionPool_InitSociaty() {

	{
		pool.factionInfo.Clear();

		string src = Config_DataPath() + string("/Sociaty.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!pool.factionInfo.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}

	{
		pool.sociatySkillInfo.Clear();
		string src = Config_DataPath() + string("/SociatySkill.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!pool.sociatySkillInfo.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}

}

int FactionPool_GetFactionMem(const PlayerAtt* att, int* id) {
	if (att == NULL) {
		return -1;
	}

	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		return -2;
	}

	int count = 0;
	set<FactionMem, FactionMemComp>* tmp = &itor->second.factionMem;
	for (set<FactionMem, FactionMemComp>::iterator it = tmp->begin(); it != tmp->end(); ++it) {
		PlayerEntity *target = PlayerEntity_PlayerByRoleID(it->roleID);
		if (target == NULL) {
			continue;
		}else {
			*(id + count++) = PlayerEntity_ID(target);
		}
	}
   return count;	
}

int FactionPool_GetRoleContribute(const PlayerAtt* att) {
	if (att == NULL) {
		return 0;
	}

	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		return 0;
	}

	FactionMem mem;
	mem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(mem);
	if (it == itor->second.factionMem.end()) {
		return 0;
	}

	return it->contribute;
}

void FactionPool_UpateRoleContribute(const PlayerAtt* att, int detail) {
	if (att == NULL) {
		return;
	}

	map<string, Faction>::iterator itor = pool.factionPool.find(att->faction());
	if (itor == pool.factionPool.end()) {
		return;
	}

	FactionMem mem;
	mem.roleID = att->att().baseAtt().roleID();
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(mem);
	if (it == itor->second.factionMem.end()) {
		return;
	}
	mem = *it;
	mem.contribute = (mem.contribute + detail) > 0 ? (mem.contribute + detail) : 0;
	itor->second.factionMem.erase(mem);
	itor->second.factionMem.insert(mem);
	
	DCProto_FactionUpdateRecord data;
	FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(att->faction())->second));

	{
		set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(att->faction())->second.factionMem;
		map<int64_t, FactionCache>::iterator it1;
		for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
			it1 = pool.cachePool.find(it2->roleID);
			if (it1 != pool.cachePool.end()) {
				it1->second.changePlayer.erase(mem);
				it1->second.changePlayer.insert(mem);
			}
		}
	}

}

void FactionPool_AddExpAndItem(const char* str, int exp, int item) {
	if (str == NULL)
		return;

	map<string, Faction>::iterator itor = pool.factionPool.find(str);
	if (itor == pool.factionPool.end()) {
		return;
	}
	
	itor->second.exp += exp;
	itor->second.item += item;
	return;
}

void FactionPool_AddMemContribute(const char* str, int num, int64_t* roleIDs, int count) {
	if (str == NULL)
		return;

	map<string, Faction>::iterator itor = pool.factionPool.find(str);
	if (itor == pool.factionPool.end()) {
		return;
	}

	FactionMem mem;
	for (int i = 0; i < count; ++i) {
		mem.roleID = *(roleIDs + i);
		set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(mem);
		if (it == itor->second.factionMem.end()) {
			continue;
		}
		mem = *it;
		mem.contribute = (mem.contribute + num) > 0 ? (mem.contribute + num) : 0;
		itor->second.factionMem.erase(mem);
		itor->second.factionMem.insert(mem);
		
		{
			set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(str)->second.factionMem;
			map<int64_t, FactionCache>::iterator it1;
			for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
				it1 = pool.cachePool.find(it2->roleID);
				if (it1 != pool.cachePool.end()) {
					it1->second.changePlayer.erase(mem);
					it1->second.changePlayer.insert(mem);
				}
			}
		}
	}

	DCProto_FactionUpdateRecord data;
	FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(str)->second));
	return;
}

void FactionPool_GetRolesFromFaction(const char* str, int bit, vector<int64_t>* vec) {
	if (str == NULL || vec == NULL) {
		return;
	}
	
	map<string, Faction>::iterator itor = pool.factionPool.find(str);
	if (itor == pool.factionPool.end()) {
		return;
	}

	if (bit & 0x01) {
		for (set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.begin(); it != itor->second.factionMem.end(); ++it) {
			if (1 == it->type) {
				vec->push_back(it->roleID);
				break;
			}
		}
	}

	if (bit & 0x02) {
		for (set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.begin(); it != itor->second.factionMem.end(); ++it) {
			if (2 == it->type) {
				vec->push_back(it->roleID);
			}
		}
	}

	if (bit & 0x04) {
		for (set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.begin(); it != itor->second.factionMem.end(); ++it) {
			if (3 == it->type) {
				vec->push_back(it->roleID);
			}
		}
	}

	if (bit & 0x08) {
		for (set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.begin(); it != itor->second.factionMem.end(); ++it) {
			if (4 == it->type) {
				vec->push_back(it->roleID);
			}
		}
	}
}

void FactionPool_GMQueryFaction(NetProto_GMQueryFaction * info) {
	if (info == NULL)
		return;
	
	map<string, Faction>::iterator itor = pool.factionPool.find(info->info().name());
	if (itor == pool.factionPool.end())
		return;

	info->mutable_info()->set_exp(itor->second.exp);
	info->mutable_info()->set_num(itor->second.num);
	info->mutable_info()->set_str(itor->second.notice);

	int64_t exp = itor->second.exp;
	int level = 0;
	for (int i = 0; i < (int)pool.factionInfo.faction_size(); ++i) {
		if (exp >= pool.factionInfo.faction(i).exp()) {
			level = pool.factionInfo.faction(i).level();
		}
	}
	info->mutable_info()->set_level(level);

	for (set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.begin(); it != itor->second.factionMem.end(); ++it) {
		NetProto_FactionMem *mem = info->add_mem();
		mem->set_roleID(it->roleID);
		mem->set_name(it->name);
		mem->set_contribute(it->contribute);
		mem->set_office(it->type);
		mem->set_lastLoginTime(it->lastLoginTime);
		
		if (pool.cachePool.find(it->roleID) != pool.cachePool.end()) {
			mem->set_type(0);
		} else {
			mem->set_type(it->lastLoginTime);
		}
	}
	
	int64_t hurt = 0;	
	PlayerPool_GetWinFactionFight(MapPool_FactionWarNum(), info->mutable_winName(), &hurt);
}

void FactionPool_GMChangeFactionMem(NetProto_GMChangeFactionMem * info) {
	if (info == NULL)
		return;

	map<string, Faction>::iterator itor = pool.factionPool.find(info->factionName());
	if (itor == pool.factionPool.end())
		return;


	if (info->flag()) {
		for (set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.begin(); it != itor->second.factionMem.end(); ++it) {
			if (it->type == 1) {
				return;
			}
		}

		struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(info->roleID());
		if (entity == NULL) 
			return;

		const PlayerAtt * att = PlayerEntity_Att(entity);
		if (att == NULL)
			return;

		FactionMem factionMem;
		factionMem.roleID = info->roleID();

		if (string("").compare(att->faction()) == 0) {
			factionMem.contribute = 0;
			factionMem.lastLoginTime = Time_TimeStamp();
			factionMem.type = 1;
			factionMem.name = att->att().baseAtt().name();
			factionMem.level = att->att().fightAtt().level();
			itor->second.factionMem.insert(factionMem);
			itor->second.num++;
			itor->second.name = att->att().baseAtt().name();
			PlayerEntity_SysFactionMem(info->roleID(), info->factionName().c_str());
		}else if (info->factionName().compare(att->faction()) == 0) {
			set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
			if (it == itor->second.factionMem.end()) {
				return;
			}
			factionMem.contribute = it->contribute;
			factionMem.lastLoginTime = Time_TimeStamp();
			factionMem.type = 1;
			factionMem.name = att->att().baseAtt().name();
			factionMem.level = att->att().fightAtt().level();
			itor->second.factionMem.erase(factionMem);
			itor->second.factionMem.insert(factionMem);
			itor->second.name = att->att().baseAtt().name();
			PlayerEntity_SysFactionMem(info->roleID(), info->factionName().c_str());
		}else {
			DEBUG_LOGERROR("GM operator error");
			return;
		}

		{
			set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(info->factionName())->second.factionMem;
			map<int64_t, FactionCache>::iterator it1;
			for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
				it1 = pool.cachePool.find(it2->roleID);
				if (it1 != pool.cachePool.end()) {
					it1->second.changePlayer.insert(factionMem);
				}
			}
		}

		int32_t id  = PlayerEntity_ID(entity);
		static NetProto_AcceptToFaction accept;
		accept.set_factionName(info->factionName());
		GCAgent_SendProtoToClients(&id, 1, &accept);
		
		DCProto_FactionUpdateRecord upData;
		FactionPool_UpdateRecord(&upData, NULL, &(pool.factionPool.find(info->factionName())->second));
	}else {
		FactionMem factionMem;
		factionMem.roleID = info->roleID();
		set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(factionMem);
		if (it == itor->second.factionMem.end()) {
			return;
		}

		itor->second.num--;
		FactionMem factionChange = *it;
		itor->second.factionMem.erase(it);

		set<FactionMem, FactionMemComp>* tmp = &pool.factionPool.find(info->factionName())->second.factionMem;
		map<int64_t, FactionCache>::iterator it1;
		for (set<FactionMem, FactionMemComp>::iterator it2 = tmp->begin(); it2 != tmp->end(); ++it2) {
			it1 = pool.cachePool.find(it2->roleID);
			if (it1 != pool.cachePool.end()) {
				if (info->roleID() == it1->first) {
					it1->second.changePlayer.clear();
				} else {
					it1->second.changePlayer.erase(factionChange);
					it1->second.changePlayer.insert(factionChange);
				}
			}
		}

		struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(info->roleID());
		if (entity != NULL)  {
			const PlayerAtt * att = PlayerEntity_Att(entity);
			if (att != NULL) {
				int32_t id  = PlayerEntity_ID(entity);
				static NetProto_AcceptToFaction accept;
				accept.set_factionName("");
				GCAgent_SendProtoToClients(&id, 1, &accept);
			}
			PlayerEntity_SysFactionMem(info->roleID(), "");
		}

		
		DCProto_FactionUpdateRecord data;
		FactionPool_UpdateRecord(&data, NULL, &(pool.factionPool.find(info->factionName())->second));
	}
}

int FactionPool_GetMemOffic(const char* str, int64_t roleID) {
	if (str == NULL)
		return 0;

	map<string, Faction>::iterator itor = pool.factionPool.find(str);
	if (itor == pool.factionPool.end()) {
		return 0;
	}

	FactionMem mem;
	mem.roleID = roleID;
	set<FactionMem, FactionMemComp>::iterator it = itor->second.factionMem.find(mem);
	if (it == itor->second.factionMem.end()) {
		return 0;
	}

	return it->type;
}

