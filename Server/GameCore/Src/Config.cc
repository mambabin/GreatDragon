#include "Config.hpp"
#include "ConfigUtil.hpp"
#include "MapPool.hpp"
#include "VIPInfoManager.hpp"
#include "NetProto.pb.h"
#include "EquipmentInfo.hpp"
#include "Debug.hpp"
#include "ActiveInfo.pb.h"
#include "Award.pb.h"
#include <sys/types.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include <fstream>

using namespace std;

struct ChannelData {
	int num;
	int rechargeNum;
	int rmbBase;
	string biName;
	string platform;
	string url;
};

static struct{
	pthread_mutex_t mutex;
	string dataPath;
	int netAgentPort;
	int64_t version;
	int line;
	set<string> platforms;
	string dbHost;
	int dbPort;
	string db;
	string dbUser;
	string dbPassword;
	string dbPublicHost;
	int dbPublicPort;
	string dbPublic;
	string dbPublicUser;
	string dbPublicPassword;
	string dbRechargeHost;
	int dbRechargePort;
	string dbRecharge;
	string dbRechargeUser;
	string dbRechargePassword;
	string rechargeTable;
	string accountTable;
	string roleTable;
	string singleTable;
	string giftTable;
	string costRecordTable;
	string chatTable;
	string gmTable;
	string rekooRoleTable;
	string gmDataTable;
	string factionTable;
	string deviceTable;
	string activateKeyTable;
	string scribeHost;
	int scribePort;
	string scribeCategory;
	string snapshotDir;
	bool logInfo;
	bool logError;
	bool logRecord;
	int maxPlayers;
	int maxNPCs;
	int maxComponents;
	int maxSkills;
	int maxStatuses;
	int maxMaps;
	vector<string> gmAccount;
	vector<string> gmPasswd;
	int serverStartTime;
	int mulExp;
	multimap<int, int64_t> fixedEventMap;
	vector<vector<MailGift> > mailGift;
	string push;
	string pushDir;
	int openTime;
	char openTimeStr[32];
	vector<string> whiteList;
	vector<string> names;
	map<int, pair<int, int> > mulTower;
	map<string, struct ChannelData> channelData;
	int queuePlayers;
	vector<SnapshotData> snapshots;
}config;

