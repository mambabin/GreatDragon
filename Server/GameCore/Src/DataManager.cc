#include "DataManager.hpp"
#include "PlayerAtt.hpp"
#include "EquipmentInfo.hpp"
#include "DirUtil.hpp"
#include "Fight.hpp"
#include "Item.hpp"
#include "DCAgent.hpp"
#include "Debug.hpp"
#include "ConfigUtil.hpp"
#include "DCProto.pb.h"
#include "Fashion.pb.h"
#include "PlayerEntity.hpp"
#include "Time.hpp"
#include "GoodsInfoManager.hpp"
#include "EquipmentInfoManager.hpp"
#include "FashionInfoManager.hpp"
#include "PlayOffManager.hpp"
#include "Web.hpp"
#include "Timer.hpp"
#include "SignalHandler.hpp"
#include "NPCInfoManager.hpp"
#include <mysql/mysql.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <set>
#include <map>
#include <deque>
#include <cstdio>
#include "MathUtil.hpp"
#include "PlayerPool.hpp"

using namespace std;

#define SEM_NAME "snapshot"

struct SavingRole {
	PB_PlayerAtt *data;
	PlayerInfo *info;
	bool insert;
};

static struct{
	MYSQL *conn;
	MYSQL_RES *res;

	MYSQL *connPublic;
	MYSQL_RES *resPublic;

	MYSQL *connRecharge;
	MYSQL_RES *resRecharge;

	map<int64_t, string> roleCache;
	map<string, int64_t> nameToID;

	// Saving thread
	vector<struct SavingRole> saving;
	pthread_mutex_t mutex;
	bool over;
	pthread_t savingThread;
}package;

template<typename Proto>
static char * MakeBlobQuery(MYSQL* conn, char *sql, size_t total, const char *tail, const Proto *proto) {
	char realData[CONFIG_FIXEDARRAY * 100];
	size_t size = proto->ByteSize();
	if (size > sizeof(realData))
		return NULL;
	proto->SerializeToArray(realData, size);

	char *end = sql + strlen(sql);
	if (total - (end - sql) - strlen(tail) <= size * 2 + 1)
		return NULL;
	end += mysql_real_escape_string(conn, end, realData, size);
	strcpy(end, tail);
	return end + strlen(tail);
}

static bool SaveRoleToDB(MYSQL *conn, const PB_PlayerAtt *data, const PlayerInfo *info, bool insert) {
	int64_t id = data->att().baseAtt().roleID();

	char lastLoginTime[32] = {'\0'};
	Time_ToDateTime(data->prevLogin(), lastLoginTime);
	char createTime[32] = {'\0'};
	Time_ToDateTime(data->createTime(), createTime);
	int friends_num = 0;
	for (int i = 0; i < data->friends_size(); i++)
		if (data->friends(i).roleID() >= 0)
			friends_num++;
	for (int i = 0; i < data->fans_size(); i++)
		if (data->fans(i).roleID() >= 0)
			friends_num++;

	static char sql[CONFIG_FIXEDARRAY * 100];
	if (insert) {
		if (info == NULL)
			return false;
		SNPRINTF1(sql, "insert into %s ("
				"id, name," // 1 2
				"uid," // 3
				"first_login_ip," // 8
				"last_login_time, last_login_ip," // 9 10
				"total_num, login_num," // 11 12
				"occupational, create_time," // 13 14
				"level, vip_level," // 15 16
				"exp, goldingot," // 17 18
				"friends_num," // 19
				"channel, line, power, tower, worldBossHurt, worldBossNum, bless, luck, selfcode, othercode, grouppurchase, " // 23 24+
				"data) values(" // 25
				"%lld,'%s'," // 1 2
				"'%s'," // 3
				"'%s'," // 8
				"'%s','%s'," // 9 10
				"%d,%d," // 11 12
				"'%s','%s'," // 13 14
				"%d,%d," // 15 16
				"%lld,%lld," // 17 18
				"%d," // 19
				"'%s',%d,%d,%d,%d,%d,%d,%d,'%s','%s','%d'," // 23 24 26 27
				"'", // 25
			Config_RoleTable(),
			(long long)id, data->att().baseAtt().name().c_str(), // 1 2
			info->account().c_str(), // 3
			data->firstLoginIP().c_str(), // 8
			lastLoginTime, data->lastLoginIP().c_str(), // 9 10
			data->totalNum(), data->loginNum(), // 11 12
			PB_ProfessionInfo::Type_Name(data->att().baseAtt().professionType()).c_str(), createTime, // 13 14
			data->att().fightAtt().level(), data->itemPackage().vip(), // 15 16
			(long long)data->att().fightAtt().exp(), (long long)data->itemPackage().rmb(), // 17 18
			friends_num, // 19
			info->platform().c_str(), Config_Line(), Fight_Power(&(data->att().fightAtt())), data->att().fightAtt().maxTower(), data->att().fightAtt().worldBossHurt(), data->att().fightAtt().worldBossNum(), data->itemPackage().blessActive(), data->fixedEvent(88), data->selfcode().c_str(), data->othercode().c_str(), data->fixedEvent(109)
			  	//23,24,26,27 
				);
		char *end = MakeBlobQuery(conn, sql, sizeof(sql), "')", data);
		if (end == NULL) {
			DEBUG_LOGERROR("Failed to add role");
			return false;
		}
		if (mysql_real_query(conn, sql, end - sql) != 0) {
			DEBUG_LOGERROR("Failed to query, error: %s", mysql_error(conn));
			return false;
		}
	} else {
		SNPRINTF1(sql, "update %s set "
				"last_login_time='%s'," // 4
				"last_login_ip='%s'," // 5
				"total_num=%d," // 6
				"login_num=%d," // 7
				"level=%d," // 8
				"vip_level=%d," // 9
				"exp=%lld," // 10
				"goldingot=%lld," // 11
				"friends_num=%d," // 12
				"power=%d," // 13
				"tower=%d," // 14
				"worldBossHurt=%d," // 
				"worldBossNum=%d," // 
				"bless=%d," // +
				"luck=%d,"
				"selfcode='%s',"
				"othercode='%s',"
				"grouppurchase=%d,"
				"data='",
				Config_RoleTable(),
				lastLoginTime, // 4
				data->lastLoginIP().c_str(), // 5
				data->totalNum(), // 6
				data->loginNum(), // 7
				data->att().fightAtt().level(), // 8
				data->itemPackage().vip(), // 9
				(long long)data->att().fightAtt().exp(), // 10
				(long long)data->itemPackage().rmb(), // 11
				friends_num, // 12
				Fight_Power(&(data->att().fightAtt())), // 13
				data->att().fightAtt().maxTower(), // 14
				data->att().fightAtt().worldBossHurt(),
				data->att().fightAtt().worldBossNum(),
				data->itemPackage().blessActive(),
				data->fixedEvent(88),
				data->selfcode().c_str(), // 6
				data->othercode().c_str(), // 6
				data->fixedEvent(109)
					);

		char tail[CONFIG_FIXEDARRAY];
		SNPRINTF1(tail, "' where id=%lld", (long long)id);
		char *end = MakeBlobQuery(conn, sql, sizeof(sql), tail, data);
		if (end == NULL) {
			DEBUG_LOGERROR("Failed to save role");
			return false;
		}

		if (mysql_real_query(conn, sql, end - sql) != 0) {
			DEBUG_LOGERROR("Failed to query, error: %s", mysql_error(conn));
			return false;
		}

		my_ulonglong res = mysql_affected_rows(conn);
		if (res == (my_ulonglong)-1) {
			DEBUG_LOGERROR("Failed to update, error: %s", mysql_error(conn));
			return false;
		} else if (res != 1) {
			// DEBUG_LOGERROR("Failed to update");
			// return;
		}
	}


	const NPCAtt* pet = NULL;
	char ally[128];
	int quality = 0;
	int level = 0;
	PB_NPCAtt npcAtt;
	for (int i = 0; i < data->pets_size(); ++i) {
		if (data->pets(i).id() != -1) {
			quality = data->pets(i).quality();
			level = data->pets(i).level();

			pet =  NPCInfoManager_Pet(data->pets(i).id(), quality, level);
			if (pet == NULL) {
				continue;
			}
			
			SNPRINTF1(ally, "%s,%s", data->att().baseAtt().name().c_str(), pet->att().baseAtt().name());
			pet->ToPB(&npcAtt);
			PB_FightAtt fightAtt = npcAtt.att().fightAtt();
			
			int32_t power = Fight_PetPower(data, i);
			if (power >= 0) { 
				RecordInfo record;
				record.set_arg1(power);
				record.mutable_role()->set_name(ally);
				// DataManager_SaveSingleRecord(-13, &info);

				int map = -13; 
				SNPRINTF1(sql, "delete from %s where mapID = %d and `name` = '%s'", Config_SingleTable(), map, record.role().name().c_str());
				if (mysql_real_query(conn, sql, strlen(sql)) != 0) { 
					DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(conn));
					continue;
				}    

				SNPRINTF1(sql, "insert into %s set mapID=%d, `name`= '%s', score=%lld", Config_SingleTable(), map, record.role().name().c_str(), (long long)record.arg1());
				if (mysql_real_query(conn, sql, strlen(sql)) != 0) { 
					DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(conn));
					continue;
				}    
			}
		}
	}

	return true;
}

MYSQL * SavingDB() {
	MYSQL *conn = mysql_init(NULL);
	if (conn == NULL) {
		DEBUG_LOGERROR("Failed to init mysql");
		exit(EXIT_FAILURE);
	}

	if (mysql_real_connect(conn, Config_DBHost(), Config_DBUser(), Config_DBPassword(), Config_DB(), Config_DBPort(), NULL, 0) == NULL) {
		DEBUG_LOGERROR("Failed to connSavingect to mysql server");
		exit(EXIT_FAILURE);
	}

	my_bool opt = true;
	mysql_options(conn, MYSQL_OPT_RECONNECT, &opt);
	return conn;
}

static void FlushLocalCache() {
	MYSQL *conn = SavingDB();

	static vector<string> vecFiles;
	vecFiles.clear();

	int count = DirUtil_AllFiles("./SavingRole", &vecFiles);
	static char path[CONFIG_FIXEDARRAY];
	if (count > 0) {
		for (size_t i = 0; i < vecFiles.size(); ++i) {
			PB_PlayerAtt data;

			SNPRINTF1(path, "%s", vecFiles[i].c_str());
			fstream in(path, ios_base::in | ios_base::binary);
			if (in.fail()) {
				DEBUG_LOGERROR("Failed to open %s", path);
				continue;
			}

			in.seekg(0, in.end);
			if (in.tellg() > 0) {
				in.seekg(0, in.beg);
				if (data.ParseFromIstream(&in)) {
					SaveRoleToDB(conn, &data, NULL, false);
				}
			}

			in.close();
			remove(path);
		}
	}

	mysql_close(conn);
}

static void * RunSavingRole(void *) {
	static vector<struct SavingRole> saving;
	static char path[CONFIG_FIXEDARRAY];
	while (true) {
		saving.clear();
		pthread_mutex_lock(&package.mutex);
		saving = package.saving;
		package.saving.clear();
		pthread_mutex_unlock(&package.mutex);
		if (!saving.empty()) {
			for (size_t i = 0; i < saving.size(); i++) {
				struct SavingRole *unit = &saving[i];
				if (SignalHandler_IsOver()) {
					if (unit->data->att().baseAtt().roleID() == CONFIG_EXIT_CODE) {
						FlushLocalCache();
						DEBUG_LOGRECORD("Exit over");
						exit(EXIT_SUCCESS);
					} else {
						MYSQL *conn = SavingDB();
						SaveRoleToDB(conn, unit->data, unit->info, false);
						mysql_close(conn);

						int64_t roleID = unit->data->att().baseAtt().roleID();
						SNPRINTF1(path, "./SavingRole/att-%lld", (long long)roleID);
						remove(path);
						DEBUG_LOG("WWWWWWWWWWWWWWWWW");
					}
				} else {
					if (unit->insert) {
						MYSQL *conn = SavingDB();
						SaveRoleToDB(conn, unit->data, unit->info, true);
						mysql_close(conn);
					}else {
						SNPRINTF1(path, "./SavingRole/att-%lld", (long long)unit->data->att().baseAtt().roleID());
						fstream out_att(path, ios_base::out | ios_base::trunc | ios_base::binary);
						if (out_att.fail()) {
							DEBUG_LOGRECORD("create %s fail", path);
							remove(path);
						} else {
							if (!unit->data->SerializeToOstream(&out_att)) {
								DEBUG_LOGRECORD("Failed to serialize data: %s", path);
								remove(path);
							}    
						}
					}
					if (unit->data != NULL) {
						delete unit->data;
						unit->data = NULL;
					}
					if (unit->info != NULL) {
						delete unit->info;
						unit->info = NULL;
					}
				}
			}
		}

		static int interval = Time_TimeStamp();
		if (Time_TimeStamp() - interval > Config_SaveToDBInterval()) {
			FlushLocalCache();
			interval = Time_TimeStamp();
		}

		usleep(1000 * 250);
	}
	return NULL;
}

static void GenAttSnapshot(const struct SnapshotData *data);
static void GenCostSnapshot();
static void GenRechargeSnapshot();
static void * GenSnapshot(void *);

