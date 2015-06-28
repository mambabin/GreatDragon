#include "ScribeClient.hpp"
#include "scribe/gen-cpp/scribe.h"
#include "PlayerEntity.hpp"
#include "Component.hpp"
#include "MapPool.hpp"
#include "MapInfoManager.hpp"
#include "NetID.hpp"
#include "Config.hpp"
#include "Debug.hpp"
#include "Timer.hpp"
#include "PlayerInfo.pb.h"
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <ctime>
#include <string>
#include <exception>
#include <vector>
#include <queue>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

using namespace std;
using boost::shared_ptr;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift;
using namespace scribe::thrift;
using namespace scribe;

#define IDLE 0
#define CONNECTING 1
#define CONNECTED 2

static struct{
	int status;
	shared_ptr<TSocket> socket;
	shared_ptr<TFramedTransport> transport;
	shared_ptr<TBinaryProtocol> protocol;
	shared_ptr<scribeClient> resendClient;
	vector<LogEntry> msgs;

	bool over;
	pthread_t sender;
	pthread_mutex_t mutex;
}package;

static void Init(bool first) {
#define RETURN {if (first) {exit(EXIT_FAILURE);} else {package.status = IDLE; return;}}
	if (package.status != IDLE)
		RETURN;
	package.status = CONNECTING;
	try{
		package.socket = shared_ptr<TSocket>(new TSocket(Config_ScribeHost(), Config_ScribePort()));
		if (!package.socket) {
			DEBUG_LOGERROR("Failed to create scribe socket");
			RETURN;
		}
		package.socket->setConnTimeout(Config_ScribeTimeout());
		package.socket->setSendTimeout(Config_ScribeTimeout());
		package.socket->setRecvTimeout(Config_ScribeTimeout());
		package.socket->setLinger(0, 0);

		package.transport = shared_ptr<TFramedTransport>(new TFramedTransport(package.socket));
		if (!package.transport) {
			DEBUG_LOGERROR("Failed to create scribe transport");
			RETURN;
		}

		package.protocol = shared_ptr<TBinaryProtocol>(new TBinaryProtocol(package.transport));
		if (!package.protocol) {
			DEBUG_LOGERROR("Failed to create scribe protocol");
			RETURN;
		}
		package.protocol->setStrict(false, false);

		package.resendClient = shared_ptr<scribeClient>(new scribeClient(package.protocol));
		if (!package.resendClient) {
			DEBUG_LOGERROR("Failed to create scribe client");
			RETURN;
		}

		package.transport->open();
	}catch(const TTransportException &error) {
		DEBUG_LOGERROR("Failed to init scribe client: %s", error.what());
		RETURN;
	}catch(...) {
		DEBUG_LOGERROR("Failed to init scribe client: unknown exceptions");
		RETURN;
	}
	if (package.transport->isOpen())
		package.status = CONNECTED;
#undef RETURN
}

static bool CheckStatus() {
	if (package.status == CONNECTED) {
		if (!package.transport || !package.transport->isOpen())
			package.status = IDLE;
	}
	if (package.status == IDLE) {
		Time_Flush();
		int64_t begin = Time_ElapsedTime();
		Init(false);
		Time_Flush();
		int64_t end = Time_ElapsedTime();
		DEBUG_LOGRECORD("ScribeClient_Init cost: %lld", end - begin);
	}
	if (package.status == CONNECTING) {
		if (package.transport->isOpen())
			package.status = CONNECTED;
	}
	return package.status == CONNECTED;
}

static void FlushScribe(vector<LogEntry> &msgs) {
	if (Config_ScribeHost() == NULL)
		return;
	if (msgs.empty())
		return;
	if (!CheckStatus())
		return;
	try {
		Time_Flush();
		ResultCode ret = package.resendClient->Log(msgs);
		if (ret == TRY_LATER) {
			DEBUG_LOGERROR("Failed to send scribe msg: TRY_LATER, count: %d", (int)msgs.size());
		} else if (ret == OK) {
			/*
			for (size_t i = 0; i < msgs.size(); i++)
				DEBUG_LOG("send scribemsg, category: %s, msg: %s", msgs[i].category.c_str(), msgs[i].message.c_str());
				*/
			/*
			   int64_t begin = Time_ElapsedTime();
			   Time_Flush();
			   int64_t end = Time_ElapsedTime();
			   DEBUG_LOGRECORD("ScribeClient_Send, msgs: %d, cost: %lld", (int)msgs.size(), end - begin);
			   */
		} else {
			DEBUG_LOGERROR("Failed to send scribe msg, ResultCode: %d, count: %d", (int)ret, (int)msgs.size());
		}
	} catch (const TTransportException &error) {
		DEBUG_LOGERROR("Failed to send scribe msg: %s, count: %d", error.what(), (int)msgs.size());
	} catch (...) {
		DEBUG_LOGERROR("Failed to send scribe msg: unknown exceptions, count: %d", (int)msgs.size());
	}
}

static void *RunSender(void *) {
	package.over = false;
	package.status = IDLE;
	Init(true);

	vector<LogEntry> msgs;
	while (true) {
		pthread_mutex_lock(&package.mutex);
		msgs = package.msgs;
		package.msgs.clear();
		pthread_mutex_unlock(&package.mutex);
		if (package.over) {
			if (msgs.empty())
				break;
		}
		FlushScribe(msgs);
		if (!package.over)
			usleep(1000 * 250);
	}

	return NULL;
}