static const char *words[] = {"系统邮件", // 0
	"系统", // 1
	"[bb1fe2]%s[-]将%s%s[-]强化到了%s%d[-]级，神兵利器再现江湖", // 2
	"[bb1fe2]%s[-]解锁了第[60dd62]%d[-]层血脉，领悟了新的境界[60dd62]%s[-]", // 3
	"等级榜", // 4
	"战力榜", // 5
	"声望榜", // 6
	"通天榜", // 7
	"修罗榜", // 8
	"竞技榜", // 9
	"富豪榜", // 10
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，吞食天地指日可待", // 11
	"[bb1fe2]%s[-]通关了[60dd62]%s[-]副本，获得了%s%s[-]", // 12
	"[bb1fe2]%s[-]在幸运轮中抽奖获得了%s%s[-]", // 13
	"[bb1fe2]%s[-]打开了%s%s[-]，获得了%s%s[-]", // 14
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，试问天下谁堪敌手", // 15
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，我命由我不由天", // 16
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，傲然立绝顶，铁血染征衣", // 17
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，莫愁无知己，谁人不识君", // 18
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，不愧是珠屑铺街金粉砌殿的土豪", // 19
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，不要迷恋哥，哥只是传说", // 20
	"[bb1fe2]%s[-]正在竞技场寻找对手，邀请诸位豪杰青梅煮酒论英雄", // 21
	"[bb1fe2]%s[-]正在寻找扫荡多人副本[60dd62]%s[-]的队伍，小伙伴们快来一起玩耍", // 22
	"[bb1fe2]%s[-]正在组织挑战英雄志[60dd62]%s[-]的队伍，有志同道合的英雄快去加入吧", // 23
	"由于包裹已满，此物品通过邮件发送", // 24
	"活动期间，每日活动额外奖励[60dd62]%d[-]倍资源", // 25
	"活动期间[bb1fe2]%s[-]每天限购[60dd62]1[-]次", // 26
	"[bb1fe2]欢迎来到天煞的世界，各位玩家如遇到任何问题，可加入官方QQ群120147551，客服会为您详细解答[-]", // 27
	"[bb1fe2]为回馈广大玩家厚爱，天煞团队特开启以下活动：1、活动期间活动副本奖励翻倍；2、推出特价礼包，每天限购一个；3、部分商品折扣幅度再度提升[-]", // 28
	"[bb1fe2][60dd62]充值任意金额[-]可获得[60dd62]超炫光武一把[-]，[60dd62]累计充值3000元[-]可升级至VIP9并获得[60dd62]全套橙装[-]，其中[60dd62]衣服同样带有超炫特效[-][-]", // 29
	"恭喜[bb1fe2]%s[-]夺取[60dd62]%s[-]第[60dd62]%d[-]名，封神领域，无往不利", // 30
	"封神榜", // 31
	"未开启", // 32
	"已结束", // 33
	"世界BOSS奖励", // 34
	"喵大人：世界boss又出现了，一定不能让它逃跑了", // 35
	"恭喜[bb1fe2]%s[-]完成了对世界BOSS的最后一击", // 36
	"已售完", // 37
	"不能指定自己", //38
	"指定玩家粉丝已满", //39
	"角色已被冻结", //40
	"角色已被禁言", //41
	"队伍不存在", //42
	"此服务器不允许%s平台用户登录", //43
	"此服务器不允许%s平台用户注册", //44
	"[bb1fe2]%s[-]正在混战场寻找对手，邀请诸位豪杰青梅煮酒论英雄", // 45
	"指定玩家不在城镇", //46
	"指定玩家已有队伍", //47
	"敖兴：无限混战开始了，是时候证明谁是真正的英雄了", // 48
	"王校长：每日领取元宝时间又到了，看看这次咱们能不能十倍暴击呢？", //49
	"苍老师：我的葡萄酸甜可口，还可以补充体力哦，快来尝尝吧。", //50
	"服务器将在%s开启，敬请期待", // 51
	"未获得资格", // 52
	"已获得下一轮资格", // 53
	"元宝暂不可邮寄", // 54
	"恭喜[bb1fe2]%s[-]在%d进%d的比赛中获胜，获得了进入下一轮的资格", // 55
	"恭喜[bb1fe2]%s[-]在天煞精英争霸赛中完美发挥，获得了冠军", // 56
	"恭喜[bb1fe2]%s[-]在天煞精英争霸赛中完美发挥，获得了亚军", // 57
	"恭喜[bb1fe2]%s[-]在天煞精英争霸赛中完美发挥，获得了季军", // 58
	"已失去资格", // 59
	"争霸赛期间无法进行此项操作", // 60
	"争霸赛已开始，请速速进场", // 61
	"封神榜奖励", // 62
	"对方条件不足", //63
	"对方已取消", //64
	"帮派BOSS已开启", //65
	"请等待所有玩家进入", //66
	"雇佣次数已用完", //67
	"福星下凡奖励", // 68
	"不能使用非法字符%c", //69
	"等待对方确认", //70
	"客户端版本过低，请更新", //71
	"本次物品卖出共获得%d金币", //72
	"名字重复", //73
	"名字包含非法字符", //74
	"帮派战已开启", // 75
	"帮派战奖励", // 76
	"帮派战连续奖励", // 77
	"7天等级榜奖励", // 78
	"7天战力榜奖励", // 79
	"10级礼包", // 80
	"屠魔勇士奖励", // 81
	"白色情人节活动", //82
	"真封神活动", //83
	"天降福彩大礼包", // 84
	"丘比特金箭", // 85
	"春节送神兵", //86
	"春节送红包", // 87
	"返还500元宝", // 88
	"每日充值送活动", // 89
	"每日消费送活动", // 90
	"最强之剑奖励", // 91
	"天降鸿福奖励", // 92
	"服务器将于3月6日11点开启", // 93
	"10级才能开启世界聊天", // 94
	"土豪[bb1fe2]%s[-]送出了[bb1fe2]%d[-]份新春红包，快来抢哈！", // 95
	"你已经抢过红包！", // 96
	"你的手速太慢了，红包飞了！", // 97
	"哇塞！玩家[bb1fe2]%s[-]获得[bb1fe2]%s[-]", // 98
	"团购返还元宝", // 99
};