void DataManager_Init() {
	if (Config_SnapshotDir() != NULL) {
		char temp[CONFIG_FIXEDARRAY];
		SNPRINTF1(temp, "%s/temp_snapshot", Config_SnapshotDir());
		int fd = open(temp, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		FILE *file = fdopen(fd, "a");
		if (file == NULL) {
			DEBUG_LOGERROR("Failed to create temp file in snapshot dir: %s", Config_SnapshotDir());
			exit(EXIT_FAILURE);
		}
		fclose(file);
		remove(temp);
	}

	{
		package.conn = mysql_init(NULL);
		if (package.conn == NULL) {
			DEBUG_LOGERROR("Failed to init mysql");
			exit(EXIT_FAILURE);
		}

		if (mysql_real_connect(package.conn, Config_DBHost(), Config_DBUser(), Config_DBPassword(), Config_DB(), Config_DBPort(), NULL, 0) == NULL) {
			DEBUG_LOGERROR("%s, %s, %s, %s, %d", Config_DBHost(), Config_DBUser(), Config_DBPassword(), Config_DB(), Config_DBPort());
			DEBUG_LOGERROR("Failed to connect to mysql server");
			exit(EXIT_FAILURE);
		}

		my_bool opt = true;
		mysql_options(package.conn, MYSQL_OPT_RECONNECT, &opt);

		package.res = NULL;
	}
	{
		package.connPublic = mysql_init(NULL);
		if (package.connPublic == NULL) {
			DEBUG_LOGERROR("Failed to init mysql");
			exit(EXIT_FAILURE);
		}

		if (mysql_real_connect(package.connPublic, Config_DBPublicHost(), Config_DBPublicUser(), Config_DBPublicPassword(), Config_DBPublic(), Config_DBPublicPort(), NULL, 0) == NULL) {
			DEBUG_LOGERROR("Failed to connect to mysql server");
			exit(EXIT_FAILURE);
		}

		my_bool optPublic = true;
		mysql_options(package.connPublic, MYSQL_OPT_RECONNECT, &optPublic);

		package.resPublic = NULL;
	}
	{
		package.connRecharge = mysql_init(NULL);
		if(package.connRecharge == NULL) {
			DEBUG_LOGERROR("Failed to init recharge mysql");
			exit(EXIT_FAILURE);
		}

		if (mysql_real_connect(package.connRecharge, Config_DBRechargeHost(), Config_DBRechargeUser(), Config_DBRechargePassword(), Config_DBRecharge(), Config_DBRechargePort(), NULL, 0) == NULL) {
			DEBUG_LOGERROR("Failed to connect to recharge mysql");
			DEBUG_LOGERROR("Failed to connect to mysql server host : %s, user %s, passwd %s, db %s, port %d, ", Config_DBRechargeHost(), Config_DBRechargeUser(), Config_DBRechargePassword(), Config_DBRecharge(), Config_DBRechargePort());
			exit(EXIT_FAILURE);
		}

		my_bool optRecharge = true;
		mysql_options(package.connRecharge, MYSQL_OPT_RECONNECT, &optRecharge);

		package.resRecharge = NULL;
	}

	package.roleCache.clear();
	package.nameToID.clear();

	package.saving.clear();
	int res = pthread_mutex_init(&package.mutex, NULL);
	assert(res == 0);
	package.over = false;
	if (pthread_create(&package.savingThread, NULL, &RunSavingRole, NULL) != 0) {
		DEBUG_LOGERROR("Failed to create thread");
		exit(EXIT_FAILURE);
	}

	DirUtil_CreateDir("./SavingRole");
	FlushLocalCache();

	// Gen Snapshot
	/*
	   GenSnapshot(NULL);
	   DEBUG_LOGRECORD("gen snapshot over");
	   exit(0);
	   */
}

static const char * Yesterday() {
	static char ret[32];
	time_t c = (time_t)Time_TimeStamp();
	c -= 3600 * 24;
	tm cur;
	Time_LocalTime(c, &cur);
	int y = cur.tm_year + 1900;
	int m = cur.tm_mon + 1;
	int d = cur.tm_mday;
	SNPRINTF1(ret, "%04d-%02d-%02d 00:00:00", y, m, d);
	return ret;
}
static const char * Today() {
	static char ret[32];
	time_t c = (time_t)Time_TimeStamp();
	tm cur;
	Time_LocalTime(c, &cur);
	int y = cur.tm_year + 1900;
	int m = cur.tm_mon + 1;
	int d = cur.tm_mday;
	SNPRINTF1(ret, "%04d-%02d-%02d 00:00:00", y, m, d);
	return ret;
}

static void GenAttSnapshot(const struct SnapshotData *data) {
#define RETURN {if (file != NULL) {Config_UnlockFile(file);fclose(file);} if (res1 != NULL) mysql_free_result(res1); if (res2 != NULL) mysql_free_result(res2); if (conn1 != NULL) mysql_close(conn1); if (conn2 != NULL) mysql_close(conn2); return;}
	MYSQL *conn1 = NULL, *conn2 = NULL;
	MYSQL_RES *res1 = NULL, *res2 = NULL;
	FILE *file = NULL;

	if (Config_SnapshotDir() == NULL)
		RETURN;

	time_t cur = Time_TimeStamp();
	tm curTM;
	Time_LocalTime(cur, &curTM);
	char snapshot[128];
	SNPRINTF1(snapshot, "%s/TIANSHA_%s_GAMEINFO_%04d-%02d-%02d", Config_SnapshotDir(), Config_ScribeCategory(), curTM.tm_year + 1900, curTM.tm_mon + 1, curTM.tm_mday);
	// int fd = open(snapshot, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	// file = fdopen(fd, "a");
	file = fopen(snapshot, "a");
	if (file == NULL) {
		DEBUG_LOGERROR("Failed to create snapshot file: %s", snapshot);
		RETURN;
	}
	Config_LockFile(file);
	fseek(file, 0, SEEK_END);

	{
		conn1 = mysql_init(NULL);
		if (conn1 == NULL) {
			DEBUG_LOGERROR("Failed to init mysql");
			RETURN;
		}
		if (mysql_real_connect(conn1, data->host.c_str(), data->user.c_str(), data->passwd.c_str(), data->db.c_str(), data->port, NULL, 0) == NULL) {
			DEBUG_LOGERROR("Failed to connect to mysql server");
			RETURN;
		}
		my_bool opt = true;
		mysql_options(conn1, MYSQL_OPT_RECONNECT, &opt);
	}
	{
		conn2 = mysql_init(NULL);
		if (conn2 == NULL) {
			DEBUG_LOGERROR("Failed to init mysql");
			RETURN;
		}
		if (mysql_real_connect(conn2, data->host.c_str(), data->user.c_str(), data->passwd.c_str(), data->db.c_str(), data->port, NULL, 0) == NULL) {
			DEBUG_LOGERROR("Failed to connect to mysql server");
			RETURN;
		}
		my_bool opt = true;
		mysql_options(conn2, MYSQL_OPT_RECONNECT, &opt);
	}

	char sql1[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql1, "select account, device_id, device_addtime, idfa, add_time, first_login_ip, last_login_time, last_login_ip, total_num, login_num, osversion, phonetype, imei, platform, line, roles, activateKey from %s where add_time < '%s'", Config_AccountTable(), Today());
	if (mysql_real_query(conn1, sql1, strlen(sql1)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql1, mysql_error(conn1));
		RETURN;
	}

	res1 = mysql_use_result(conn1);
	if (res1 == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(conn1));
		RETURN;
	}

	bool allPlatform = strcmp(Config_ScribeCategory(), "ALL") == 0;

	char format[CONFIG_FIXEDARRAY] = {'\0'};;
	if (allPlatform) {
		int total = 25;
		for (int i = 0; i < total; i++) {
			if (i == 0)
				strcat(format, "%s");
			else if (i == total - 2)
				strcat(format, "	server%s");
			else
				strcat(format, "	%s");
		}
	} else {
		int total = 24;
		for (int i = 0; i < total; i++) {
			if (i == 0)
				strcat(format, "%s");
			else if (i == total - 1)
				strcat(format, "	server%s");
			else
				strcat(format, "	%s");
		}
	}
	strcat(format, "\n");

#define S(s) (((s) == NULL) ? "" : (s))
	for (MYSQL_ROW row1 = mysql_fetch_row(res1); row1 != NULL; row1 = mysql_fetch_row(res1)) {
		const char *roleID = NULL;
		const char *roleName = NULL;
		const char *createTime = NULL;
		const char *occupational = NULL;
		const char *level = NULL;
		const char *vipLevel = NULL;
		const char *exp = NULL;
		const char *goldingot= NULL;
		const char *friendsNum = NULL;

		char channel[128];
		const char *biChannel = Config_ChannelBIName(S(row1[13]));
		if (biChannel != NULL)
			strcpy(channel, biChannel);
		else
			strcpy(channel, S(row1[13]));
		/*
		   if (strcmp(channel, "rekoo") == 0) {
		   bool flag = Config_IsApplePhone(S(row1[11]));
		   if (flag) {
		   strcat(channel, "_ios");
		   } else {
		   strcat(channel, "_android");
		   }
		   }
		   */

		char roles[64];
		strcpy(roles, row1[15]);
		char *tokens[MAX_ROLE_PER_ACCOUNT];
		int count = ConfigUtil_ExtraToken(roles, tokens, MAX_ROLE_PER_ACCOUNT, ", "); 
		if (count <= 0) {
			static char buf[CONFIG_FIXEDARRAY * 10];
			int n;
			if (allPlatform) {
				n = snprintf(buf, sizeof(buf), format, S(row1[0]), S(row1[1]), S(row1[2]), S(row1[3]), S(row1[4]), S(row1[5]), S(row1[6]), S(row1[7]), S(row1[8]), S(row1[9]), S(roleID), S(roleName), S(occupational), S(createTime), S(level), S(vipLevel), S(exp), S(goldingot), S(friendsNum), S(row1[10]), S(row1[11]), S(row1[12]), S(channel), S(row1[14]), Config_ChannelPlatform(S(row1[13])));
			} else {
				n = snprintf(buf, sizeof(buf), format, S(row1[0]), S(row1[1]), S(row1[2]), S(row1[3]), S(row1[4]), S(row1[5]), S(row1[6]), S(row1[7]), S(row1[8]), S(row1[9]), S(roleID), S(roleName), S(occupational), S(createTime), S(level), S(vipLevel), S(exp), S(goldingot), S(friendsNum), S(row1[10]), S(row1[11]), S(row1[12]), S(channel), S(row1[14]));
			}
			if (n > 0 && n < (int)sizeof(buf))
				fputs(buf, file);
		} else {
			for (int i = 0; i < count; i++) {
				if (res2 != NULL) {
					mysql_free_result(res2);
					res2 = NULL;
				}
				char sql2[CONFIG_FIXEDARRAY];
				SNPRINTF1(sql2, "select name, occupational, create_time, level, vip_level, exp, goldingot, friends_num from %s where id=%s", Config_RoleTable(), tokens[i]);
				if (mysql_real_query(conn2, sql2, strlen(sql2)) != 0)
					continue;
				res2 = mysql_store_result(conn2);
				if (res2 == NULL)
					continue;
				MYSQL_ROW row2 = mysql_fetch_row(res2);
				if (row2 == NULL)
					continue;
				roleID = tokens[i];
				roleName = row2[0];
				occupational = row2[1];
				createTime = row2[2];
				level = row2[3];
				vipLevel = row2[4];
				exp = row2[5];
				goldingot = row2[6];
				friendsNum = row2[7];
				static char buf[CONFIG_FIXEDARRAY * 10];
				int n;
				if (allPlatform) {
					n = snprintf(buf, sizeof(buf), format, S(row1[0]), S(row1[1]), S(row1[2]), S(row1[3]), S(row1[4]), S(row1[5]), S(row1[6]), S(row1[7]), S(row1[8]), S(row1[9]), S(roleID), S(roleName), S(occupational), S(createTime), S(level), S(vipLevel), S(exp), S(goldingot), S(friendsNum), S(row1[10]), S(row1[11]), S(row1[12]), S(channel), S(row1[14]), Config_ChannelPlatform(S(row1[13])));
				} else {
					n = snprintf(buf, sizeof(buf), format, S(row1[0]), S(row1[1]), S(row1[2]), S(row1[3]), S(row1[4]), S(row1[5]), S(row1[6]), S(row1[7]), S(row1[8]), S(row1[9]), S(roleID), S(roleName), S(occupational), S(createTime), S(level), S(vipLevel), S(exp), S(goldingot), S(friendsNum), S(row1[10]), S(row1[11]), S(row1[12]), S(channel), S(row1[14]));
				}
				if (n > 0 && n < (int)sizeof(buf))
					fputs(buf, file);
			}
		}
	}
#undef S

	RETURN;
#undef RETURN
}

static void GenRechargeSnapshot() {
#define RETURN {if (file != NULL) {Config_UnlockFile(file);fclose(file);} if (res != NULL) mysql_free_result(res); if (conn != NULL) mysql_close(conn); return;}
	MYSQL *conn = NULL;
	MYSQL_RES *res = NULL;
	FILE *file = NULL;

	if (Config_SnapshotDir() == NULL)
		RETURN;

	conn = mysql_init(NULL);
	if (conn == NULL) {
		DEBUG_LOGERROR("Failed to init mysql");
		RETURN;
	}
	if (mysql_real_connect(conn, Config_DBRechargeHost(), Config_DBRechargeUser(), Config_DBRechargePassword(), Config_DBRecharge(), Config_DBRechargePort(), NULL, 0) == NULL) {
		DEBUG_LOGERROR("Failed to connect to mysql server");
		RETURN;
	}
	my_bool opt = true;
	mysql_options(conn, MYSQL_OPT_RECONNECT, &opt);

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select orderid,wtime,device_id,idfa,uid,role_id,money,payfrom,level,osversion,phonetype,imei,s_channel,line from %s where wtime>='%s' and wtime<'%s'", Config_RechargeTable(), Yesterday(), Today());
	if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(conn));
		RETURN;
	}

	res = mysql_use_result(conn);
	if (res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(conn));
		RETURN;
	}

	time_t cur = Time_TimeStamp();
	tm curTM;
	Time_LocalTime(cur, &curTM);
	char snapshot[128];
	SNPRINTF1(snapshot, "%s/TIANSHA_%s_PAY_%04d-%02d-%02d", Config_SnapshotDir(), Config_ScribeCategory(), curTM.tm_year + 1900, curTM.tm_mon + 1, curTM.tm_mday);
	// int fd = open(snapshot, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	// file = fdopen(fd, "a");
	file = fopen(snapshot, "a");
	if (file == NULL) {
		DEBUG_LOGERROR("Failed to create snapshot file: %s", snapshot);
		RETURN;
	}
	Config_LockFile(file);
	fseek(file, 0, SEEK_END);

	bool allPlatform = strcmp(Config_ScribeCategory(), "ALL") == 0;

	char format[CONFIG_FIXEDARRAY] = {'\0'};;
	if (allPlatform) {
		int total = 16;
		for (int i = 0; i < total; i++) {
			if (i == 0)
				strcat(format, "%s");
			else if (i == total - 2)
				strcat(format, "	server%s");
			else
				strcat(format, "	%s");
		}
	} else {
		int total = 15;
		for (int i = 0; i < total; i++) {
			if (i == 0)
				strcat(format, "%s");
			else if (i == total - 1)
				strcat(format, "	server%s");
			else
				strcat(format, "	%s");
		}
	}
	strcat(format, "\n");


