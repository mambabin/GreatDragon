#include "Debug.hpp"
#include "Config.hpp"
#include "RidesInfoManager.hpp"
#include "PlayerEntity.hpp"
#include "PlayerPool.hpp"
#include "Movement.hpp"
#include "Equipment.hpp"
#include "Fight.hpp"
#include "Status.hpp"
#include "Item.hpp"
#include "StatusEntity.hpp"
#include "StatusInfoManager.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "EquipmentInfoManager.hpp"
#include "GoodsInfoManager.hpp"
#include "ProfessionInfoManager.hpp"
#include "SkillInfoManager.hpp"
#include "SkillEntity.hpp"
#include "SkillPool.hpp"
#include "NPCEntity.hpp"
#include "NPCPool.hpp"
#include "NPCInfoManager.hpp"
#include "AI.hpp"
#include "Func.hpp"
#include "Mission.hpp"
#include "MissionInfoManager.hpp"
#include "GodShipInfoManager.hpp"
#include "BusinessInfoManager.hpp"
#include "GlobalMissionManager.hpp"
#include "SignalHandler.hpp"
#include "Time.hpp"
#include "Timer.hpp"
#include "GCAgent.hpp"
#include "IDGenerator.hpp"
#include "NetID.hpp"
#include "NetAgent.hpp"
#include "HashArray.hpp"
#include "DCAgent.hpp"
#include "DataManager.hpp"
#include "DropTableManager.hpp"
#include "AccountPool.hpp"
#include "ConnectionPool.hpp"
#include "BloodInfoManager.hpp"
#include "BoxInfoManager.hpp"
#include "AwardInfoManager.hpp"
#include "DesignationInfoManager.hpp"
#include "VIPInfoManager.hpp"
#include "FashionInfoManager.hpp"
#include "GMPool.hpp"
#include "Web.hpp"
#include "ScribeClient.hpp"
#include "Event.hpp"
#include "AStar.hpp"
#include "MapInfoManager.hpp"
#include "PlayOffManager.hpp"
#include "MapInfo.pb.h"
#include "Math.hpp"
#include "FactionPool.hpp"
#include "TransformInfoManager.hpp"
#include "GmSet.hpp"
#include <fstream>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

static inline void SetupSignalHandlers() {
	signal(SIGINT, &SignalHandler_NormalExit);
	signal(SIGTERM, &SignalHandler_NormalExit);
	// Use core dump
	/*
	signal(SIGFPE, &SignalHandler_ExitAndLogCallStack);
	signal(SIGILL, &SignalHandler_ExitAndLogCallStack);
	signal(SIGSEGV, &SignalHandler_ExitAndLogCallStack);
	*/
}

static void InitGameCore() {
	Debug_Init();
	Time_Init();
	Config_Init();
	Timer_Init(TIMER_THREAD_GC);
	RidesInfoManager_Init();
	NPCInfoManager_Init();
	NPCEntity_Init();
	NPCPool_Init();
	ProfessionInfoManager_Init();
	EquipmentInfoManager_Init();
	GoodsInfoManager_Init();
	SkillInfoManager_Init();
	PlayerEntity_Init();
	PlayerPool_Init();
	Movement_Init();
	Equipment_Init();
	Fight_Init();
	AI_Init();
	Status_Init();
	Func_Init();
	MissionInfoManager_Init();
	Mission_Init();
	Item_Init();
	SkillEntity_Init();
	StatusEntity_Init();
	StatusInfoManager_Init();
	SkillPool_Init();
	MapInfoManager_Init();
	MapPool_Init();
	BusinessInfoManager_Init();
	DropTableManager_Init();
	GlobalMissionManager_Init();
	AccountPool_Init();
	ConnectionPool_Init();
	BloodInfoManager_Init();
	BoxInfoManager_Init();
	AwardInfoManager_Init();
	DesignationInfoManager_Init();
	VIPInfoManager_Init();
	FashionInfoManager_Init();
	GodShipInfoManager_Init();
	PlayOffManager_Init();
	GMPool_Init();
	Web_Init();
	ScribeClient_Init();
	FactionPool_InitSociaty();
	TransformInfoManager_Init();
	GCAgent_Init();
	GmSet_Init();
}

static void PrepareGameCore() {
	ProfessionInfoManager_Prepare();
	PlayerPool_Prepare();
	SkillPool_Prepare();
	NPCPool_Prepare();
	MissionInfoManager_Prepare();
	MapInfoManager_Prepare();
	MapPool_Prepare();
	StatusEntity_Prepare();
	ConnectionPool_Prepare();
	ScribeClient_Prepare();
}

static void ExitGameCore() {
	PlayerPool_Finalize();
}

static void InitNetAgent() {
	NetID_Init();
	NetAgent_Init(GCAgent_Context());
}