void Config_Init() {
	int res = pthread_mutex_init(&config.mutex, NULL);
	assert(res == 0);

	config.serverStartTime = Time_TimeStamp();
	const char *key = "Config.txt";
	FILE *file = fopen(key, "r");
	if (file == NULL) {
		DEBUG_LOGERROR("Failed to open config file: %s", key);
		exit(EXIT_FAILURE);
	}

	char value[1024];
	key = "logInfo";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.logInfo = atoi(value) != 0;

	key = "logError";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.logError = atoi(value) != 0;

	key = "logRecord";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.logRecord = atoi(value) != 0;

	key = "dataPath";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dataPath = value;

	key = "version";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.version = atoll(value);

	key = "line";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.line = atoi(value);

	config.platforms.clear();
	key = "platforms";
	if (ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		char *tokens[CONFIG_FIXEDARRAY];
		int count = ConfigUtil_ExtraToken(value, tokens, CONFIG_FIXEDARRAY, ", ");
		for (int i = 0; i < count; i++)
			config.platforms.insert(tokens[i]);
	}
	{
		string str = "Platform: ";
		if (config.platforms.empty()) {
			str += "All";
		} else {
			for (set<string>::iterator it = config.platforms.begin(); it != config.platforms.end(); it++) {
				str += *it;
				str += ", ";
			}
		}
		DEBUG_LOGRECORD("%s", str.c_str());
	}

	key = "netAgentPort";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.netAgentPort = atoi(value);

	key = "openTime";
	config.openTime = 0;
	config.openTimeStr[0] = '\0';
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.openTime = atoi(value);
	Time_ToDateTime(config.openTime + 60 * 30, config.openTimeStr);

	key = "dbHost";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbHost = value;

	key = "dbPort";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbPort = atoi(value);

	key = "db";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.db = value;

	key = "dbPublicHost";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbPublicHost = value;

	key = "dbPublicPort";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbPublicPort = atoi(value);

	key = "dbPublic";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbPublic = value;

	key = "dbRechargeHost";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbRechargeHost = value;

	key = "dbRechargePort";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbRechargePort = atoi(value);

	key = "dbRecharge";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbRecharge = value;

	key = "dbRechargeUser";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbRechargeUser = value;

	key = "dbRechargePassword";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbRechargePassword = value;


	key = "rechargeTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.rechargeTable = value;

	key = "accountTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.accountTable = value;

	key = "roleTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.roleTable = value;

	key = "singleTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.singleTable = value;

	key = "giftTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.giftTable = value;

	key = "costRecordTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.costRecordTable = value;

	key = "chatTable";
	if (ConfigUtil_ReadStr(file, key, value, sizeof(value)))
		config.chatTable = value;
	else
		config.chatTable.clear();
	if (config.chatTable.empty())
		DEBUG_LOGRECORD("No chatTable");

	key = "gmTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.gmTable = value;

	key = "rekooRoleTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.rekooRoleTable = value;

	key = "gmDataTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.gmDataTable = value;

	key = "deviceTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.deviceTable = value;

	key = "activateKeyTable";
	config.activateKeyTable.clear();
	if (ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		config.activateKeyTable = value;
	}
	if (config.activateKeyTable.empty())
		DEBUG_LOGRECORD("No activateKeyTable");

	key = "factionTable";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.factionTable = value;

	key = "dbUser";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbUser = value;

	key = "dbPassword";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbPassword = value;

	key = "dbPublicUser";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbPublicUser = value;

	key = "dbPublicPassword";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.dbPublicPassword = value;

	key = "scribeHost";
	if (ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		config.scribeHost = value;

		key = "scribePort";
		if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
			DEBUG_LOGERROR("Failed to read config field: %s", key);
			exit(EXIT_FAILURE);
		}
		config.scribePort = atoi(value);
	} else {
		config.scribeHost.clear();
	}
	if (config.scribeHost.empty())
		DEBUG_LOGRECORD("No scribeHost");

	key = "scribeCategory";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.scribeCategory = value;


	key = "gmAccount";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	{
		char *tokens[CONFIG_FIXEDARRAY];
		int count = ConfigUtil_ExtraToken(value, tokens, CONFIG_FIXEDARRAY, ", ");
		for (int i = 0; i < count; i++)
			config.gmAccount.push_back(tokens[i]);
	}
	key = "gmPasswd";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	{
		char *tokens[CONFIG_FIXEDARRAY];
		int count = ConfigUtil_ExtraToken(value, tokens, CONFIG_FIXEDARRAY, ", ");
		for (int i = 0; i < count; i++)
			config.gmPasswd.push_back(tokens[i]);
	}

	key = "maxPlayers";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.maxPlayers = atoi(value);

	key = "maxMaps";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.maxMaps = atoi(value);

	key = "push";
	config.push.clear();
	config.pushDir.clear();
	if (ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		for (char *i = value + strlen(value) - 1; i != value; i--) {
			if (*i == '/') {
				for (char *j = i + 1; *j != '\0'; j++)
					config.push += *j;
				for (char *j = value; j != i; j++)
					config.pushDir += *j;
				break;
			}
		}
		if (config.push.empty())
			config.push = value;
		if (config.pushDir.empty())
			config.pushDir = ".";
	}
	if (config.pushDir.empty())
		DEBUG_LOGRECORD("No push");

	fclose(file);
	Config_Reload(NULL);
	Config_ReloadSnapshotData();

	{
		AllActives allActives;
		string activePath = Config_DataPath() + string("/Active.bytes");
		fstream in(activePath.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", activePath.c_str());
			exit(EXIT_FAILURE);
		}

		if (!allActives.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", activePath.c_str());
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < allActives.allActives_size(); ++i) {
			if (allActives.allActives(i).id() == -1)
				continue;
			for (int j = 0; j < allActives.allActives(i).beginTime_size(); ++j) {
				int64_t begin = allActives.allActives(i).beginTime(j);
				int64_t end = allActives.allActives(i).endTime(j);
				if (begin == 0) {
					begin = Config_OpenTime(NULL);
					end += begin;
					//-----------------------
					time_t cur = end;
					tm date;
					Time_LocalTime(cur, &date);
					date.tm_hour = 0;
					date.tm_min = 0;
					date.tm_sec = 0;
					time_t next = mktime(&date);
					end = next;
					//------------------------------------
				}

				int64_t value = (begin << 32) + end;
				config.fixedEventMap.insert(pair<int, int64_t>(allActives.allActives(i).id(), value));
			}
		}

		Config_ActiveTick(NULL);
		DEBUG_LOG("active size = %d", config.fixedEventMap.size());
	}

	{
		string activePath = Config_DataPath() + string("/MailGift.bytes");
		fstream in(activePath.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", activePath.c_str());
			exit(EXIT_FAILURE);
		}

		AllMailGift all;
		if (!all.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", activePath.c_str());
			exit(EXIT_FAILURE);
		}

		config.mailGift.clear();
		for (int i = 0; i < all.mailGift_size(); i++) {
			const MailGift *unit = &all.mailGift(i);
			if (unit->type() >= (int)config.mailGift.size())
				config.mailGift.resize(unit->type() + 1);
			config.mailGift[unit->type()].push_back(*unit);
		}
	}

	{
		string path = Config_DataPath() + string("/WhiteList.txt");
		file = fopen(path.c_str(), "r");
		config.whiteList.clear();
		if (file != NULL) {
			while (fgets(value, sizeof(value), file) != NULL) {
				config.whiteList.push_back(value);
				string &str = config.whiteList.back();
				for (int i = 0; i < 2; i++) {
					if (str[str.length() - 1] == '\n'
							|| str[str.length() - 1] == '\r')
						str.erase(str.size() - 1);
				}
			}
			fclose(file);
		}
		if (config.whiteList.empty())
			DEBUG_LOGRECORD("No WhiteList");
	}

	{
		string path = Config_DataPath() + string("/Names.txt");
		file = fopen(path.c_str(), "r");
		config.names.clear();
		if (file == NULL) {
			DEBUG_LOGERROR("Failed to open %s", path.c_str());
			exit(EXIT_FAILURE);
		}
		while (fgets(value, sizeof(value), file) != NULL) {
			config.names.push_back(value);
			string &str = config.names.back();
			if (str[str.length() - 1] == '\n')
				str.erase(str.size() - 1);
		}
		fclose(file);
	}

	{
		string path = Config_DataPath() + string("/MulTower.txt");
		file = fopen(path.c_str(), "r");
		config.mulTower.clear();
		if (file != NULL) {
			for (int i = 0; ; i++) {
				char key[CONFIG_FIXEDARRAY];
				if (!ConfigUtil_Key(file, i, key, CONFIG_FIXEDARRAY))
					break;

				if (!ConfigUtil_ReadStr(file, key, value, CONFIG_FIXEDARRAY)) {
					DEBUG_LOGERROR("Failed to read key %s in %s", key, path.c_str());
					exit(EXIT_FAILURE);
				}

				char *tokens[CONFIG_FIXEDARRAY];
				int count = ConfigUtil_ExtraToken(value, tokens, CONFIG_FIXEDARRAY, ", ");
				assert(count == 2);
				config.mulTower.insert(make_pair(atoi(key), make_pair(atoi(tokens[0]), atoi(tokens[1]))));
			}
			fclose(file);
		}
		if (config.mulTower.empty())
			DEBUG_LOGRECORD("No MulTower");
	}

	config.mulExp = 1;
	config.maxNPCs = config.maxMaps * 50;
	config.maxSkills = config.maxNPCs * 10;
	config.maxStatuses = config.maxNPCs * 20;
	config.maxComponents = (config.maxPlayers + config.maxNPCs) * 20;
}