#define S(s) (((s) == NULL) ? "" : (s))
	for (MYSQL_ROW row = mysql_fetch_row(res); row != NULL; row = mysql_fetch_row(res)) {
		int add_coin = atoi(row[6]) * Config_RMBToGem();
		char s_add_coin[32] = {'\0'};;
		if (add_coin >= 0)
			SNPRINTF1(s_add_coin, "%d", add_coin);

		const char *channel = NULL;
		if (row[12] != NULL && strcmp(row[12], "") != 0) { 
			channel = row[12];
		} else if (row[14] != NULL && strcmp(row[14], "") != 0) { 
			channel = Config_ChannelName(atoi(row[14]));
		}    

		const char *s_channel = NULL;
		if (channel != NULL)
			s_channel = Config_ChannelBIName(channel);

		const char *s_platform = "";
		if (channel != NULL)
			s_platform = Config_ChannelPlatform(channel);

		static char buf[CONFIG_FIXEDARRAY * 10];
		int n;
		if (allPlatform) {
			n = snprintf(buf, sizeof(buf), format,
					S(row[0]),
					S(row[1]),
					S(row[2]),
					S(row[3]),
					S(row[4]),
					S(row[5]),
					S(row[6]),
					s_add_coin,
					S(row[7]),
					S(row[8]),
					S(row[11]),
					S(row[0]),
					S(row[1]),
					S(s_channel),
					S(row[13]),
					S(s_platform)
					);
		} else {
			n = snprintf(buf, sizeof(buf), format,
					S(row[0]),
					S(row[1]),
					S(row[2]),
					S(row[3]),
					S(row[4]),
					S(row[5]),
					S(row[6]),
					s_add_coin,
					S(row[7]),
					S(row[8]),
					S(row[11]),
					S(row[0]),
					S(row[1]),
					S(s_channel),
					S(row[13])
					);
		}
		if (n > 0 && n < (int)sizeof(buf))
			fputs(buf, file);
	}
#undef S

	RETURN;
#undef RETURN
}

static void CastReason(const char *src, char *out) {
	int n = 0;
	for (int i = 0; src[i] != '\0'; i++, n++) {
		char c = src[i];
		if (c == ' ')
			out[n] = '_';
		else
			out[n] = src[i];
	}
	out[n] = '\0';
}

static void GenCostSnapshot() {
#define RETURN {if (file != NULL) {Config_UnlockFile(file);fclose(file);} if (res != NULL) mysql_free_result(res); if (conn != NULL) mysql_close(conn); return;}
	MYSQL *conn = NULL;
	MYSQL_RES *res = NULL;
	FILE *file = NULL;

	if (Config_SnapshotDir() == NULL)
		RETURN;

	conn = mysql_init(NULL);
	if (conn == NULL) {
		DEBUG_LOGERROR("Failed to init mysql");
		RETURN;
	}
	if (mysql_real_connect(conn, Config_DBPublicHost(), Config_DBPublicUser(), Config_DBPublicPassword(), Config_DBPublic(), Config_DBPublicPort(), NULL, 0) == NULL) {
		DEBUG_LOGERROR("Failed to connect to mysql server");
		RETURN;
	}
	my_bool opt = true;
	mysql_options(conn, MYSQL_OPT_RECONNECT, &opt);

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select uid,device_id,idfa,role,time,rmb_value,level,osversion,phonetype,imei,channel,line,reason,arg1,arg2,subrmb_value from %s where time>='%s' and time<'%s' and (rmb_value>0 or subrmb_value>0)", Config_CostRecordTable(), Yesterday(), Today());
	if (mysql_real_query(conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(conn));
		RETURN;
	}

	res = mysql_use_result(conn);
	if (res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(conn));
		RETURN;
	}

	time_t cur = Time_TimeStamp();
	tm curTM;
	Time_LocalTime(cur, &curTM);
	char snapshot[128];
	SNPRINTF1(snapshot, "%s/TIANSHA_%s_SUBINFO_%04d-%02d-%02d", Config_SnapshotDir(), Config_ScribeCategory(), curTM.tm_year + 1900, curTM.tm_mon + 1, curTM.tm_mday);
	// int fd = open(snapshot, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	// file = fdopen(fd, "a");
	file = fopen(snapshot, "a");
	if (file == NULL) {
		DEBUG_LOGERROR("Failed to create snapshot file: %s", snapshot);
		RETURN;
	}
	Config_LockFile(file);
	fseek(file, 0, SEEK_END);

	bool allPlatform = strcmp(Config_ScribeCategory(), "ALL") == 0;

	char format[CONFIG_FIXEDARRAY] = {'\0'};;
	if (allPlatform) {
		int total = 20;
		for (int i = 0; i < total; i++) {
			if (i == 0)
				strcat(format, "%s");
			else if (i == total - 3)
				strcat(format, "	server%s");
			else
				strcat(format, "	%s");
		}
	} else {
		int total = 19;
		for (int i = 0; i < total; i++) {
			if (i == 0)
				strcat(format, "%s");
			else if (i == total - 2)
				strcat(format, "	server%s");
			else
				strcat(format, "	%s");
		}
	}
	strcat(format, "\n");

#define S(s) (((s) == NULL) ? "" : (s))
	for (MYSQL_ROW row = mysql_fetch_row(res); row != NULL; row = mysql_fetch_row(res)) {
		char reason[64] = {'\0'};;
		char count[16] = {'\0'};
		char price[16] = {'\0'};
		const char *name = NULL;
		if (strcmp(row[12], "buy goods") == 0) {
			SNPRINTF1(reason, "bg_%s", row[13]);
			strcpy(count, row[14]);
			const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(atoi(row[13]));
			if (goods != NULL) {
				SNPRINTF1(price, "%d", (int)goods->rmb());
				name = goods->name().c_str();
			}
		} else {
			strcpy(reason, "xh_");
			CastReason(row[12], reason + 3);
			if (strcmp(row[12], "buy equip") == 0) {
				SNPRINTF2(reason, reason + strlen(reason), "_%s", row[13]);
				strcpy(count, row[14]);
				const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(atoi(row[13]));
				if (equip != NULL) {
					SNPRINTF1(price, "%d", (int)equip->rmb());
					name = equip->name().c_str();
				}
			} else if (strcmp(row[12], "buy wing") == 0) {
				SNPRINTF2(reason, reason + strlen(reason), "_%s", row[13]);
				strcpy(count, row[14]);
				const Wing *wing = EquipmentInfoManager_Wing(atoi(row[13]));
				if (wing != NULL) {
					SNPRINTF1(price, "%d", (int)wing->rmb());
					name = wing->name().c_str();
				}
			} else if (strcmp(row[12], "buy fashion") == 0) {
				SNPRINTF2(reason, reason + strlen(reason), "_%s", row[13]);
				strcpy(count, row[14]);
				const FashionInfo *fashion = FashionInfoManager_FashionInfo(atoi(row[13]));
				if (fashion != NULL) {
					SNPRINTF1(price, "%d", (int)fashion->rmb());
					name = fashion->name().c_str();
				}
			}
		}
		char channel[128];
		const char *biChannel = Config_ChannelBIName(S(row[10]));
		if (biChannel != NULL)
			strcpy(channel, biChannel);
		else
			strcpy(channel, S(row[10]));
		/*
		   if (strcmp(channel, "rekoo") == 0) {
		   bool flag = Config_IsApplePhone(S(row[8]));
		   if (flag) {
		   strcat(channel, "_ios");
		   } else {
		   strcat(channel, "_android");
		   }
		   }
		   */
		static char buf[CONFIG_FIXEDARRAY * 10];
		int n;
		if (allPlatform) {
			n = snprintf(buf, sizeof(buf), format,
					"",
					S(row[0]),
					S(row[1]),
					S(row[2]),
					S(row[3]),
					S(row[4]),
					S(row[5]),
					S(count),
					S(price),
					S(reason),
					"",
					S(name),
					S(row[6]),
					S(row[7]),
					S(row[8]),
					S(row[9]),
					S(channel),
					S(row[11]),
					Config_ChannelPlatform(S(row[10])),
					S(row[15])
					);
		} else {
			n = snprintf(buf, sizeof(buf), format,
					"",
					S(row[0]),
					S(row[1]),
					S(row[2]),
					S(row[3]),
					S(row[4]),
					S(row[5]),
					S(count),
					S(price),
					S(reason),
					"",
					S(name),
					S(row[6]),
					S(row[7]),
					S(row[8]),
					S(row[9]),
					S(channel),
					S(row[11]),
					S(row[15])
					);
		}
		if (n > 0 && n < (int)sizeof(buf))
			fputs(buf, file);
	}
#undef S

	RETURN;
#undef RETURN
}

static void * GenSnapshot(void *) {
	const vector<struct SnapshotData> *data = Config_Snapshots();
	for (size_t i = 0; i < data->size(); i++)
		GenAttSnapshot(&(*data)[i]);
	GenCostSnapshot();
	GenRechargeSnapshot();
	return NULL;
}

void DataManager_GenSnapshot() {
	if (Config_SnapshotDir() == NULL)
		return;
	if (Config_Line() != 1)
		return;
	Config_ReloadSnapshotData();

	pthread_t t;
	if (pthread_create(&t, NULL, &GenSnapshot, NULL) != 0) {
		DEBUG_LOGERROR("Failed to run GenSnapshot");
		return;
	}
}

static void FreeResult() {
	if (package.res != NULL) {
		mysql_free_result(package.res);
		package.res = NULL;
	}
	int ret = mysql_ping(package.conn);
	if (ret != 0)
		DEBUG_LOGERROR("Failed to mysql ping, ret: %d", ret);

	/*
	   if (package.conn != NULL) {
	   mysql_close(package.conn);
	   package.conn = NULL;
	   }

	   package.conn = mysql_init(NULL);
	   if (package.conn == NULL) {
	   DEBUG_LOGERROR("Failed to init mysql");
	   }

	   if (mysql_real_connect(package.conn, Config_DBHost(), Config_DBUser(), Config_DBPassword(), Config_DB(), Config_DBPort(), NULL, 0) == NULL) {
	   DEBUG_LOGERROR("Failed to connect to mysql server");
	   }

	// my_bool opt = true;
	// mysql_options(package.conn, MYSQL_OPT_RECONNECT, &opt);

	package.res = NULL;
	*/
}

static void FreeResultPublic() {
	if (package.resPublic != NULL) {
		mysql_free_result(package.resPublic);
		package.resPublic = NULL;
	}
	int ret = mysql_ping(package.connPublic);
	if (ret != 0)
		DEBUG_LOGERROR("Failed to mysql ping, ret: %d", ret);

	/*
	   if (package.connPublic != NULL) {
	   mysql_close(package.connPublic);
	   package.connPublic = NULL;
	   }

	   package.connPublic = mysql_init(NULL);
	   if (package.connPublic == NULL) {
	   DEBUG_LOGERROR("Failed to init mysql");
	   }

	   if (mysql_real_connect(package.connPublic, Config_DBPublicHost(), Config_DBPublicUser(), Config_DBPublicPassword(), Config_DBPublic(), Config_DBPublicPort(), NULL, 0) == NULL) {
	   DEBUG_LOGERROR("Failed to connect to mysql server");
	   }

	// my_bool optPublic = true;
	// mysql_options(package.connPublic, MYSQL_OPT_RECONNECT, &optPublic);

	package.resPublic = NULL;
	*/
}

static void FreeResultRecharge() {
	if(package.resRecharge != NULL) {
		mysql_free_result(package.resRecharge);
		package.resRecharge = NULL;
	}
	int ret = mysql_ping(package.connRecharge);
	if (ret != 0)
		DEBUG_LOGERROR("Failed to mysql ping, ret: %d", ret);

	/*
	   if (package.connRecharge != NULL) {
	   mysql_close(package.connRecharge);
	   package.connRecharge = NULL;
	   }

	   package.connRecharge = mysql_init(NULL);
	   if(package.connRecharge == NULL) {
	   DEBUG_LOGERROR("Failed to init recharge mysql");
	   }

	   if (mysql_real_connect(package.connRecharge, Config_DBRechargeHost(), Config_DBRechargeUser(), Config_DBRechargePassword(), Config_DBRecharge(), Config_DBRechargePort(), NULL, 0) == NULL) {
	   DEBUG_LOGERROR("Failed to connect to recharge mysql");
	   DEBUG_LOGERROR("Failed to connect to mysql server host : %s, user %s, passwd %s, db %s, port %d, ", Config_DBRechargeHost(), Config_DBRechargeUser(), Config_DBRechargePassword(), Config_DBRecharge(), Config_DBRechargePort());
	   }

	// my_bool optRecharge = true;
	// mysql_options(package.connRecharge, MYSQL_OPT_RECONNECT, &optRecharge);

	package.resRecharge = NULL;
	*/
}

/*
   static void FilterMissions() {
   PB_PlayerAtt att;
   for (int i = 0; i < 8000; i++) {
   if (DataManager_LoadRoleData(i, &att, NULL)) {
   PB_MissionAllRecord *records = att.mutable_missions();
   for (int k = 0; k <= 49; k++) {
   records->mutable_records(k)->set_count(1);
   for (int j = 0; j < records->records(k).target_size(); j++) 
   for (int n = 0; n < records->records(k).target(j).arg_size(); n++) 
   records->mutable_records(k)->mutable_target(j)->set_arg(n, 0);
   }    
   for (int j = 50; j <= 54; j++) 
 *records->mutable_records(j) = records->records(j + 33); 
 for (int j = 55; j <= 64; j++) 
 *records->mutable_records(j) = records->records(j + 44); 
 for (int k = 65; k <= 87; k++) {
 records->mutable_records(k)->set_count(0);
 for (int j = 0; j < records->records(k).target_size(); j++) 
 for (int n = 0; n < records->records(k).target(j).arg_size(); n++) 
 records->mutable_records(k)->mutable_target(j)->set_arg(n, 0);
 }    
 for (int k = 98; k < records->records_size(); k++) {
 records->mutable_records(k)->set_count(0);
 for (int j = 0; j < records->records(k).target_size(); j++) 
 for (int n = 0; n < records->records(k).target(j).arg_size(); n++) 
 records->mutable_records(k)->mutable_target(j)->set_arg(n, 0);
 }    

 for (int j = 0; j < records->cur_size(); j++) {
 int id = records->cur(j);
 if (id == -1)
 continue;
 if (id < 83 || id == 98) {
 records->set_cur(j, 50);
 break;
 } else if (id >= 83 && id <= 87) {
 records->set_cur(j, id - 33);
 break;
 } else if (id >= 99 && id <= 108) {
 records->set_cur(j, id - 44);
 break;
 }
 }

 for (int i = 0; i < att.passGuide_size(); i++)
 att.set_passGuide(i, true);

 DataManager_SaveRoleData(&att);
 }
 }
 DEBUG_LOGRECORD("DONE");
 }
 */