static void * RunNetAgent(void *ud) {
	InitNetAgent();

	while (!SignalHandler_IsOver()) {
		try{
			NetAgent_ProcessAlways();
		} catch(const zmq::error_t &) {
		}
	}

	return NULL;
}

static void InitDC() {
	Timer_Init(TIMER_THREAD_DC);
	DCAgent_Init(GCAgent_Context());
	DataManager_Init();
}

static void * RunDC(void *ud) {
	InitDC();

	while (true) {
		int v = 0;
		try{
			// time_t begin = time(NULL);
			//DEBUG_LOGERROR("1");
			v++;
			DCAgent_Process(33);
			// time_t cur = time(NULL);
			// if (cur - begin > 5)
			// 	DEBUG_LOGRECORD("Timeout, DCAgent_Process, v: %d", (int)(cur - begin));

			// begin = time(NULL);
			//DEBUG_LOGERROR("2");
			v++;
			Web_Update();
			// cur = time(NULL);
			// if (cur - begin > 5)
			// 	DEBUG_LOGRECORD("Timeout, Web_Update, v: %d", (int)(cur - begin));

			// begin = time(NULL);
			//DEBUG_LOGERROR("3");
			v++;
			Timer_Update(TIMER_THREAD_DC);
			v++;
			//DEBUG_LOGERROR("4");
			// cur = time(NULL);
			// if (cur - begin > 5)
			// 	DEBUG_LOGRECORD("Timeout, Timer_Update, v: %d", (int)(cur - begin));
		}
		catch(...)
		{
			DEBUG_LOGERROR("run dc thread error->%d", v);
		}
	}

	return NULL;
}

static inline int Max(int a, int b) {
	return a > b ? a : b;
}

static void InitDaemon() {
	int pid = fork();
	if (pid > 0)
		exit(EXIT_SUCCESS);
	else if (pid < 0)
		exit(EXIT_FAILURE);

	setsid();

	pid = fork();
	if (pid > 0)
		exit(EXIT_SUCCESS);
	else if (pid < 0)
		exit(EXIT_FAILURE);

	for (int i = 0; i < NOFILE; i++)
		close(i);

	umask(0);
}

static bool IsRunning(const char *target) {
	char buf[1024];
	char *tempname = tmpnam(NULL);
	SNPRINTF1(buf, "ps -u `whoami` | grep %s > %s", target, tempname);
	system(buf);
	FILE *file = fopen(tempname, "r");
	bool ret = fgets(buf, sizeof(buf), file) != NULL && fgets(buf, sizeof(buf), file) != NULL;
	fclose(file);
	remove(tempname);
	return ret;
}

static const char * Self(const char *self) {
	if (self[0] == '.' && self[1] == '/')
		return self + 2;
	return self;
}

static bool CheckSingle(const char *self) {
	return IsRunning(Self(self));
}

static void ShowHelp(const char *self) {
	printf("Usage: %s [-h|--help]\n", self);
}

static bool ExecuteCommand(int argc, char *argv[]) {
	const char *self = Self(argv[0]);
	bool done = false;
	for (int i = 1; i < argc; i++) {
		done = true;
		const char *command = argv[i];
		if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0 || true) {
			ShowHelp(self);
			break;
		}
	}
	return done;
}

int main(int argc, char *argv[]) {
	if (ExecuteCommand(argc, argv))
		return 0;
	if (CheckSingle(argv[0]))
		return 0;
	SetupSignalHandlers();
	InitDaemon();
	InitGameCore();

	// DC thread.
	pthread_t dc;
	if (pthread_create(&dc, NULL, &RunDC, NULL) != 0) {
		DEBUG_LOGERROR("Failed to create thread");
		return EXIT_FAILURE;
	}

	// NetAgent thread.
	pthread_t netAgent;
	if (pthread_create(&netAgent, NULL, &RunNetAgent, NULL) != 0) {
		DEBUG_LOGERROR("Failed to create thread");
		return EXIT_FAILURE;
	}

	PrepareGameCore();
	DEBUG_LOGRECORD("Init over");

	while (!SignalHandler_IsOver()) {
		try {
			Time_Begin();
			Timer_Update(TIMER_THREAD_GC);
			GCAgent_Process(Max(33 - (int)Time_FrameTime(), 0));
		} catch(const zmq::error_t &) {
		}
	}

	ExitGameCore();
	DEBUG_LOG("FFFFFFFFFFFFFFFFFF");
	pthread_join(netAgent, NULL);
	DEBUG_LOG("FFFFFFFFFFFFFFFFFF");
	pthread_join(dc, NULL);
	DEBUG_LOG("FFFFFFFFFFFFFFFFFF");

	return 0;
}