void Config_Reload(void *arg) {
	pthread_mutex_lock(&config.mutex);

	const char *key = "Config.txt";
	FILE *file = fopen(key, "r");
	if (file == NULL) {
		DEBUG_LOGERROR("Failed to open config file: %s", key);
		exit(EXIT_FAILURE);
	}
	Config_LockFile(file);

	char value[1024];
	key = "queuePlayers";
	if (!ConfigUtil_ReadStr(file, key, value, sizeof(value))) {
		DEBUG_LOGERROR("Failed to read config field: %s", key);
		exit(EXIT_FAILURE);
	}
	config.queuePlayers = atoi(value);

	Config_UnlockFile(file);
	fclose(file);

	{
		string path = Config_DataPath() + string("/ChannelData.txt");
		file = fopen(path.c_str(), "r");
		config.channelData.clear();

		if (file != NULL) {
			Config_LockFile(file);

			for (int i = 0; ; i++) {
				char key[CONFIG_FIXEDARRAY];
				if (!ConfigUtil_Key(file, i, key, CONFIG_FIXEDARRAY))
					break;

				if (!ConfigUtil_ReadStr(file, key, value, CONFIG_FIXEDARRAY)) {
					DEBUG_LOGERROR("Failed to read key %s in %s", key, path.c_str());
					exit(EXIT_FAILURE);
				}

				char *tokens[CONFIG_FIXEDARRAY];
				int count = ConfigUtil_ExtraToken(value, tokens, CONFIG_FIXEDARRAY, ", ");
				assert(count == 6);

				struct ChannelData unit;
				unit.num = atoi(tokens[0]);
				unit.rechargeNum = atoi(tokens[1]);
				unit.rmbBase = atoi(tokens[2]);
				unit.biName = tokens[3];
				unit.platform = tokens[4];
				unit.url = tokens[5];
				config.channelData[key] = unit;
			}

			Config_UnlockFile(file);
			fclose(file);
		}
		//		if (config.channelData.empty())
		//			DEBUG_LOGRECORD("No ChannelData");
	}

	pthread_mutex_unlock(&config.mutex);
}