static const PB_PlayerAtt * RoleFromCache(int64_t id) {
	map<int64_t, string>::iterator it = package.roleCache.find(id);
	if (it == package.roleCache.end()) {
		return NULL;
	} else {
		static PB_PlayerAtt att;
		if (!att.ParseFromString(it->second))
			return NULL;
		else
			return &att;
	}
}

static const PB_PlayerAtt * RoleFromCache(const string &name) {
	map<string, int64_t>::iterator it = package.nameToID.find(name);
	if (it == package.nameToID.end()) {
		return NULL;
	} else {
		return RoleFromCache(it->second);
	}
}

static bool PushRoleToCache(const PB_PlayerAtt *data) {
	if (data == NULL)
		return false;

	package.nameToID[data->att().baseAtt().name()] = data->att().baseAtt().roleID();

	static string buf;
	buf.clear();
	if (data->SerializeToString(&buf)) {
		package.roleCache[data->att().baseAtt().roleID()] = buf;
		return true;
	} else {
		return false;
	}
}

bool DataManager_CollectRole(int64_t *cur, google::protobuf::RepeatedPtrField<RecordInfo> *singleRecord, google::protobuf::RepeatedPtrField<RecordInfo> *Restriction, RecordInfo* godInfoTime, RecordInfo* winFactionInfo, google::protobuf::RepeatedPtrField<RecordInfo>* factionInfo) {
	if (cur == NULL || singleRecord == NULL || Restriction == NULL || godInfoTime == NULL || winFactionInfo == NULL || factionInfo == NULL)  
		return false;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select id,data from %s", Config_RoleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	// package.res = mysql_use_result(package.conn);
	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	DEBUG_LOGRECORD("begin load role");
	*cur = 0;
	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		int64_t id = atoll(row[0]);
		if (id >= *cur)
			*cur = id + 1;

		PB_PlayerAtt att;
		unsigned long *lengths = mysql_fetch_lengths(package.res);
		if (!att.ParseFromArray(row[1], lengths[1])) {
			DEBUG_LOGERROR("Failed to load role data from db, id: %lld", (long long)id);
			exit(EXIT_FAILURE);
		}
		PushRoleToCache(&att);
	}
	DEBUG_LOGRECORD("end load role");

	SNPRINTF1(sql, "select mapID,roleID,name,professionType,score from %s where mapID >= 0 and mapID < 10000", Config_SingleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	RecordInfo record;
	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		int32_t mapID = atoll(row[0]);
		for (int i = singleRecord->size(); i <= mapID; i++)
			singleRecord->Add()->mutable_role()->set_roleID(-1);

		record.mutable_role()->set_roleID((int64_t)atoll(row[1]));
		record.mutable_role()->set_name((const char *)row[2]);
		record.mutable_role()->set_professionType((PB_ProfessionInfo::Type)atoi(row[3]));
		record.set_arg1((int64_t)atoll(row[4]));

		*singleRecord->Mutable(mapID) = record;
	}

	SNPRINTF1(sql, "select mapID,roleID,name,professionType,score from %s where mapID >= 10400 and mapID < 10700", Config_SingleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		record.mutable_role()->set_roleID((int64_t)atoll(row[1]));
		record.mutable_role()->set_name((const char *)row[2]);
		record.mutable_role()->set_professionType((PB_ProfessionInfo::Type)atoi(row[3]));
		record.set_arg1((int64_t)atoll(row[4]));
		*Restriction->Add() = record;		
	}


	SNPRINTF1(sql, "select score from %s where mapID = -10", Config_SingleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		godInfoTime->set_arg1((int64_t)atoll(row[0]));
		break;
	}

	SNPRINTF1(sql, "select roleID, `name`, score from %s where mapID = -11", Config_SingleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		winFactionInfo->mutable_role()->set_roleID((int64_t)atoll(row[0]));
		winFactionInfo->mutable_role()->set_name((const char *)row[1]);
		winFactionInfo->set_arg1((int64_t)atoll(row[2]));
	}

	SNPRINTF1(sql, "select roleID, `name`, score from %s where mapID = -12", Config_SingleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		record.mutable_role()->set_roleID((int64_t)atoll(row[0]));
		record.mutable_role()->set_name((const char *)row[1]);
		record.set_arg1((int64_t)atoll(row[2]));
		*factionInfo->Add() = record;		
	}

	// FilterMissions();
	// exit(0);

	return true;
}

static bool CheckAccountPassword(const char *platform, const char *account, const char *password) {
	/*
	   static const char *platforms[] = {LOCAL_PLATFORM, "tongbu"};
	   static size_t size = sizeof(platforms) / sizeof(platforms[0]);
	   bool has = false;
	   for (size_t i = 0; i < size; i++) {
	   if (strcmp(platform, platforms[i]) == 0) {
	   has = true;
	   break;
	   }
	   }
	   if (!has)
	   return false;
	   */
	if (strcmp(platform, LOCAL_PLATFORM) == 0) {
		size_t len = strlen(account);
		for (size_t i = 0; i < len; i++) {
			char c = account[i];
			if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9'))
				return false;
		}
	}
	return true;
}

int DataManager_AddAccount(const PlayerInfo *info, const char *ip, const char *activateKey, char *addTime, char *deviceAddTime, bool beyond) {
	if (info == NULL)
		return -1;

	const char *platform = info->platform().c_str();
	const char *account = info->account().c_str();
	const char *password = info->password().c_str();

	if (!CheckAccountPassword(platform, account, password))
		return -1;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select add_time, UNIX_TIMESTAMP(last_login_time), total_num, login_num from %s where platform='%s' and account='%s'", Config_AccountTable(), platform, account);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return -1;
	}
	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to store result, error: %s", mysql_error(package.conn));
		return -1;
	}
	if (mysql_num_rows(package.res) > 0) {
		if (addTime != NULL) {
			MYSQL_ROW row = mysql_fetch_row(package.res);
			if (row == NULL) {
				DEBUG_LOGERROR("Failed to fetch row, error: %s", mysql_error(package.conn));
				return -1;
			}
			strcpy(addTime, row[0] == NULL ? "" : row[0]);
		}
		return 1;
	}

	char dat[32] = {'\0'};;
	if (!info->deviceID().empty()) {
		SNPRINTF1(sql, "select registtime from %s where device='%s'", Config_DeviceTable(), info->deviceID().c_str());
		FreeResultPublic();
		if (mysql_real_query(package.connPublic, sql, strlen(sql)) == 0) {
			package.resPublic = mysql_store_result(package.connPublic);
			if (package.resPublic != NULL) {
				if (mysql_num_rows(package.resPublic) > 0) {
					MYSQL_ROW row = mysql_fetch_row(package.resPublic);
					if (row[0] != NULL)
						strcpy(dat, row[0]);
				}
			}
		}
	}
	if (dat[0] == '\0')
		Time_ToDateTime((int)Time_TimeStamp(), dat);
	if (deviceAddTime != NULL)
		strcpy(deviceAddTime, dat);

	char cur[32];
	Time_ToDateTime((int)Time_TimeStamp(), cur);

	if (!beyond) {
#define S(s) (((s) == NULL) ? "" : (s))
		SNPRINTF1(sql, "insert into %s (platform, account, password, roles, add_time, device_id, device_addtime, idfa, first_login_ip, last_login_time, last_login_ip, total_num, login_num, osversion, phonetype, imei, line, activateKey) values('%s', '%s', '%s', '', '%s', '%s', '%s', '%s', '%s', '%s', '%s', 0, 0, '%s', '%s', '%s', %d, '%s')", Config_AccountTable(), platform, account, password, cur, info->deviceID().c_str(), dat, info->idfa().c_str(), ip, cur, ip, info->osversion().c_str(), info->phonetype().c_str(), info->imei().c_str(), Config_Line(), S(activateKey));
#undef S
		FreeResult();
		if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			return -1;
		}
	}

	if (addTime != NULL) {
		strcpy(addTime, cur);
	}

	return 0;
}

bool DataManager_HasAccount(const char *platform, const char *account) {
	if (platform == NULL || account == NULL)
		return false;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select account from %s where platform='%s' and account='%s'", Config_AccountTable(), platform, account);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return -1;
	}
	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to store result, error: %s", mysql_error(package.conn));
		return -1;
	}
	return mysql_num_rows(package.res) > 0;
}

static bool CheckKey(const char *key) {
	if (strlen(key) > 100)
		return false;
	for (const char *c = key; *c != '\0'; c++) {
		if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9')))
			return false;
	}
	return true;
}

bool DataManager_ActiveKey(const char *key) {
	if (key == NULL)
		return false;
	if (!CheckKey(key))
		return false;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "update %s set valid=0 where `key`='%s' and valid=1", Config_ActivateKeyTable(), key);
	FreeResultPublic();
	if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.connPublic));
		return false;
	}
	return mysql_affected_rows(package.connPublic) > 0;
}

void DataManager_UpdateAccount(const PlayerInfo *info, const char *ip) {
	if (info == NULL)
		return;

	const char *platform = info->platform().c_str();
	const char *account = info->account().c_str();
	const char *password = info->password().c_str();
	if (!CheckAccountPassword(platform, account, password))
		return;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select UNIX_TIMESTAMP(last_login_time), total_num, login_num from %s where platform='%s' and account='%s'", Config_AccountTable(), platform, account);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return;
	}
	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to store result, error: %s", mysql_error(package.conn));
		return;
	}
	if (mysql_num_rows(package.res) <= 0)
		return;

	MYSQL_ROW row = mysql_fetch_row(package.res);
	if (row == NULL) {
		DEBUG_LOGERROR("Failed to fetch row, error: %s", mysql_error(package.conn));
		return;
	}

	int lastLoginTime = 0;
	if (row[0] != NULL)
		lastLoginTime = atoi(row[0]);
	int totalNum = 0;
	if (row[1] != NULL)
		totalNum = atoi(row[1]);
	int loginNum = 0;
	if (row[2] != NULL)
		loginNum = atoi(row[2]);

	int cur = (int)Time_TimeStamp();
	int dayDelta = Time_DayDelta(lastLoginTime, cur);
	if (dayDelta > 0) {
		totalNum++;
		if (dayDelta == 1)
			loginNum++;
		else
			loginNum = 0;
	}

	SNPRINTF1(sql, "update %s set last_login_time=FROM_UNIXTIME(%d), last_login_ip='%s', total_num=%d, login_num=%d  where platform='%s' and account='%s'", Config_AccountTable(), cur, ip, totalNum, loginNum, info->platform().c_str(), info->account().c_str());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return;
	}
}

static void AddRoleToAccount(const char *platform, const char *account, int64_t id) {
	int64_t ids[CONFIG_FIXEDARRAY];
	int count = DataManager_RoleIDs(platform, account, ids, CONFIG_FIXEDARRAY);
	for (int i = 0; i < count; i++) {
		if (ids[i] == id)
			return;
	}

	char value[CONFIG_FIXEDARRAY];
	if (count <= 0) {
		SNPRINTF1(value, "%lld", (long long)id);
	} else {
		SNPRINTF1(value, "%lld", (long long)ids[0]);
		for (int i = 1; i < count; i++)
			SNPRINTF2(value, value + strlen(value), ",%lld", (long long)ids[i]);
		SNPRINTF2(value, value + strlen(value), ",%lld", (long long)id);
	}

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "update %s set roles='%s' where platform='%s' and account='%s'", Config_AccountTable(), value, platform, account);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return;
	}

	my_ulonglong res = mysql_affected_rows(package.conn);
	if (res == (my_ulonglong)-1) {
		DEBUG_LOGERROR("Failed to update, error: %s", mysql_error(package.conn));
		return;
	} else if (res != 1) {
		DEBUG_LOGERROR("Failed to update");
		return;
	}
}

static bool SaveRoleData(const PB_PlayerAtt *data, const PlayerInfo *info, bool insert) {
	PushRoleToCache(data);

	struct SavingRole sr;
	sr.data = new PB_PlayerAtt(*data);
	if (info != NULL) {
		sr.info = new PlayerInfo(*info);
	} else {
		sr.info = NULL;
	}
	sr.insert = insert;

	pthread_mutex_lock(&package.mutex);
	package.saving.push_back(sr);
	pthread_mutex_unlock(&package.mutex);

	return true;
}

void DataManager_AddRole(const PB_PlayerAtt *data, const PlayerInfo *info) {
	if (info == NULL || data == NULL)
		return;

	if (!SaveRoleData(data, info, true))
		return;
	int64_t id = data->att().baseAtt().roleID();
	AddRoleToAccount(info->platform().c_str(), info->account().c_str(), id);
}

int DataManager_RoleIDs(const char *platform, const char *account, int64_t *ids, size_t size) {
	if (platform == NULL || account == NULL || ids == NULL)
		return -1;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select roles from %s where platform='%s' and account='%s'", Config_AccountTable(), platform, account);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return -1;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to store result, error: %s", mysql_error(package.conn));
		return -1;
	}

	if (mysql_num_rows(package.res) != 1 || mysql_num_fields(package.res) != 1) {
		DEBUG_LOGERROR("Failed to find account: %s", account);
		return -1;
	}

	MYSQL_ROW row = mysql_fetch_row(package.res);
	if (row == NULL) {
		DEBUG_LOGERROR("Failed to fetch row, error: %s", mysql_error(package.conn));
		return -1;
	}

	char line[CONFIG_FIXEDARRAY];
	strcpy(line, row[0]);

	char *tokens[CONFIG_FIXEDARRAY];
	int total = ConfigUtil_ExtraToken(line, tokens, CONFIG_FIXEDARRAY, ",");
	int count = 0;
	for (; count < total; count++)
		ids[count] = atoll(tokens[count]);

	return count;
}

void DataManager_RandomRoleID(int max, std::vector<int64_t> *ids) {
	if (ids == NULL)
		return;

	int size = (int)package.roleCache.size();
	if (size <= 0)
		return;

	int index = Time_Random(0, size); 
	map<int64_t, string>::iterator begin = package.roleCache.begin();
	for (int i = 0; i < index; i++) 
		begin++;

	int total = 0;

	for (map<int64_t, string>::iterator cur = begin; cur != package.roleCache.end(); cur++) {
		if (total++ >= max)
			return;
		ids->push_back(cur->first);
	}

	for (map<int64_t, string>::iterator cur = package.roleCache.begin(); cur != begin; cur++) {
		if (total++ >= max)
			return;
		ids->push_back(cur->first);
	}
}