void ScribeClient_Init() {
	if (Config_ScribeHost() == NULL)
		return;

	int res = pthread_mutex_init(&package.mutex, NULL);
	assert(res == 0);

	if (pthread_create(&package.sender, NULL, &RunSender, NULL) != 0) {
		DEBUG_LOGERROR("Failed to create thread");
		exit(EXIT_FAILURE);
	}
}

void ScribeClient_Prepare() {
}

static int Line(const char *action) {
	if (strcmp(action, "RegistDevice") == 0
			|| strcmp(action, "StartLoad1") == 0
			|| strcmp(action, "EndLoad1") == 0)
		return 0;
	else
		return Config_Line();
}

const char * ScribeClient_Format(const PlayerInfo *info, int32_t id, const char *action, const char *v1, const char *v2, const char *v3, const char *v4) {
	if (Config_ScribeHost() == NULL)
		return NULL;

	time_t cur = Time_TimeStamp();
	tm curTM;
	Time_LocalTime(cur, &curTM);

	const char *device_id = NULL;
	const char *idfa = NULL;
	const char *uid = NULL;
	const char *osversion = NULL;
	const char *phonetype = NULL;
	const char *imei = NULL;
	char channel[128] = {'\0'};
	char s_reg_days[64] = {'\0'};
	const char *s_platform = NULL;
	if (info != NULL) {
		device_id = info->deviceID().c_str();
		idfa = info->idfa().c_str();
		uid = info->account().c_str();
		osversion = info->osversion().c_str();
		phonetype = info->phonetype().c_str();
		imei = info->imei().c_str();
		const char *biChannel = Config_ChannelBIName(info->platform().c_str());
		if (biChannel != NULL)
			strcpy(channel, biChannel);
		else
			strcpy(channel, info->platform().c_str());
		s_platform = Config_ChannelPlatform(info->platform().c_str());
		/*
		   if (strcmp(channel, "rekoo") == 0) {
		   bool flag = Config_IsApplePhone(phonetype);
		   if (flag) {
		   strcat(channel, "_ios");
		   } else {
		   strcat(channel, "_android");
		   }
		   }
		   */
		if(info->addTime().length() == 19){
			int delta = (int)cur - Time_TMToTimeNull(info->addTime().c_str());
			if(delta >= 0)
				SNPRINTF1(s_reg_days, "%d", delta / (3600 * 24) + 1);
		}
	}

	const char *ip = NetID_IP(id);
	const char *mapName = NULL;
	char s_role_id[64] = {'\0'};
	char s_level[64] = {'\0'};

	struct PlayerEntity *entity = PlayerEntity_Player(id);
	if (PlayerEntity_IsValid(entity)) {
		struct Component *component = PlayerEntity_Component(entity);

		const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(component->roleAtt->movementAtt().mapID()));
		if (mapInfo != NULL)
			mapName = mapInfo->name().c_str();

		SNPRINTF1(s_role_id, "%lld", (long long)component->baseAtt->roleID());
		SNPRINTF1(s_level, "%d", component->roleAtt->fightAtt().level());
	}

#define S(s) (((s) == NULL) ? "" : (s))
	static char buf[CONFIG_FIXEDARRAY];
	int n;
	if (strcmp(Config_ScribeCategory(), "ALL") == 0) {
		n = snprintf(buf, sizeof(buf),
				"%02d:%02d:%02d	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	server%d	%s",
				curTM.tm_hour, curTM.tm_min, curTM.tm_sec,
				S(device_id),
				S(idfa),
				S(uid),
				S(s_role_id),
				S(mapName),
				S(s_level),
				S(s_reg_days),
				S(action),
				S(v1),
				S(v2),
				S(v3),
				S(v4),
				S(ip),
				S(osversion),
				S(phonetype),
				S(imei),
				S(channel),
				Line(action),
				S(s_platform)
					);
	} else {
		n = snprintf(buf, sizeof(buf),
				"%02d:%02d:%02d	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	%s	server%d",
				curTM.tm_hour, curTM.tm_min, curTM.tm_sec,
				S(device_id),
				S(idfa),
				S(uid),
				S(s_role_id),
				S(mapName),
				S(s_level),
				S(s_reg_days),
				S(action),
				S(v1),
				S(v2),
				S(v3),
				S(v4),
				S(ip),
				S(osversion),
				S(phonetype),
				S(imei),
				S(channel),
				Line(action)
					);
	}
#undef S
	return (n > 0 && n < (int)sizeof(buf)) ? buf : NULL;
}

static const char * GenCategory() {
	time_t cur = Time_TimeStamp();
	tm curTM;
	Time_LocalTime(cur, &curTM);
	static char buf[128];
	SNPRINTF1(buf, "TIANSHA_%s_%04d-%02d-%02d", Config_ScribeCategory(), curTM.tm_year + 1900, curTM.tm_mon + 1, curTM.tm_mday);
	return buf;
}

static void PushMsg(const char *msg) {
	LogEntry entry;
	entry.category = GenCategory();
	entry.message = msg;

	pthread_mutex_lock(&package.mutex);
	package.msgs.push_back(entry);
	pthread_mutex_unlock(&package.mutex);
}

void ScribeClient_SendMsg(const char *msg) {
	if (Config_ScribeHost() == NULL)
		return;
	if (msg == NULL)
		return;
	PushMsg(msg);
}