void Config_ReloadSnapshotData() {
	FILE *file = fopen("Config.txt", "r");
	if (file == NULL) {
		DEBUG_LOGERROR("Failed to open config file: Config.txt");
		exit(EXIT_FAILURE);
	}
	Config_LockFile(file);

	char key[128];
	char value[1024];
	strcpy(key, "snapshotDir");
	if (ConfigUtil_ReadStr(file, key, value, sizeof(value)))
		config.snapshotDir = value;
	else
		config.snapshotDir.clear();
	if (config.snapshotDir.empty())
		DEBUG_LOGRECORD("No snapshotDir");

	config.snapshots.clear();

	for (int i = 0; ; i++) {
		sprintf(key, "snapshotData-%d", i + 1);
		if (!ConfigUtil_ReadStr(file, key, value, sizeof(value)))
			break;

		char *tokens[32];
		int count = ConfigUtil_ExtraToken(value, tokens, CONFIG_FIXEDARRAY, ", ");
		assert(count == 5);

		struct SnapshotData unit;
		unit.host = tokens[0];
		unit.port = atoi(tokens[1]);
		unit.user = tokens[2];
		unit.passwd = tokens[3];
		unit.db = tokens[4];
		config.snapshots.push_back(unit);
	}

	Config_UnlockFile(file);
	fclose(file);
}

const vector<struct SnapshotData> * Config_Snapshots() {
	return &config.snapshots;
}

const char * Config_DataPath() {
	return config.dataPath.c_str();
}

const char * Config_Words(int i) {
	return words[i];
}

int Config_NetAgentPort() {
	return config.netAgentPort;
}

int64_t Config_Version() {
	return config.version;
}

int Config_Line() {
	return config.line;
}

bool Config_HasPlatform(const char *platform) {
	if (config.platforms.empty()) {
		return true;
	} else {
		return config.platforms.find(platform) != config.platforms.end();
	}
}

const char * Config_DBHost() {
	return config.dbHost.c_str();;
}