static bool LoadRoleFromDB(int64_t id, const char *name, PB_PlayerAtt *data) {
	char sql[CONFIG_FIXEDARRAY];
	if (id >= 0 && name == NULL) {
		SNPRINTF1(sql, "select data from %s where id=%lld", Config_RoleTable(), (long long)id);
	} else if (id < 0 && name != NULL) {
		SNPRINTF1(sql, "select data from %s where name='%s'", Config_RoleTable(), name);
	} else {
		return false;
	}
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	if (mysql_num_rows(package.res) != 1 || mysql_num_fields(package.res) != 1) {
		// DEBUG_LOGERROR("Failed to find role: %lld", (long long)id);
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row(package.res);
	if (row == NULL) {
		DEBUG_LOGERROR("Failed to fetch row, error: %s", mysql_error(package.conn));
		return false;
	}

	unsigned long *lengths = mysql_fetch_lengths(package.res);
	if (!data->ParseFromArray(row[0], lengths[0])) {
		DEBUG_LOGERROR("Failed to parse role");
		return false;
	}

	return true;
}

static bool LoadRoleFromLocal(int64_t id, const char* name, PB_PlayerAtt* data) {
	if (id == -1) {
		if (name == NULL) {
			return false;
		}else {
			map<string, int64_t>::iterator it = package.nameToID.find(name);
			if (it == package.nameToID.end()) {
				return false;
			}else {
				id = it->second;
			}
		}
	}

	static char path[CONFIG_FIXEDARRAY];
	SNPRINTF1(path, "./SavingRole/att-%lld", (long long)id);
	fstream in(path, ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOG("Failed to open %s", path);
		return false;
	}

	if (!data->ParseFromIstream(&in)) {
		return false;
	}

	in.close();
	return true;
}

bool DataManager_LoadRoleData(int64_t id, PB_PlayerAtt *data) {
	if (id < 0 || data == NULL)
		return false;

	const PB_PlayerAtt *ret = RoleFromCache(id);
	if (ret != NULL) {
		*data = *ret;
	} else if (!LoadRoleFromLocal(id, NULL, data)) {
		if (!LoadRoleFromDB(id, NULL, data)) {
			return false;
		}
	}
	PlayerEntity_FilterData(data);
	return true;
}

bool DataManager_LoadRoleData(const string &name, PB_PlayerAtt *data) {
	if (name.empty() || data == NULL)
		return false;

	const PB_PlayerAtt *ret = RoleFromCache(name);
	if (ret != NULL) {
		*data = *ret;
	} else if (!LoadRoleFromLocal(-1, name.c_str(), data)) {
		if (!LoadRoleFromDB(-1, name.c_str(), data)) {
			return false;
		}
	}
	PlayerEntity_FilterData(data);
	return true;
}

void DataManager_FilterRecharge(DCProto_FilterRecharge *proto) {
	if (proto == NULL)
		return;

	const char *account = proto->info().account().c_str();
	const char *platform = proto->info().platform().c_str();
	int64_t roleID = proto->roleID();

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select orderid from %s where channel=%d and uid='%s' and state=0 and line=%d and role_id=%lld", Config_RechargeTable(), Config_ChannelRechargeNum(platform), account, Config_Line(), (long long)roleID);
	FreeResultRecharge();
	if (mysql_real_query(package.connRecharge, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.connRecharge));
		return;
	}
	package.resRecharge = mysql_store_result(package.connRecharge);
	if (package.resRecharge == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.connRecharge));
		return;
	}
	for (MYSQL_ROW row = mysql_fetch_row(package.resRecharge); row != NULL; row = mysql_fetch_row(package.resRecharge)) {
		if (row[0] != NULL && row[0] != '\0') {
			NetProto_Recharge *unit = proto->add_recharge();
			unit->set_order(row[0]);
		}
	}
}

bool DataManager_SaveRoleData(const PB_PlayerAtt *data, const PlayerInfo *info) {
	if (data == NULL)
		return false;
	return SaveRoleData(data, info, false);
}

int64_t DataManager_DelRoleData(const char *platform, const char *account, int64_t id, const google::protobuf::RepeatedField<int64_t> *equipments) {
	/*
	   if (platform == NULL || account == NULL || id < 0 || equipments == NULL)
	   return -1;

	   char sql[CONFIG_FIXEDARRAY];
	   SNPRINTF1(sql, "delete from %s where id=%lld", Config_RoleTable(), (long long)id);
	   FreeResult();
	   if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
	   DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
	   } else {
	   my_ulonglong res = mysql_affected_rows(package.conn);
	   if (res == (my_ulonglong)-1) {
	   DEBUG_LOGERROR("Failed to delete, error: %s", mysql_error(package.conn));
	   } else if (res != 1) {
	   DEBUG_LOGERROR("Failed to delete");
	   }
	   }

	   int64_t ids[CONFIG_FIXEDARRAY];
	   int count = DataManager_RoleIDs(platform, account, ids, CONFIG_FIXEDARRAY);
	   char value[CONFIG_FIXEDARRAY];
	   value[0] = '\0';
	   for (int i = 0, n = 0; i < count; i++) {
	   if (ids[i] != id) {
	   if ((n++) == 0)
	   SNPRINTF1(value, "%lld", (long long)ids[i]);
	   else
	   SNPRINTF1(value + strlen(value), ",%lld", (long long)ids[i]);
	   }
	   }

	   SNPRINTF1(sql, "update %s set roles='%s' where platform='%s' and account='%s'", Config_AccountTable(), value, platform, account);
	   FreeResult();
	   if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
	   DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
	   return -1;
	   }

	   my_ulonglong res = mysql_affected_rows(package.conn);
	   if (res == (my_ulonglong)-1) {
	   DEBUG_LOGERROR("Failed to update, error: %s", mysql_error(package.conn));
	   return -1;
	   } else if (res != 1) {
	   DEBUG_LOGERROR("Failed to update");
	   return -1;
	   }
	   */

	return id;
}

void DataManager_SaveSingleRecord(int32_t map, const RecordInfo *record) {
	char sql[CONFIG_FIXEDARRAY];
	if (record == NULL)
		return;
	if (map < 0) {
		if (map == -15) {
			string name = "";
			int rmb = 0;
			bool flag = true;
			for (int i = 0; i < (int)record->role().name().length(); ++i) {
				if (record->role().name().at(i) == ',') {
					flag = false;
					continue;
				}

				if (flag) {
					name += record->role().name().at(i);
				}else {
					rmb = 10 * rmb + record->role().name().at(i) - '0';
				}
			}

			SNPRINTF1(sql, "delete from %s where mapID = %d and  roleID = %lld", Config_SingleTable(), map, (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}

			SNPRINTF1(sql, "insert into %s set mapID=%d,roleID=%lld,name='%s',professionType=%d,score=%lld", Config_SingleTable(), map, (long long)record->role().roleID(), name.c_str(), (int)record->role().professionType(), (long long)rmb);
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}else if (map == -14) {
			SNPRINTF1(sql, "insert into %s set mapID=%d,roleID=%lld,name='%s',professionType=%d,score=%lld", Config_SingleTable(), map, (long long)record->role().roleID(), record->role().name().c_str(), (int)record->role().professionType(), (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}else if (map == -10) {
			SNPRINTF1(sql, "delete from %s where mapID = %d", Config_SingleTable(), map);
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}

			SNPRINTF1(sql, "insert into %s set mapID=%d,score=%lld", Config_SingleTable(), map, (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}else if (map == -11) {
			SNPRINTF1(sql, "delete from %s where mapID = %d", Config_SingleTable(), map);
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}

			SNPRINTF1(sql, "insert into %s set mapID=%d, roleID=%lld, `name`= '%s', score=%lld", Config_SingleTable(), map, (long long)record->role().roleID(), record->role().name().c_str(), (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}if (map == -12) {
			SNPRINTF1(sql, "delete from %s where mapID = %d and `name` = '%s' ", Config_SingleTable(), map, record->role().name().c_str());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}

			SNPRINTF1(sql, "insert into %s set mapID=%d, roleID=%lld, `name`= '%s', score=%lld", Config_SingleTable(), map, (long long)record->role().roleID(), record->role().name().c_str(), (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}if (map == -13) {
			SNPRINTF1(sql, "delete from %s where mapID = %d and `name` = '%s'", Config_SingleTable(), map, record->role().name().c_str());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}	

			SNPRINTF1(sql, "insert into %s set mapID=%d, `name`= '%s', score=%lld", Config_SingleTable(), map, record->role().name().c_str(), (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}

			/*
			   int begin = record->role().roleID() >> 32;
			   int end = record->role().roleID() & 0xFFFFFFFF;
			   if (begin == end) {
			   SNPRINTF1(sql, "delete from %s where mapID = %d and roleID = %lld ", Config_SingleTable(), map, (long long)begin);
			   FreeResult();
			   if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			   DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			   return;
			   }	

			   SNPRINTF1(sql, "insert into %s set mapID=%d, roleID=%lld, `name`= '%s', score=%lld", Config_SingleTable(), map, (long long)begin, record->role().name().c_str(), (long long)record->arg1());
			   FreeResult();
			   if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			   DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			   return;
			   }
			   }else {
			   SNPRINTF1(sql, "delete from %s where mapID = %d and roleID = %lld", Config_SingleTable(), map, (long long)end);
			   FreeResult();
			   if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			   DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			   return;
			   }	
			   while (end > begin) {
			   SNPRINTF1(sql, "update %s set roleID = %lld where mapID = %d and roleID = %lld ", Config_SingleTable(), (long long)end,  map, (long long)end - 1);
			   FreeResult();
			   if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			   DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			   return;
			   }
			   --end;	
			   }

			   SNPRINTF1(sql, "insert into %s set mapID=%d, roleID=%lld, `name`= '%s', score=%lld", Config_SingleTable(), map, (long long)begin, record->role().name().c_str(), (long long)record->arg1());
			   FreeResult();
			   if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			   DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			   return;
			   }	
			   }
			   */
		}else if (map == -20) {
			SNPRINTF1(sql, "insert into %s set mapID=%d, roleID = %lld, `name` = '%s' ,score = %lld", Config_SingleTable(), map, (long long)record->role().roleID(), record->role().name().c_str(), (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}else if (map == -21 || map == -22) {
			SNPRINTF1(sql, "delete from %s where mapID = %d ", Config_SingleTable(), map);
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}	

			SNPRINTF1(sql, "insert into %s set mapID=%d, roleID = %lld, score = %lld", Config_SingleTable(), map, (long long)record->role().roleID(), (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}else if (map == -24) {
			SNPRINTF1(sql, "delete from %s where mapID = %d and roleID = %lld", Config_SingleTable(), map, (long long)record->role().roleID());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}	
			SNPRINTF1(sql, "insert into %s set mapID=%d, roleID = %lld, yuezhan = '%s', score = %lld", Config_SingleTable(), map, (long long)record->role().roleID(), record->role().name().c_str(), (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}else if (map == -100) {
			SNPRINTF1(sql, "insert into %s set mapID=%d, roleID = %lld, score = %lld", Config_SingleTable(), map, (long long)record->role().roleID(),  (long long)record->arg1());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				return;
			}
		}
		return;
	}

	if (record->role().roleID() != -1) {
		SNPRINTF1(sql, "delete from %s where mapID = %d", Config_SingleTable(), map);
		FreeResult();
		if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			return;
		}

		SNPRINTF1(sql, "insert into %s set mapID=%d,roleID=%lld,name='%s',professionType=%d,score=%lld", Config_SingleTable(), map, (long long)record->role().roleID(), record->role().name().c_str(), (int)record->role().professionType(), (long long)record->arg1());
	}
	else {
		SNPRINTF1(sql, "delete from %s where mapID=%d", Config_SingleTable(), map);
	}

	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return;
	}
}

bool DataManager_HasName(const char *name) {
	if (name == NULL)
		return true;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select id from %s where name='%s'", Config_RoleTable(), name);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return true;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to store result, error: %s", mysql_error(package.conn));
		return true;
	}

	bool has = mysql_num_rows(package.res) != 0;
	return has;
}

int DataManager_AddMail(PB_PlayerAtt *att, PB_MailInfo *mail) {
	if (att == NULL || mail == NULL)
		return -1;

	int equipID = -1;
	if (mail->item().type() == PB_ItemInfo::EQUIPMENT) {
		equipID = Item_AddEquip(att->mutable_itemPackage(), EquipmentInfoManager_EquipmentInfo(mail->item().id()));
		if (equipID == -1)
			return -1;
	}

	mail->set_time((int64_t)Time_TimeStamp());
	mail->set_read(false);
	if (equipID != -1)
		mail->mutable_item()->set_id(equipID);

	int pos = PlayerEntity_FindEmptyMail(att);
	if (pos == -1)
		return -1;
	PB_MailInfo *info = att->mutable_mails(pos);
	if (info->time() != 0) {
		if (info->item().type() == PB_ItemInfo::EQUIPMENT)
			Item_DelEquip(att->mutable_itemPackage(), info->item().id());
	}
	*info = *mail;

	if (!DataManager_SaveRoleData(att))
		return -1;
	return pos;
}

int DataManager_AddMail(int64_t roleID, PB_MailInfo *mail) {
	if (mail == NULL)
		return -1;

	static PB_PlayerAtt att;
	att.Clear();
	if (!DataManager_LoadRoleData(roleID, &att))
		return -1;

	return DataManager_AddMail(&att, mail);
}

int DataManager_AddMail(std::set<int64_t>& roleIDs, const PB_MailInfo *mail) {
	if (mail == NULL)
		return -1;

	for (map<int64_t, string>::iterator it = package.roleCache.begin(); it != package.roleCache.end(); ++it) {
		PB_MailInfo mailInfo = *mail;
		if (roleIDs.find(it->first) == roleIDs.end()) {
			DataManager_AddMail(it->first, &mailInfo);
		}
	}
	return 0;
}

int32_t DataManager_GetKeyGift(const char *key, int32_t *event, size_t size, bool done, int64_t roleID, int32_t &groupIndex) {
	if (key == NULL || event == NULL)
		return -2;
	if (!CheckKey(key))
		return -2;

	if (done) {
		groupIndex = 0;
		char sql[CONFIG_FIXEDARRAY];
		int cur = (int)Time_TimeStamp();
		SNPRINTF1(sql, "update %s set valid=0, role_id=%lld, line=%d, gettime=FROM_UNIXTIME(%d) where code='%s'", Config_GiftTable(), (long long)roleID, Config_Line(), cur, key);
		FreeResultPublic();
		if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.connPublic));
			return -2;
		}
		return 0;
	} else {
		char sql[CONFIG_FIXEDARRAY];
		SNPRINTF1(sql, "select gift,valid,UNIX_TIMESTAMP(life), `group` from %s where code='%s'", Config_GiftTable(), key);
		FreeResultPublic();
		if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.connPublic));
			return -2;
		}

		package.res = mysql_store_result(package.connPublic);
		if (package.res == NULL) {
			DEBUG_LOGERROR("Failed to store result, error: %s", mysql_error(package.connPublic));
			return -2;
		}

		if (mysql_num_rows(package.res) != 1 || mysql_num_fields(package.res) != 4) {
			return -2;
		}

		MYSQL_ROW row = mysql_fetch_row(package.res);
		int valid = atoi(row[1]);
		
		int group = atoi(row[3]);
		groupIndex = group;

		if (valid == 0)
			return -1;

		int index = group / 32;
		if (index < 0 || index >= (int)size)
			return -1;
		if (event[index] & (1 << (group % 32)))
			return -1;

		time_t endTime = atoi(row[2]);
		if (Time_TimeStamp() > endTime) {
			return -3;
		}

		event[index] |= (1 << (group % 32));
		return atoi(row[0]);
	}
	return -2;
}