int Config_DBPort() {
	return config.dbPort;
}

const char * Config_DB() {
	return config.db.c_str();
}

const char * Config_DBPublicHost() {
	return config.dbPublicHost.c_str();
}

int Config_DBPublicPort() {
	return config.dbPublicPort;
}

const char * Config_DBPublic() {
	return config.dbPublic.c_str();
}

const char * Config_DBRechargeHost() {
	return config.dbRechargeHost.c_str();
}

int Config_DBRechargePort() {
	return config.dbRechargePort;
}

const char * Config_DBRecharge(){
	return config.dbRecharge.c_str();
}

const char * Config_DBRechargeUser(){
	return config.dbRechargeUser.c_str();
}

const char * Config_DBRechargePassword() {
	return config.dbRechargePassword.c_str();
}

const char * Config_RechargeTable() {
	return config.rechargeTable.c_str();
}

const char * Config_AccountTable() {
	return config.accountTable.c_str();
}

const char * Config_RoleTable() {
	return config.roleTable.c_str();
}

const char * Config_DeviceTable() {
	return config.deviceTable.c_str();
}

const char * Config_GMTable() {
	return config.gmTable.c_str();
}

const char * Config_RekooRoleTable() {
	return config.rekooRoleTable.c_str();
}

const char * Config_GMDataTable() {
	return config.gmDataTable.c_str();
}

const char * Config_SingleTable() {
	return config.singleTable.c_str();
}

const char * Config_GiftTable() {
	return config.giftTable.c_str();
}

const char * Config_CostRecordTable() {
	return config.costRecordTable.c_str();
}

const char * Config_ChatTable() {
	return config.chatTable.empty() ? NULL : config.chatTable.c_str();
}

const char * Config_ActivateKeyTable() {
	return config.activateKeyTable.empty() ? NULL : config.activateKeyTable.c_str();
}

const char * Config_DBUser() {
	return config.dbUser.c_str();
}

const char * Config_DBPublicUser() {
	return config.dbPublicUser.c_str();
}

const char * Config_FactionTable() {
	return config.factionTable.c_str();
}

const char * Config_DBPassword() {
	return config.dbPassword.c_str();
}

const char * Config_DBPublicPassword() {
	return config.dbPublicPassword.c_str();
}

const char * Config_ScribeHost() {
	if (config.scribeHost.empty())
		return NULL;
	else
		return config.scribeHost.c_str();
}

int Config_ScribePort() {
	return config.scribePort;
}

const char * Config_ScribeCategory() {
	return config.scribeCategory.c_str();
}

const char * Config_SnapshotDir() {
	return config.snapshotDir.empty() ? NULL : config.snapshotDir.c_str();
}

const char * Config_Push() {
	return config.push.empty() ? NULL : config.push.c_str();
}

const char * Config_PushDir() {
	return config.pushDir.empty() ? NULL : config.pushDir.c_str();
}

bool Config_LogInfo() {
	return config.logInfo;
}

bool Config_LogError() {
	return config.logError;
}

bool Config_LogRecord() {
	return config.logRecord;
}

bool Config_IsGM(const char *account, const char *passwd) {
	for (size_t i = 0; i < config.gmAccount.size() && i < config.gmPasswd.size(); i++) {
		DEBUG_LOG("%s--%s", config.gmAccount[i].c_str(), config.gmPasswd[i].c_str());
		if (config.gmAccount[i] == account && config.gmPasswd[i] == passwd)
			return true;
	}
	return false;
}

int Config_OpenTime(char **str) {
	if (str != NULL)
		*str = config.openTimeStr;
	return config.openTime;
}

bool Config_InWhiteList(const char *account) {
	for (size_t i = 0; i < config.whiteList.size(); i++) {
		if (config.whiteList[i] == account)
			return true;
	}
	return false;
}

const char * Config_RandomName() {
	if (config.names.empty())
		return NULL;
	return config.names[Time_Random(0, (int)config.names.size())].c_str();
}

const vector<MailGift> * Config_MailGift(MailGift::Type type) {
	if (type < 0 || type >= (int)config.mailGift.size())
		return NULL;
	return &config.mailGift[type];
}

int Config_MaxPlayers() {
	return config.maxPlayers;
}

int Config_MaxNPCs() {
	return config.maxNPCs;
}

int Config_MaxComponents() {
	return config.maxComponents;
}

int Config_MaxSkills() {
	return config.maxSkills;
}

int Config_MaxStatuses() {
	return config.maxStatuses;
}

int Config_MaxMaps() {
	return config.maxMaps;
}

int Config_ServerStartTime() {
	return config.serverStartTime;
}

void Config_SetMulExp(int mul) {
	if (mul >= 1)
		config.mulExp = mul;
}

int Config_MulExp() {
	return config.mulExp;
}

const multimap<int, int64_t>* Config_fixedEventMap() {
	return &config.fixedEventMap;
}

bool Config_InActiveTime(int id) {
	multimap<int, int64_t>::iterator begin = config.fixedEventMap.find(-id);
	if (begin == config.fixedEventMap.end()) {
		return false;
	}
	return true;
}

bool Config_SameActiveInterval(int id, int preTime, int endTime) {
	multimap<int, int64_t>::iterator it = config.fixedEventMap.find(-id);
	if (it != config.fixedEventMap.end()) {
		int begin = (it->second & 0xFFFFFFFF00000000) >> 32;
		int end = it->second & 0xFFFFFFFF;
		if (begin <= preTime && preTime <= end && begin <= endTime && endTime <= end) {
			return true;
		}else {
			return false;
		}
	}
	return false;
}

int Config_MulTowerGroup(int room) {
	for (map<int, pair<int, int> >::iterator it = config.mulTower.begin(); it != config.mulTower.end(); it++) {
		int begin = it->second.first;
		int end = it->second.second;
		if (begin <= room && room <= end)
			return it->first;
	}
	return -1;
}

int Config_MulTowerType(int room) {
	for (map<int, pair<int, int> >::iterator it = config.mulTower.begin(); it != config.mulTower.end(); it++) {
		int begin = it->second.first;
		int end = it->second.second;
		if (begin == room)
			return 1;
		else if (end == room)
			return 2;
		else if (begin < room && room < end)
			return 0;
	}
	return -1;
}

const char * Config_ChannelName(int num) {
	pthread_mutex_lock(&config.mutex);
	const char *ret = NULL;
	for (map<string, struct ChannelData>::iterator it = config.channelData.begin(); it != config.channelData.end(); it++) {
		if (it->second.rechargeNum == num) {
			if (ret != NULL) {
				pthread_mutex_unlock(&config.mutex);
				return NULL;
			}
			ret = it->first.c_str();
		}    
	}    
	pthread_mutex_unlock(&config.mutex);
	return ret; 
}

int Config_ChannelNum(const char *channel) {
	pthread_mutex_lock(&config.mutex);
	int ret;
	map<string, struct ChannelData>::iterator it = config.channelData.find(channel);
	if (it == config.channelData.end())
		ret = -1;
	else
		ret = it->second.num;
	pthread_mutex_unlock(&config.mutex);
	return ret;
}

int Config_ChannelRechargeNum(const char *channel) {
	pthread_mutex_lock(&config.mutex);
	int ret;
	map<string, struct ChannelData>::iterator it = config.channelData.find(channel);
	if (it == config.channelData.end())
		ret = -1;
	else
		ret = it->second.rechargeNum;
	pthread_mutex_unlock(&config.mutex);
	return ret;
}

int Config_ChannelRMBBase(const char *channel) {
	pthread_mutex_lock(&config.mutex);
	int ret;
	map<string, struct ChannelData>::iterator it = config.channelData.find(channel);
	if (it == config.channelData.end())
		ret = 1;
	else
		ret = it->second.rmbBase;
	pthread_mutex_unlock(&config.mutex);
	return ret;
}

const char * Config_ChannelBIName(const char *channel) {
	pthread_mutex_lock(&config.mutex);
	const char *ret;
	map<string, struct ChannelData>::iterator it = config.channelData.find(channel);
	if (it == config.channelData.end())
		ret = NULL;
	else
		ret = it->second.biName.c_str();
	pthread_mutex_unlock(&config.mutex);
	return ret;
}

const char * Config_ChannelPlatform(const char *channel) {
	pthread_mutex_lock(&config.mutex);
	const char *ret;
	map<string, struct ChannelData>::iterator it = config.channelData.find(channel);
	if (it == config.channelData.end())
		ret = NULL;
	else
		ret = it->second.platform.c_str();
	pthread_mutex_unlock(&config.mutex);
	return ret;
}

const char * Config_ChannelURL(const char *channel) {
	if (channel == NULL)
		return NULL;
	pthread_mutex_lock(&config.mutex);
	const char * str = "null";
	map<string, struct ChannelData>::iterator it = config.channelData.find(channel);
	if (it == config.channelData.end()) {
		pthread_mutex_unlock(&config.mutex);
		return NULL;
	} else if (it->second.url == str) {
		pthread_mutex_unlock(&config.mutex);
		return NULL;
	} else { 
		pthread_mutex_unlock(&config.mutex);
		return it->second.url.c_str();
	}
}