int DataManager_Login(const char *platform, const char *account, const char *password, char *addTime, char *deviceAddTime) {
	if (platform == NULL || account == NULL || password == NULL)
		return -2;

	if (!CheckAccountPassword(platform, account, password))
		return -2;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select password,add_time,device_addtime from %s where platform ='%s' and account='%s'", Config_AccountTable(), platform, account);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return -3;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to store result, error: %s", mysql_error(package.conn));
		return -3;
	}

	if (mysql_num_rows(package.res) != 1)
		return -2;

	MYSQL_ROW row = mysql_fetch_row(package.res);
	if (addTime != NULL)
		strcpy(addTime, row[1] == NULL ? "" : row[1]);
	if (deviceAddTime != NULL)
		strcpy(deviceAddTime, row[2] == NULL ? "" : row[2]);
	if (strcmp(password, row[0]) != 0)
		return -3;

	return 0;
}

struct RechargeData {
	DCProto_Recharge recharge;
	int count;
};

static void QueryRecharge(void *arg) {
	struct RechargeData *unit = (struct RechargeData *)arg;
	int ret = DataManager_RechargeValue(&unit->recharge, unit->count + 1);
	if (ret >= 0) {
		unit->recharge.set_rmb(ret);
		DCAgent_SendProtoToGCAgent(&unit->recharge);
	}
	delete unit;
}

static void QueryRechargeLater(const DCProto_Recharge *recharge, int count) {
	const char *order = recharge->recharge().order().c_str();
	const char *platform = recharge->info().platform().c_str();
	const char *account = recharge->info().account().c_str();
	int64_t roleID = recharge->roleID();
	if (count >= Config_QueryRechargeCount()) {
		char sql[CONFIG_FIXEDARRAY];
		SNPRINTF1(sql, "select money from %s where orderid='%s' and channel=%d and uid='%s' and state=0 and line=%d and role_id=%lld", Config_RechargeTable(), order, Config_ChannelRechargeNum(platform), account, Config_Line(), (long long)roleID);
		DEBUG_LOGERROR("Failed to get recharge value, sql: %s", sql);
		return;
	}

	struct RechargeData *unit = new struct RechargeData;
	unit->recharge = *recharge;
	unit->count = count;
	int interval = Config_QueryRechargeInterval() * (count + 1);
	if (interval > 60)
		interval = 60;
	Timer_AddEvent(Time_TimeStamp() + interval, -1, QueryRecharge, unit, NULL, TIMER_THREAD_DC);
}

int32_t DataManager_RechargeValue(const DCProto_Recharge *recharge, int count) {
	if (recharge == NULL)
		return -1;

	const char *order = recharge->recharge().order().c_str();
	const char *platform = recharge->info().platform().c_str();
	const char *account = recharge->info().account().c_str();
	int64_t roleID = recharge->roleID();

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select money from %s where orderid='%s' and channel=%d and uid='%s' and state=0 and line=%d and role_id=%lld", Config_RechargeTable(), order, Config_ChannelRechargeNum(platform), account, Config_Line(), (long long)roleID);
	FreeResultRecharge();
	if (mysql_real_query(package.connRecharge, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.connRecharge));
		// QueryRechargeLater(recharge, count);
		return -1;
	}
	package.resRecharge = mysql_store_result(package.connRecharge);
	if (package.resRecharge == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.connRecharge));
		// QueryRechargeLater(recharge, count);
		return -1;
	}
	if (mysql_num_rows(package.resRecharge) != 1 || mysql_num_fields(package.resRecharge) != 1) {
		// DEBUG_LOGERROR("Failed to get recharge value, account: %s, roleID: %lld", account, roleID);
		QueryRechargeLater(recharge, count);
		return -1;
	}
	MYSQL_ROW row = mysql_fetch_row(package.resRecharge);
	int32_t v = atoi(row[0]) / Config_ChannelRMBBase(platform);
	/*
	// test
	if (v == 0)
	v = 1;
	*/
	return v >= 0 ? v : -1;
}

void DataManager_RechargeOver(const char *order, const PlayerInfo *info, int64_t roleID, int level) {
	if (info == NULL || order == NULL)
		return;

	const char *platform = info->platform().c_str();
	const char *account = info->account().c_str();
	const char *deviceID = info->deviceID().c_str();
	const char *idfa = info->idfa().c_str();
	const char *osversion = info->osversion().c_str();
	const char *phonetype = info->phonetype().c_str();
	const char *imei = info->imei().c_str();

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "update %s set state=1, device_id='%s', idfa='%s', osversion='%s', phonetype='%s', imei='%s', level=%d, s_channel='%s'  where orderid='%s' and channel=%d and uid='%s' and state=0 and line=%d and role_id=%lld", Config_RechargeTable(), deviceID, idfa, osversion, phonetype, imei, level, platform, order, Config_ChannelRechargeNum(platform), account, Config_Line(), (long long)roleID);
	FreeResultRecharge();
	mysql_real_query(package.connRecharge, sql, strlen(sql));
}

void DataManager_CostRecord(const DCProto_CostRecord *cr) {
	if (cr == NULL)
		return;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "insert into %s (role, reason, rmb_value, subrmb_value, overrmb, oversubrmb, arg1, arg2, line, uid, device_id, idfa, level, osversion, phonetype, imei, channel) values(%lld, '%s', %d, %d, %lld, %lld, %d, %d, %d, '%s', '%s', '%s', %d, '%s', '%s', '%s', '%s')", Config_CostRecordTable(), (long long)cr->role(), cr->reason().c_str(), cr->rmbValue(), cr->subrmbValue(), (long long)cr->rmb(), (long long)cr->subRMB(), cr->arg1(), cr->arg2(), Config_Line(), cr->info().account().c_str(), cr->info().deviceID().c_str(), cr->info().idfa().c_str(), cr->level(), cr->info().osversion().c_str(), cr->info().phonetype().c_str(), cr->info().imei().c_str(), cr->info().platform().c_str());
	FreeResultPublic();
	if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0)
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.connPublic));
}

void DataManager_SaveChat(const DCProto_SaveChat* info) {
	if (info == NULL)
		return;
	if (Config_ChatTable() == NULL)
		return;

	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "insert into %s (line, type, sender, receiver, content, senderID, receiverID) values(%d, %d, '%s', '%s', '%s', %lld, %lld)", Config_ChatTable(), Config_Line(), info->type(), info->sender().c_str(), info->receiver().c_str() == NULL ? "" : info->receiver().c_str(), info->content().c_str(), (long long)info->senderID(), (long long)info->receiverID());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0)
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
}

void DataManager_SaveRestrictionRecord(const char* str, int64_t num, int count) {
	char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select mapID,roleID,name,professionType,score from %s where name = '%s'", Config_SingleTable(), str);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		return;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		return;
	}

	if (mysql_num_rows(package.res) == 0) {
		int index = 10400 + count;
		SNPRINTF1(sql, "insert into %s ( mapID,roleID,name,professionType,score) values( %d, %lld,'%s', %d, %lld)", Config_SingleTable(), index, (long long)0, str, 0,(long long)num);
		FreeResult();
		if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to updata restrictionrecord: %s, error: %s", sql, mysql_error(package.conn));
		}
		return;
	}

	if (mysql_num_rows(package.res) != 1)
		return;

	SNPRINTF1(sql, "update %s set score = %lld where name = '%s'", Config_SingleTable(), (long long)num, str);
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to updata restrictionrecord: %s, error: %s", sql, mysql_error(package.conn));
		return;
	}

}


void DateManager_GMSaveData(const DCProto_GMSaveData* gmSaveData) {
	if (gmSaveData == NULL) {
		return;
	}

	if (Config_GMTable() == NULL) {
		return;
	}

	char sql[CONFIG_FIXEDARRAY];

	if (gmSaveData->addOrDel()) {
		SNPRINTF1(sql, "insert into %s (line, id, roleID, name, level, profession, GM, op, time0, time1) value(%d, %d, %lld, '%s', %d, %d, '%s', %d, FROM_UNIXTIME(%d), FROM_UNIXTIME(%d))", Config_GMTable(), Config_Line(), gmSaveData->gmData().id(),(long long)gmSaveData->gmData().roleID(), gmSaveData->gmData().name().c_str(), gmSaveData->gmData().level(), gmSaveData->gmData().profession(),gmSaveData->gmData().GM().c_str(), gmSaveData->gmData().flag(), gmSaveData->gmData().startTime(), gmSaveData->gmData().endTime());
		FreeResult();
		if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to insert: %s, error: %s", sql, mysql_error(package.conn));
			return;
		}
	} else {
		SNPRINTF1(sql, "delete from %s where line = %d and id = %d", Config_GMTable(), Config_Line(), gmSaveData->gmData().id());
		FreeResult();
		if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to delete: %s, error: %s", sql, mysql_error(package.conn));
			return;
		}
	}
}

bool DataMansger_GMLoadData(DCProto_GMLoadData* gmLoadData) {
	if (gmLoadData == NULL) {
		return false;
	}

	if (Config_GMTable() == NULL) {
		return false;
	}

	char sql[CONFIG_FIXEDARRAY];

	SNPRINTF1(sql, "select id, roleID, name, level, profession, GM, op, UNIX_TIMESTAMP(time0), UNIX_TIMESTAMP(time1) from %s where line = %d", Config_GMTable(), Config_Line());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return false;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return false;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		DCProto_GMData* gmData = gmLoadData->add_gmData();

		gmData->set_id(atoi(row[0]));
		gmData->set_roleID((int64_t)atoll(row[1]));
		gmData->set_name((const char*)row[2]);
		gmData->set_level(atoi(row[3]));
		gmData->set_profession(atoi(row[4]));
		gmData->set_GM((const char*)row[5]);
		gmData->set_flag(atoi(row[6]));
		gmData->set_startTime(atoi(row[7]));
		gmData->set_endTime(atoi(row[8]));
	}
	return true;
}

bool DataManager_GMLoadAtt(DCProto_GMPlayerQuery* gmQuery) {
	if (gmQuery == NULL)
		return false;

	if (gmQuery->att().att().baseAtt().has_roleID()) {
		return DataManager_LoadRoleData(gmQuery->att().att().baseAtt().roleID(), gmQuery->mutable_att());
	} else if (gmQuery->att().att().baseAtt().has_name()) {
		return DataManager_LoadRoleData(gmQuery->att().att().baseAtt().name().c_str(), gmQuery->mutable_att());
	} else {
		return false;
	}
}

int DataManager_RegistDeviceServer(const char* str, bool flag, int time) {
	if (str == NULL) {
		return -1;
	}

	static char sql[CONFIG_FIXEDARRAY];
	if (flag) {
		SNPRINTF1(sql, "select count(*) from %s where device='%s'", Config_DeviceTable(), str);
		FreeResultPublic();
		if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
			return -1;
		}

		package.resPublic = mysql_store_result(package.connPublic);
		if (package.resPublic == NULL) {
			DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.connPublic));
			return -1;
		}

		MYSQL_ROW row = mysql_fetch_row(package.resPublic);
		if (atoi(row[0]) <= 0) {
			if (time == 0) {
				SNPRINTF1(sql, "insert into %s (line, device) value(%d, '%s')", Config_DeviceTable(), -1, str);
			}
			else {
				SNPRINTF1(sql, "insert into %s (line, device, registtime) value(%d, '%s', FROM_UNIXTIME(%d))", Config_DeviceTable(), -1, str, time);
			}
			FreeResultPublic();
			if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to insert: %s, error: %s", sql, mysql_error(package.connPublic));
				return -1;
			}
			return 0;
		}
	} else {
		SNPRINTF1(sql, "select count(*) from %s where line = %d and device='%s'", Config_DeviceTable(), Config_Line(), str);

		FreeResultPublic();
		if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.connPublic));
			return -1;
		}

		package.resPublic = mysql_store_result(package.connPublic);
		if (package.resPublic == NULL) {
			DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.connPublic));
			return -1;
		}

		MYSQL_ROW row = mysql_fetch_row(package.resPublic);
		if (atoi(row[0]) <= 0) {
			SNPRINTF1(sql, "insert into %s (line, device) value(%d, '%s')", Config_DeviceTable(), Config_Line(), str);
			FreeResultPublic();
			if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to insert: %s, error: %s", sql, mysql_error(package.connPublic));
				return -1;
			}
			return 0;
		}
	}
	return -1;
}

int DataManager_FactionLoadData(DCProto_FactionLoadData * factionLoadData) {
	if (factionLoadData == NULL) {
		return -1;
	}

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select * from %s", Config_FactionTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -1;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -1;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		DCProto_FactionData* factionData = factionLoadData->add_data();

		if (strlen( (const char*)row[0]) == 0) {
			factionData->set_exp(atoll(row[3]));
			factionData->set_team((const char*)row[7]);
		} else {
			factionData->set_factionName((const char*)row[0]);
			factionData->set_name((const char*)row[1]);
			factionData->set_num(atoi(row[2]));
			factionData->set_exp(atoll(row[3]));
			factionData->set_exp_time(atoi(row[4]));
			factionData->set_item(atoi(row[5]));
			factionData->set_notice((const char*)row[6]);
			factionData->set_team((const char*)row[7]);
			factionData->set_guardian((const char*)row[8]);
		}
	}

	return 0;
}

int DataManager_FactionSaveData(DCProto_FactionSaveData * factionSaveData) {
	/*
	   if (factionSaveData == NULL) {
	   return -1;
	   }

	   static char sql[CONFIG_FIXEDARRAY];
	   for (int i = 0; i < factionSaveData.size(); ++i) {
	   SNPRINTF1(sql, "insert into %s (factionName, name, num, exp, exp_time, item, notice, team, guardian) values('%s', '%s', %d, %ld, %d, %d, '%s', '%s', '%s')", Config_FactionTable(),);


	   }
	   */
	return 0;
}