int Config_MaxOnlinePlayers() {
	pthread_mutex_lock(&config.mutex);
	int ret = config.queuePlayers;
	pthread_mutex_unlock(&config.mutex);
	return ret;
}

void Config_ActiveTick(void *arg) {
	int activeTime = Time_TimeStamp();
	int size = 0;
	for (multimap<int, int64_t>::iterator it = config.fixedEventMap.begin(); it != config.fixedEventMap.end(); it++) {
		if (it->first > size)
			size = it->first;
	}

	for (int key = 1; key <= size; key++) {
		multimap<int, int64_t>::iterator it1 = config.fixedEventMap.lower_bound(key);
		multimap<int, int64_t>::iterator it2 = config.fixedEventMap.upper_bound(key);
		multimap<int, int64_t>::iterator it3 = config.fixedEventMap.find(-key);
		if (it1 != config.fixedEventMap.end()) {
			if (it3 == config.fixedEventMap.end()) {
				while (it1 != it2) {
					int64_t value = it1->second;
					if ((activeTime >= value >> 32) && (activeTime <= (value & 0x0FFFFFFFF))) {
						config.fixedEventMap.erase(it1);
						config.fixedEventMap.insert(pair<int, int64_t>(-key, value));
						break;
					}
					it1++;
				}
			}
		}

		it3 = config.fixedEventMap.find(-key);
		if (it3 != config.fixedEventMap.end()) {
			if (activeTime > (it3->second & 0x0FFFFFFFF)) {
				config.fixedEventMap.erase(it3);
				if (key == 3 || key == 8 || key == 13 || key == 16 || key == 20 || key == 23 || key == 29) {
					Time_ActiveEndSendAward(key);
				}
			}
		}
	}
	/*
	   for (multimap<int, int64_t>::iterator it = config.fixedEventMap.begin(); it != config.fixedEventMap.end();) {
	   if (it->first == 1) {
	   if (config.fixedEventMap.find(-1) == config.fixedEventMap.end()) {
	   int64_t value = it->second;
	   if ((activeTime >= value >> 32) && (activeTime <= (value & 0x0FFFFFFFF))) {
	   config.fixedEventMap.erase(it);
	   config.fixedEventMap.insert(pair<int, int64_t>(-1, value));
	   it = config.fixedEventMap.begin();
	   continue;
	   }
	   }
	   } else if (it->first == 2) {
	   if (config.fixedEventMap.find(-2) == config.fixedEventMap.end()) {
	   int64_t value = it->second;
	   if ((activeTime >= value >> 32) && (activeTime <= (value & 0x0FFFFFFFF))) {
	   config.fixedEventMap.erase(it);
	   config.fixedEventMap.insert(pair<int, int64_t>(-2, value));
	   it = config.fixedEventMap.begin();	
	   continue;
	   }
	   }
	   } else if (it->first == 3) {
	   if (config.fixedEventMap.find(-3) == config.fixedEventMap.end()) {
	   int64_t value = it->second;
	   if ((activeTime >= value >> 32) && (activeTime <= (value & 0x0FFFFFFFF))) {
	   config.fixedEventMap.erase(it);
	   config.fixedEventMap.insert(pair<int, int64_t>(-3, value));
	   it = config.fixedEventMap.begin();	
	   continue;
	   }
	   }
	   } else if (it->first == -1) {
	   if (activeTime > (it->second & 0x0FFFFFFFF)) {
	   config.fixedEventMap.erase(it++);
	   continue;
	   }
	   } else if (it->first == -2) {
	   if (activeTime > (it->second & 0x0FFFFFFFF)) {
	   config.fixedEventMap.erase(it++);
	   continue;
	   }
	   } else if (it->first == -3) {
	   if (activeTime > (it->second & 0x0FFFFFFFF)) {
	   config.fixedEventMap.erase(it++);

	   struct PlayerEntity *all[MAX_ONLINE_PLAYERS];
	   int count = PlayerPool_Players(all, MAX_ONLINE_PLAYERS);
	   for (int i = 0; i < count; i++) {
//int32_t target = PlayerEntity_ID(all[i]);
PlayerEntity_ActiveStrongeSolider(all[i]);
}
continue;
}
}
it++;
}
*/
}