int DataManager_FactionAddRecord(DCProto_FactionAddRecord * factionAddRecord) {
	if (factionAddRecord == NULL) {
		return -1;
	}

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "insert into %s (factionName, name, num, exp, exp_time, item, notice, team, guardian) values('%s', '%s', %d, %lld, %d, %d, '%s', '%s', '%s')", Config_FactionTable(), factionAddRecord->data().factionName().c_str(), factionAddRecord->data().name().c_str(), factionAddRecord->data().num(), (long long)factionAddRecord->data().exp(), factionAddRecord->data().exp_time(), factionAddRecord->data().item(), factionAddRecord->data().notice().c_str(), factionAddRecord->data().team().c_str(), factionAddRecord->data().guardian().c_str());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -1;
	}
	return 0;
}

int DataManager_FactionDelRecord(DCProto_FactionDelRecord * factionDelRecord) {
	if (factionDelRecord == NULL) {
		return -1;
	}

	static char sql[CONFIG_FIXEDARRAY];
	if (factionDelRecord->data().factionName() != "") {
		SNPRINTF1(sql, "delete from %s where factionName='%s'", Config_FactionTable(), factionDelRecord->data().factionName().c_str());
	} else {
		SNPRINTF1(sql, "delete from %s where exp=%lld and factionName = ''", Config_FactionTable(), (long long)factionDelRecord->data().exp());
	}
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: '%s'", sql, mysql_error(package.conn));
		return -1;
	}
	return 0;
}

int DataManager_FactionUpdateRecord(DCProto_FactionUpdateRecord * factionUpdateRecord) {
	if (factionUpdateRecord == NULL) {
		return -1;
	}

	static char sql[CONFIG_FIXEDARRAY];
	if (factionUpdateRecord->data().factionName() == "") {
		SNPRINTF1(sql, "update %s set team = '%s' where exp = %lld", Config_FactionTable(), factionUpdateRecord->data().team().c_str(), (long long)factionUpdateRecord->data().exp());
	} else {
		SNPRINTF1(sql, "update %s set name = '%s', num = %d, exp = %lld, exp_time = %d, item = %d, notice = '%s', team = '%s', guardian = '%s' where factionName = '%s'", Config_FactionTable(), factionUpdateRecord->data().name().c_str(), factionUpdateRecord->data().num(), (long long)factionUpdateRecord->data().exp(), factionUpdateRecord->data().exp_time(), factionUpdateRecord->data().item(), factionUpdateRecord->data().notice().c_str(), factionUpdateRecord->data().team().c_str(), factionUpdateRecord->data().guardian().c_str(), factionUpdateRecord->data().factionName().c_str());
	}

	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: '%s'", sql, mysql_error(package.conn));
		return -1;
	}
	return 0;
}

int DataManager_LoadRankRecord(DCProto_InitRank* rank) {
	if (rank == NULL) {
		return -1;
	}

	static char sql[CONFIG_FIXEDARRAY];
	if (rank->type() == NetProto_Rank::POWER) {
		SNPRINTF1(sql, "select id, name, occupational, power from %s order by power desc", Config_RoleTable());
	} else if (rank->type() == NetProto_Rank::TOWER) {
		SNPRINTF1(sql, "select id, name, occupational, tower from %s order by tower desc", Config_RoleTable());
	} else if (rank->type() == NetProto_Rank::LEVEL) {
		SNPRINTF1(sql, "select id, name, occupational, level from %s order by level desc", Config_RoleTable());
	} else if (rank->type() == NetProto_Rank::WORLD_BOSS) {
		if (rank->flag() == false) {
			SNPRINTF1(sql, "select roleID, name, professionType from %s where mapID = -1 and score = %d", Config_SingleTable(), MapPool_WorldBossNum());
			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
				return -1;
			}

			package.res = mysql_store_result(package.conn);
			if (package.res == NULL) {
				DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
				return -1;
			}

			RecordInfo* info = rank->mutable_finalKiller();
			if (mysql_num_rows(package.res) == 0) {
				info->mutable_role()->set_roleID(-1);
			} else {
				MYSQL_ROW row = mysql_fetch_row(package.res);
				info->mutable_role()->set_roleID(atoll(row[0]));
				info->mutable_role()->set_name((const char*)row[1]);
				info->mutable_role()->set_professionType((PB_ProfessionInfo::Type)atoi(row[2]));
			}
		} else {
			SNPRINTF1(sql, "delete from %s where mapID = -1", Config_SingleTable());

			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
				return -1;
			}

			SNPRINTF1(sql, "insert into %s set mapID=%d,roleID=%lld,name='%s',professionType=%d,score=%lld", Config_SingleTable(), -1, (long long)rank->finalKiller().role().roleID(), rank->finalKiller().role().name().c_str(), (int)rank->finalKiller().role().professionType(), (long long)rank->finalKiller().arg1());

			FreeResult();
			if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
				return -1;
			}
		}
		SNPRINTF1(sql, "select id, name, occupational, worldBossHurt from %s where worldBossNum = %d order by worldBossHurt desc", Config_RoleTable(), MapPool_WorldBossNum());
	} else if (rank->type() == NetProto_Rank::GOD) {
		SNPRINTF1(sql, "select id, name, occupational, level, power from %s order by godRank asc", Config_RoleTable());
	} else if (rank->type() == NetProto_Rank::BLESSCOME) {
		SNPRINTF1(sql, "select id, name, occupational, bless from %s where bless > 0 order by bless desc", Config_RoleTable());
	} else if (rank->type() == NetProto_Rank::FACTION) {
		SNPRINTF1(sql, "select factionName, `name`, exp from %s where factionName != '' order by exp desc limit %d", Config_FactionTable(), Config_RankCount());
	} else if (rank->type() == NetProto_Rank::PET) {
		//	SNPRINTF1(sql, "select roleID, `name`, score from %s where mapID = -13 order by score desc limit %d", Config_SingleTable(), Config_RankCount());
		SNPRINTF1(sql, "select `name`, score from %s where mapID = -13 order by score desc limit %d", Config_SingleTable(), Config_RankCount());
	} else if (rank->type() == NetProto_Rank::DEVIL) {
		SNPRINTF1(sql, "select roleID, `name`, professionType, score from %s where mapID = -14", Config_SingleTable());
	} else if (rank->type() == NetProto_Rank::LUCK) {
		SNPRINTF1(sql, "select id, `name`, occupational, luck from %s where luck > 0 order by luck desc", Config_RoleTable());
	} else if (rank->type() == NetProto_Rank::CONSUME) {
		SNPRINTF1(sql, "select roleID, `name`, professionType, score from %s where mapID = -15 order by score limit 10", Config_SingleTable());
	} else if (rank->type() == NetProto_Rank::CATGIFT) {
		SNPRINTF1(sql, "select roleID, score from %s where mapID = -21 ", Config_SingleTable());
	} else if (rank->type() == NetProto_Rank::GROUPRECORD) {
		SNPRINTF1(sql, "select roleID, score from %s where mapID = -22 ", Config_SingleTable());
	} else if (rank->type() == NetProto_Rank::GROUP_PURCHASE) {
		SNPRINTF1(sql, "select id, `name`, occupational, grouppurchase from %s where grouppurchase > 0 order by grouppurchase desc", Config_RoleTable());
	} else if (rank->type() == NetProto_Rank::RESERVATION) {
		SNPRINTF1(sql, "select roleID, yuezhan from %s where mapID = -24 and score > %d", Config_SingleTable(), (int)rank->finalKiller().arg1());
	} else {
		return -2;
	}

	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -1;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -1;
	}

	if (mysql_num_rows(package.res) == 0) {
		return 0;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		RecordInfo* info = rank->add_rank();

		if (rank->type() == NetProto_Rank::FACTION) {
			static char ally[CONFIG_FIXEDARRAY];
			SNPRINTF1(ally, "%s,%s", (const char*)row[0], (const char*)row[1]);
			info->mutable_role()->set_name(ally);
			info->set_arg1(atoll(row[2]));
		}else if (rank->type() == NetProto_Rank::PET) {
			//	info->mutable_role()->set_roleID(atoll(row[0]));
			info->mutable_role()->set_name((const char*)row[0]);
			info->set_arg1(atoll(row[1]));
		}else if (rank->type() == NetProto_Rank::CATGIFT) {
			info->mutable_role()->set_roleID(atoll(row[0]));
			info->set_arg1(atoll(row[1]));
		}else if (rank->type() == NetProto_Rank::GROUPRECORD) {
			info->mutable_role()->set_roleID(atoll(row[0]));
			info->set_arg1(atoll(row[1]));
		}else if (rank->type() == NetProto_Rank::RESERVATION) {
			info->mutable_role()->set_roleID(atoll(row[0]));
			info->mutable_role()->set_name((const char *)row[1]);
		} else {
			info->mutable_role()->set_roleID(atoll(row[0]));
			info->mutable_role()->set_name((const char*)row[1]);
			if (rank->type() == NetProto_Rank::DEVIL || rank->type() == NetProto_Rank::CONSUME) {
				info->mutable_role()->set_professionType(PB_ProfessionInfo_Type(atoi(row[2])));
			}else {
				PB_ProfessionInfo_Type type;
				PB_ProfessionInfo::Type_Parse((const char*)row[2], &type);
				info->mutable_role()->set_professionType(type);
			}
			int64_t value = 0;
			if (rank->type() == NetProto_Rank::GOD){
				value = (((long long)atoi(row[3]) << 32)) + atoi(row[4]);
			}else {
				value = (long long)atoi(row[3]);
			}
			info->set_arg1(value);
		}
	}
	return 0;
}

void DataManager_SysFactionMemInfo(DCProto_SysFactionMemInfo* info) {
	if (info == NULL) {
		return;
	}

	static PB_PlayerAtt att;
	att.Clear();
	if (DataManager_LoadRoleData(info->roleID(), &att)) {
		att.set_faction(info->str());
		DataManager_SaveRoleData(&att);
	}
}

void DataManager_ModifyGodRank(const DCProto_ModifyGodRank* info) {
	if (info == NULL)
		return;

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "update %s set godRank=%d where id=%lld", Config_RoleTable(), info->rank(), (long long)info->roleID());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: '%s'", sql, mysql_error(package.conn));
		return;
	}
	return;
}

int DataManager_SaveGodRankInfoRecord(DCProto_SaveGodRankInfoRecord* info) {
	if (info == NULL) {
		return -1;
	}

	static char sql[CONFIG_FIXEDARRAY];
	if (info->info1().flag()) {
		SNPRINTF1(sql, "select name, occupational, level from %s where id=%lld", Config_RoleTable(), (long long)info->info1().info().role().roleID());
		FreeResult();
		if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
			return -2;
		}

		package.res = mysql_store_result(package.conn);
		if (package.res == NULL) {
			DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
			return -3;
		}

		if (mysql_num_rows(package.res) == 0) {
			return -4;
		}

		MYSQL_ROW row = mysql_fetch_row(package.res);
		info->mutable_info1()->mutable_info()->mutable_role()->set_name((const char*)row[0]);
		PB_ProfessionInfo_Type type;
		PB_ProfessionInfo::Type_Parse((const char*)row[1], &type);
		info->mutable_info1()->mutable_info()->mutable_role()->set_professionType((type));
		int level = atoi(row[2]);
		info->mutable_info1()->mutable_info()->set_arg1(info->info1().info().arg1() + level);
	} else {
		static PB_PlayerAtt att;
		att.Clear();
		if (DataManager_LoadRoleData(info->info2().info().role().roleID(), &att)) {
			int index = att.godRankIndex(); 
			*(att.mutable_godRecordInfo(index % att.godRecordInfo_size())) = info->info1().info().role();
			att.set_godRecordArg1(index % att.godRecordArg1_size(), info->info1().info().arg1());
			att.set_godRankIndex(index + 1);
			DataManager_SaveRoleData(&att);
		}
	}

	if (info->info2().flag()) {
		SNPRINTF1(sql, "select name, occupational, level from %s where id=%lld", Config_RoleTable(), (long long)info->info2().info().role().roleID());
		FreeResult();
		if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
			return -2;
		}

		package.res = mysql_store_result(package.conn);
		if (package.res == NULL) {
			DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
			return -3;
		}

		if (mysql_num_rows(package.res) == 0) {
			return -4;
		}

		MYSQL_ROW row = mysql_fetch_row(package.res);
		info->mutable_info2()->mutable_info()->mutable_role()->set_name((const char*)row[0]);
		PB_ProfessionInfo_Type type;
		PB_ProfessionInfo::Type_Parse((const char*)row[1], &type);
		info->mutable_info2()->mutable_info()->mutable_role()->set_professionType((type));
		int level = atoi(row[2]);
		info->mutable_info2()->mutable_info()->set_arg1(info->info2().info().arg1() + level);
	} else {
		static PB_PlayerAtt att;
		att.Clear();
		if (DataManager_LoadRoleData(info->info1().info().role().roleID(), &att)) {
			int index = att.godRankIndex();
			if (att.godRecordInfo_size() < Config_GodRankSize()) {
				*att.add_godRecordInfo() = info->info2().info().role();
			} else {
				*(att.mutable_godRecordInfo(index % att.godRecordInfo_size())) = info->info2().info().role();
			}	
			if (att.godRecordArg1_size() < Config_GodRankSize()) {
				att.add_godRecordArg1(info->info2().info().arg1());
			} else {
				att.set_godRecordArg1(index % att.godRecordArg1_size(), info->info2().info().arg1());
			}	
			att.set_godRankIndex(index + 1);
			DataManager_SaveRoleData(&att);
		}
	}
	return 0;
}

int DataManager_GMChatRecords(DCProto_GMChatRecords* info) {
	if (info == NULL)
		return -1;

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select UNIX_TIMESTAMP(time), type, sender, receiver, content, senderID, receiverID from %s where time >= FROM_UNIXTIME(%d) and time < FROM_UNIXTIME(%d) limit 1000", Config_ChatTable(), info->record().startTime(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -3;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		NetProto_GMChat *gmChat = info->mutable_record()->add_chat();

		gmChat->set_time(atoi(row[0]));
		gmChat->set_channel(::NetProto_Chat_Channel(atoi(row[1])));
		gmChat->mutable_sender()->set_name((const char *)row[2]);
		gmChat->mutable_recver()->set_name((const char *)row[3]);
		gmChat->set_content((const char *)row[4]);
		gmChat->mutable_sender()->set_roleID(atoll(row[5]));
		gmChat->mutable_recver()->set_roleID(atoll(row[6]));
	}	

	return 0;
}

int DataManager_GMRegistrCount(DCProto_GMRegistrCount* info) {
	if (info == NULL)
		return -1;

	static char sql[CONFIG_FIXEDARRAY];

	//----------------
	SNPRINTF1(sql, "select count(*) from %s where device_addtime >= FROM_UNIXTIME(%d) and device_addtime < FROM_UNIXTIME(%d)", Config_AccountTable(), info->record().startTime(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -3;
	}

	MYSQL_ROW row = mysql_fetch_row(package.res);
	info->mutable_record()->set_deviceID(atoi(row[0]));

	//--------------------
	SNPRINTF1(sql, "select count(*) from %s where device_addtime < FROM_UNIXTIME(%d)", Config_AccountTable(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -4;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -5;
	}

	row = mysql_fetch_row(package.res);
	info->mutable_record()->set_allDeviceID(atoi(row[0]));

	//----------------
	SNPRINTF1(sql, "select count(*) from %s where add_time >= FROM_UNIXTIME(%d) and add_time < FROM_UNIXTIME(%d)", Config_AccountTable(), info->record().startTime(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -6;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -7;
	}

	row = mysql_fetch_row(package.res);
	info->mutable_record()->set_accountID(atoi(row[0]));

	//--------------------
	SNPRINTF1(sql, "select count(*) from %s where add_time < FROM_UNIXTIME(%d)", Config_AccountTable(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -8;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -9;
	}

	row = mysql_fetch_row(package.res);
	info->mutable_record()->set_allAccountID(atoi(row[0]));

	//----------------
	SNPRINTF1(sql, "select count(*) from %s where create_time >= FROM_UNIXTIME(%d) and create_time < FROM_UNIXTIME(%d)", Config_RoleTable(), info->record().startTime(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -10;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -11;
	}

	row = mysql_fetch_row(package.res);
	info->mutable_record()->set_roleID(atoi(row[0]));

	//--------------------
	SNPRINTF1(sql, "select count(*) from %s where create_time < FROM_UNIXTIME(%d)", Config_RoleTable(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -12;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -12;
	}

	row = mysql_fetch_row(package.res);
	info->mutable_record()->set_allRoleID(atoi(row[0]));

	return 0;
}

int DataManager_GMLevelStatistics(DCProto_GMLevelStatistics* info) {
	if (info == NULL)
		return -1;

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select level, count(*) from %s where create_time >= FROM_UNIXTIME(%d) and create_time < FROM_UNIXTIME(%d) group by level", Config_RoleTable(), info->record().startTime(), info->record().endTime());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -3;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		LevelStatistics *gmLevel = info->mutable_record()->add_array();

		gmLevel->set_level(atoi(row[0]));
		gmLevel->set_count(atoi(row[1]));
	}	

	return 0;
}

int DataManager_GMRoleCount(DCProto_GMRoleCount* info) {
	if (info == NULL)
		return -1;

	int interval = (info->record().endTime() - info->record().startTime()) / 12;
	interval = interval < 300 ? 300 : interval;
	info->mutable_record()->set_interval(interval);

	static char sql[CONFIG_FIXEDARRAY];
	memset(sql, 0, sizeof(sql));
	char *index = sql;

	SNPRINTF2(sql, index, " (select roleID, score from %s where mapID = -100 and score between %d and %d  and roleID = (select max(roleID) from %s where mapID = -100 and score between %d and %d) limit 1 )", Config_SingleTable(), info->record().startTime(), info->record().endTime(), Config_SingleTable(), info->record().startTime(), info->record().endTime());
	index += strlen(index);
	SNPRINTF2(sql, index, " union all ( select roleID, score from %s where mapID = -100 and score between %d and %d  and roleID = (select min(roleID) from %s where mapID = -100 and score between %d and %d) limit 1) ", Config_SingleTable(), info->record().startTime(), info->record().endTime(), Config_SingleTable(), info->record().startTime(), info->record().endTime());

	int begin = info->record().startTime();
	int end = begin + interval;
	while (end <= info->record().endTime()) {
		index += strlen(index);
		SNPRINTF2(sql, index, " union all (select round(avg(roleID), 0), round(avg(score), 0) from %s where mapID = -100 and score between %d and %d) ", Config_SingleTable(), begin, end);
		begin = end;
		end += interval;
	}

	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -3;
	}

#define IS_NULL_OR_EMPTY(v) ((v[0]) == NULL || (v[1]) == NULL || (v[0])[0] == '\0' || (v[1])[0] == '\0')
	int count = 0;
	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res), ++count) {
		if (!IS_NULL_OR_EMPTY(row)) {
			if (count == 0) {
				info->mutable_record()->set_maxCount(atoi(row[0]));
				info->mutable_record()->set_maxCountTime(atoi(row[1]));
			}else if (count == 1) {
				info->mutable_record()->set_minCount(atoi(row[0]));
				info->mutable_record()->set_minCountTime(atoi(row[1]));
			}else {
				info->mutable_record()->add_count(atoi(row[0]));
				info->mutable_record()->add_time(atoi(row[1]));
			}
		}
	}	
#undef IS_NULL_OR_EMPTY

	return 0;
}

int DataManager_QueryRoleFaction(DCProto_QueryRoleFaction* info) {
	if (info == NULL)
		return -1;

	for (int i = 0; i < info->roleID_size(); ++i) {
		PB_PlayerAtt data;
		if (DataManager_LoadRoleData(info->roleID(i), &data)) {
			info->set_faction(i, data.faction().c_str());
		}
	}
	return 0;
}

int DataManager_FactionPower(DCProto_FactionPower* info) {
	if (info == NULL)
		return -1;

	if (info->info_size() == 0)
		return 0;

	static char sql[CONFIG_FIXEDARRAY * 20];
	memset(sql, 0, sizeof(sql));
	char *index = sql;

	for (int i = 0; i < info->info_size(); ++i) {
		if (i != 0) {
			SNPRINTF2(sql, index, " union all  ");
			index += strlen(index);
		}

		SNPRINTF2(sql, index, " select sum(power) from %s where id in ", Config_RoleTable());
		index += strlen(index);

		if (0 == (int)info->info(i).roleID_size()) {
			SNPRINTF2(sql, index, " ( -1 ) ");
			index += strlen(index);
			continue;
		}else {
			SNPRINTF2(sql, index, " ( ");
			index += strlen(index);

			for (int j = 0; j < (int)info->info(i).roleID_size(); ++j) {
				if (j != 0) {
					SNPRINTF2(sql, index, " , ");
					index += strlen(index);
				}

				SNPRINTF2(sql, index, " %lld ", (long long)info->info(i).roleID(j));
				index += strlen(index);
			}

			SNPRINTF2(sql, index, " ) ");
			index += strlen(index);
		}

	}

	//	DEBUG_LOG("faction power: %s",sql);

	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -3;
	}

	int powerIndex = 0;
	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		if (row[0] == NULL) {
			info->mutable_info(powerIndex)->set_power(0);
		}else {
			info->mutable_info(powerIndex)->set_power(atoll(row[0]));
		}
		powerIndex++;
	}	
	return 0;
}

int DataManager_LoadAllDataFromGMDataTable(DCProto_LoadAllDataFromGMDataTable * info) {
	if (info == NULL)
		return -1;

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select `key`, arg1, arg2, arg3, str1, str2, str3, roleID from %s where line = %d or line = -1 ", Config_GMDataTable(), Config_Line());
	FreeResultPublic();
	if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.resPublic = mysql_store_result(package.connPublic);
	if (package.resPublic == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.connPublic));
		return -3;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.resPublic); row != NULL; row = mysql_fetch_row(package.resPublic)) {
		DCProto_GMInfo *gm = info->add_info();

		gm->set_key(atoi(row[0]));
		gm->set_arg1(atoi(row[1]));
		gm->set_arg2(atoi(row[2]));
		gm->set_arg3(atoi(row[3]));
		gm->set_str1((const char *)row[4]);
		gm->set_str2((const char *)row[5]);
		gm->set_str3((const char *)row[6]);
		gm->set_roleID(atoll(row[7]));
	}	

	return 0;
}

void DataManager_SaveGMDataTable(DCProto_SaveGMDataTable* info) {
	if (info == NULL)
		return;

	static char sql[CONFIG_FIXEDARRAY];
	for (int i = 0; i < info->info_size(); ++i) {
		if (info->op() == 1) {
			if (info->info(i).key() == -2) {
				SNPRINTF1(sql, "insert into %s set line=-1, `key`=%d, arg1 = %d, arg2 = %d, arg3 = %d, str1 = \"%s\", str2 = \"%s\", str3 = \"%s\", roleID = %lld", Config_GMDataTable(), info->info(i).key(), info->info(i).arg1(), info->info(i).arg2(), info->info(i).arg3(), info->info(i).str1().c_str(), info->info(i).str2().c_str(), info->info(i).str3().c_str(), (long long)info->info(i).roleID());
			}else {
				SNPRINTF1(sql, "insert into %s set line=%d, `key`=%d, arg1 = %d, arg2 = %d, arg3 = %d, str1 = \"%s\", str2 = \"%s\", str3 = \"%s\", roleID = %lld", Config_GMDataTable(), Config_Line(), info->info(i).key(), info->info(i).arg1(), info->info(i).arg2(), info->info(i).arg3(), info->info(i).str1().c_str(), info->info(i).str2().c_str(), info->info(i).str3().c_str(), (long long)info->info(i).roleID());
			}
			FreeResultPublic();
			if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				continue;
			}
		}else if (info->op() == 2) {
			SNPRINTF1(sql, "delete from %s where line=%d and `key`=%d and arg1 = %d and arg2 = %d and arg3 = %d and  str1 = \"%s\" and str2 = \"%s\" and str3 = \"%s\" and roleID = %lld ", Config_GMDataTable(), Config_Line(), info->info(i).key(), info->info(i).arg1(), info->info(i).arg2(), info->info(i).arg3(), info->info(i).str1().c_str(), info->info(i).str2().c_str(), info->info(i).str3().c_str(), (long long)info->info(i).roleID());
			FreeResultPublic();
			if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
				DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
				continue;
			}
		}
	}
}

void DataManager_GMAddExchange(DCProto_GMAddExchange * info) {
	if (info == NULL)
		return;

#define RETURN { if (sql != NULL) delete[] sql; return; }
	int group = info->info().group();
	char *sql = new char[CONFIG_FIXEDARRAY * 1024];

	if (group == -1) {
		SNPRINTF1(sql, "select max(`group`) from %s ", Config_GiftTable());
		FreeResultPublic();
		if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.connPublic));
			RETURN;
		}

		package.resPublic = mysql_store_result(package.connPublic);
		if (package.resPublic == NULL) {
			DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.connPublic));
			RETURN;
		}

		MYSQL_ROW row = mysql_fetch_row(package.resPublic);
		if (row == NULL || row[0] == NULL) {
			group = 0; 
		}else {
			group = atoi(row[0]) + 1; 
		}
	}

	if (group >= 93) 
		RETURN;

	char* index = sql;
	SNPRINTF1(sql, "insert into %s value ", Config_GiftTable());
	index += strlen(index);

	for (int i = 0; i < info->info().exchange_size(); ++i) {
		if (i == 0) {
			SNPRINTF2(sql, index, " (%s, %d, %d, FROM_UNIXTIME(%d), %d ) ", info->info().exchange(i).c_str(), info->info().giftID(), 1, info->info().endTime(), group);
			index += strlen(index);
		}else {
			SNPRINTF2(sql, index, " ,(%s, %d, %d, FROM_UNIXTIME(%d), %d ) ", info->info().exchange(i).c_str(), info->info().giftID(), 1, info->info().endTime(), group);
			index += strlen(index);
		}
	}

	if (info->info().exchange_size() > 0) {
		FreeResultPublic();
		if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
			DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.connPublic));
			RETURN;
		}
	}

	RETURN;
#undef RETURN
}

void DataManager_GMAddRekooRole(DCProto_GMRekooRole * info) {
	if (info == NULL)
		return;

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "insert into %s set roleID=%lld,gm=\"%s\",time=FROM_UNIXTIME(%d)", Config_RekooRoleTable(), (long long)info->info().roleID(), info->str().c_str(), (int)Time_TimeStamp());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return;
	}
}

int DataManager_LoadRekooRole(DCProto_LoadRekooRole * info) {
	if (info == NULL)
		return -1;

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select roleID from %s ", Config_RekooRoleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -3;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		info->add_roleID(atoll(row[0]));
	}
	return 0;	
}

int DataManager_LoadInviteCode(DCProto_LoadInviteCode * info) {
	if (info == NULL)
		return -1;

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select id, selfcode, othercode from %s ", Config_RoleTable());
	FreeResult();
	if (mysql_real_query(package.conn, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to select: %s, error: %s", sql, mysql_error(package.conn));
		return -2;
	}

	package.res = mysql_store_result(package.conn);
	if (package.res == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.conn));
		return -3;
	}

	for (MYSQL_ROW row = mysql_fetch_row(package.res); row != NULL; row = mysql_fetch_row(package.res)) {
		InviteCode * code = info->add_info();
		code->set_roleID(atoll(row[0]));
		if (row[1] != NULL) {
			code->set_selfCode((const char *)row[1]);
		}else {
			code->set_selfCode("");
		}
		if (row[2] != NULL) {
			code->set_otherCode((const char *)row[2]);
		}else {
			code->set_otherCode("");
		}
	}
	return 0;
}

void DataManager_QueryGMAccountFromSql(DCProto_QueryGMAccount * info) {
	if (info == NULL) {
		return;
	}

	static char sql[CONFIG_FIXEDARRAY];
	SNPRINTF1(sql, "select arg1 from %s where `key` = -2 and line = -1 and str1 = '%s' and str2 = '%s' ", Config_GMDataTable(), info->gm().account().c_str(), info->gm().passwd().c_str());
	FreeResultPublic();
	if (mysql_real_query(package.connPublic, sql, strlen(sql)) != 0) {
		DEBUG_LOGERROR("Failed to query: %s, error: %s", sql, mysql_error(package.conn));
		return;
	}

	package.resPublic = mysql_store_result(package.connPublic);
	if (package.resPublic == NULL) {
		DEBUG_LOGERROR("Failed to use result, error: %s", mysql_error(package.connPublic));
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(package.resPublic);
	if (row == NULL || row[0] == NULL) 
		return;

	int op = atoi(row[0]);
	if ( op == 1 || op == 2 || op == 3  ) {
		info->mutable_gm()->set_permission((::NetProto_GMLogin_OP)op);
	} 
}
