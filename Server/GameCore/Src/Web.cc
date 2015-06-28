#include "Web.hpp"
#include "Config.hpp"
#include "DCAgent.hpp"
#include "DataManager.hpp"
#include "NetID.hpp"
#include "DCProto.pb.h"
#include "PlayerInfo.pb.h"
#include "MathUtil.hpp"
#include "HMAC_SHA1.h"
#include <curl/curl.h>
#include <sys/types.h>
#include <cassert>
#include <vector>
#include <string>
#include <map>
#include <MD5.hpp>

using namespace std;

#define CONNECT_TIMEOUT 10
#define TIMEOUT 10

struct Unit{
	CURL *curl;
	bool done;
	string buf;
	DCProto_Login login;
	DCProto_Recharge recharge;
	DCProto_Cost cost;
};

static struct{
	CURLM *mcurl;
	CHMAC_SHA1 HMAC_SHA1;
	map<void *, struct Unit *> ud;
}package;

static void URL_Encoding(const char *src, char *out) {
	int j = 0;
	for (int i = 0; src[i] != '\0'; i++) {
		unsigned char c = src[i];
		if ((c >= '0' && c <= '9')
				|| (c >= 'a' && c <= 'z')
				|| (c >= 'A' && c <= 'Z')
				|| c == '_'
				|| c == '-'
				|| c == '~'
				|| c == '.') {
			out[j++] = c;
		} else {
			sprintf(&out[j], "%%%X", c);
			while (out[++j] != '\0');
		}
	}
	out[j] = '\0';
}

/*
   static void Tencent_Encoding(const char *src, char *out) {
   int j = 0;
   for (int i = 0; src[i] != '\0'; i++) {
   unsigned char c = src[i];
   if ((c >= '0' && c <= '9')
   || (c >= 'a' && c <= 'z')
   || (c >= 'A' && c <= 'Z')
   || c == '!'
   || c == '*'
   || c == '('
   || c == ')') {
   out[j++] = c;
   } else {
   SNPRINTF1(&out[j], "%%%X", c);
   while (out[++j] != '\0');
   }
   }
   out[j] = '\0';
   }
   */

static inline void Tencent_AddArgToTable(map<string, string> *table, const char *k, const char *v) {
	(*table)[k] = v;
}
static inline void Tencent_AddArgToTable(map<string, string> *table, const char *k, int v) {
	char temp[CONFIG_FIXEDARRAY];
	SNPRINTF1(temp, "%d", v);
	(*table)[k] = temp;
}

static void Tencent_ProcessArg(map<string, string> *table) {
	for (map<string, string>::iterator it = table->begin(); it != table->end(); it++) {
		static char temp[CONFIG_FIXEDARRAY];
		// Tencent_Encoding(it->second.c_str(), temp);
		URL_Encoding(it->second.c_str(), temp);
		it->second = temp;
	}
}

static const char * Tencent_StrArg(map<string, string> *table) {
	static string buf;
	buf.clear();
	for (map<string, string>::iterator it = table->begin(); it != table->end(); it++) {
		if (it == table->begin()) {
			buf += it->first + "=" + it->second;
		} else {
			buf += "&" + it->first + "=" + it->second;
		}
	}
	return buf.c_str();
}

void Web_Init() {
	package.mcurl = curl_multi_init();
	assert(package.mcurl != NULL);
	package.ud.clear();

	static char url[CONFIG_FIXEDARRAY];
	URL_Encoding("/v3/pay/buy_goods", url);
	DEBUG_LOG("url: %s", url);

	/*
	const char *key = "12345f9a47df4d1eaeb3bad9a7e54321&";

	map<string, string> table;
	Tencent_AddArgToTable(&table, "amt", "4");
	Tencent_AddArgToTable(&table, "appid", "600");
	Tencent_AddArgToTable(&table, "appmode", "1");
	Tencent_AddArgToTable(&table, "format", "json");
	Tencent_AddArgToTable(&table, "goodsmeta", "道具*测试描述信息！！！");
	Tencent_AddArgToTable(&table, "goodsurl", "http://qzonestyle.gtimg.cn/qzonestyle/act/qzone_app_img/app613_613_75.png");
	Tencent_AddArgToTable(&table, "openid", "0000000000000000000000000E111111");
	Tencent_AddArgToTable(&table, "openkey", "1111806DC5D1C52150CF405E42222222");
	Tencent_AddArgToTable(&table, "payitem", "50005*4*1");
	Tencent_AddArgToTable(&table, "pf", "qzone");
	Tencent_AddArgToTable(&table, "pfkey", "1B59A5C3D77C7C56D7AFC3E2C823105D");
	Tencent_AddArgToTable(&table, "ts", "1333674935");
	Tencent_AddArgToTable(&table, "zoneid", "0");

	static char arg[CONFIG_FIXEDARRAY];
	snprintf(arg, sizeof(arg), "%s", Tencent_StrArg(&table));
	arg[sizeof(arg) - 1] = '\0';
	DEBUG_LOG("arg: %s", arg);
	static char temp[CONFIG_FIXEDARRAY];
	URL_Encoding(arg, temp);

	static char source[CONFIG_FIXEDARRAY];
	snprintf(source, sizeof(source), "GET&%s&%s", url, temp);
	source[sizeof(source) - 1] = '\0';
	DEBUG_LOG("source: %s", source);

	static char sig[CONFIG_FIXEDARRAY];
	package.HMAC_SHA1.HMAC_SHA1((BYTE *)source, strlen(source), (BYTE *)key, strlen(key), (BYTE *)temp);
	strcpy(sig, base64_encode((const unsigned char *)temp, strlen(temp)));
	DEBUG_LOG("sig: %s", sig);

	URL_Encoding(sig, temp);
	Tencent_ProcessArg(&table);
	static char data[CONFIG_FIXEDARRAY];
	snprintf(data, sizeof(data), "http://msdktest.qq.com/mpay/buy_goods_m?%s&sig=%s", Tencent_StrArg(&table), temp);
	data[sizeof(data) - 1] = '\0';
	DEBUG_LOG("final: %s", data);
	*/

	// const char *key = "56abfbcd12fe46f5ad85ad9f12345678&";
	// const char *source = "GET&%2Fv3%2Fpay%2Fpay&amt%3D10%26appid%3D15499%26openid%3D00000000000000000000000014BDF6E4%26openkey%3DAB43BF3DC5C3C79D358CC5318E41CF59%26payitem%3Did%2Aname%2Adesc%26pf%3Dqzone%26pfkey%3DCA641BC173479B8C0B35BC84873B3DB9%26ts%3D1340880299%26userip%3D112.90.139.30";
	/*
	   const char *key = "228bf094169a40a3bd188ba37ebe8723&";
	   const char *source = "GET&%2Fv3%2Fuser%2Fget_info&appid%3D123456%26format%3Djson%26openid%3D11111111111111111%26openkey%3D2222222222222222%26pf%3Dqzone%26userip%3D112.90.139.30";

	   char v1[1024 * 10];
	   CHMAC_SHA1 sha1;
	   sha1.HMAC_SHA1((BYTE *)source, strlen(source), (BYTE *)key, strlen(key), (BYTE *)v1);
	   DEBUG_LOG("after sha1, size: %d, v1: %s", strlen(v1), v1);
	   DEBUG_LOG("after base64: %s", base64_encode((unsigned char const *)v1, strlen(v1)));
	   */
}

static void DelUD(void *key) {
	map<void *, struct Unit *>::iterator it = package.ud.find(key);
	if (it != package.ud.end()) {
		delete it->second;
		package.ud.erase(it);
	}
}

static void ClearCurls(bool all) {
	static vector<CURLMsg> pool;
	pool.clear();
	int num;
	CURLMsg *msg = NULL;
	while ((msg = curl_multi_info_read(package.mcurl, &num)) != NULL)
		pool.push_back(*msg);
	for (size_t i = 0; i < pool.size(); i++) {
		msg = &pool[i];
		if (all) {
			curl_multi_remove_handle(package.mcurl, msg->easy_handle);
			curl_easy_cleanup(msg->easy_handle);
			// delete (struct Unit *)msg->data.whatever;
			DelUD(msg->easy_handle);
		} else {
			if (msg->msg == CURLMSG_DONE) {
				curl_multi_remove_handle(package.mcurl, msg->easy_handle);
				curl_easy_cleanup(msg->easy_handle);
				// delete (struct Unit *)msg->data.whatever;
				DelUD(msg->easy_handle);
			}
		}
	}
}

static size_t CB_CheckUser(void *ptr, size_t size, size_t nmemb, void *userdata) {
	size_t total = size * nmemb;
	if (userdata == NULL)
		return total;

	struct Unit *unit = (struct Unit *)userdata;
	if (unit->done)
		return total;

	if (total > 0)
		unit->buf.append((const char *)ptr, total);

	DEBUG_LOG("CB_CheckUser: %s", unit->buf.c_str());

	int status = 0;
	static char v[CONFIG_FIXEDARRAY];
	if (ConfigUtil_ReadStrFromJson(unit->buf.c_str(), "ret", v, sizeof(v))) {
		if (atoi(v) == 0) {
			status = 1;
		} else {
			status = -1;
		}
		unit->done = true;
	}

	char addTime[32] = {'\0'};
	char deviceAddTime[32] = {'\0'};
	if (status == 0) {
		// data is not completed
	} else {
		static DCProto_Login login;
		login.Clear();
		login = unit->login;

		if (status == 1) {
			PlayerInfo info;
			PlayerEntity_LoginToPlayerInfo(&unit->login.login(), &info);
			int res = DataManager_AddAccount(&info, unit->login.ip().c_str(), unit->login.login().activateKey().c_str(), addTime, deviceAddTime, login.beyond());
			unit->login.mutable_login()->set_addTime(addTime);
			unit->login.mutable_login()->set_deviceAddTime(deviceAddTime);
			info.set_addTime(addTime);
			info.set_deviceAddTime(deviceAddTime);
			//if (res == 0 && !login.beyond()) {
			if (res == 0)
			{
				if(!login.beyond())
				{
					static DCProto_AddAccount addAccount;
					addAccount.Clear();
					addAccount.set_id(unit->login.id());
					addAccount.set_res(true);
					*addAccount.mutable_info() = info;
					addAccount.set_ip(unit->login.ip());
					DCAgent_SendProtoToGCAgent(&addAccount);
				}
				login.set_useActivateKey(true);
				login.set_newUser(true);
			} else if (res == 1) {
				DataManager_UpdateAccount(&info, unit->login.ip().c_str());
			}
			login.set_res(res);
		} else if (status == -1) {
			login.set_res(-4);
		}

		DCAgent_SendProtoToGCAgent(&login);
		DEBUG_LOG("send DCProto_Login to GC");
		}

		return total;
	}

	void Web_CheckUser(const DCProto_Login *login) {
		int channel = Config_ChannelNum(login->login().platform().c_str());
		if (channel == -1) {
			DEBUG_LOGERROR("Failed to checkuser, platform: %s", login->login().platform().c_str());
			return;
		}

		struct Unit *unit = new struct Unit;
		unit->done = false;
		unit->login = *login;
		unit->curl = curl_easy_init();

		if (login->login().platform() == "QQ" || login->login().platform() == "WX") {
			int stamp = (int)Time_TimeStamp();
			static char sig[CONFIG_FIXEDARRAY];
			SNPRINTF1(sig, "%s%d", Config_AppKey(login->login().platform().c_str()), stamp);

			static char url[CONFIG_FIXEDARRAY];
			SNPRINTF1(url, "http://msdktest.qq.com/auth/verify_login/?timestamp=%d&appid=%s&sig=%s&openid=%s&encode=1", stamp, Config_AppID(login->login().platform().c_str()), md5(sig).c_str(), login->login().account().c_str());
			curl_easy_setopt(unit->curl, CURLOPT_URL, url);

			static char param[CONFIG_FIXEDARRAY];
			if (login->login().platform() == "QQ") {
				const char *ip = NetID_IP(login->id());
				if (ip == NULL)
					ip = "";
				SNPRINTF1(param, "{\"appid\":%s,\"openid\":\"%s\",\"openkey\":\"%s\",\"userip\":\"%s\"}", Config_AppID(login->login().platform().c_str()), login->login().account().c_str(), login->login().password().c_str(), ip);
			} else {
				SNPRINTF1(param, "{\"appid\":\"%s\",\"refreshToken\":\"%s\"}", Config_AppID(login->login().platform().c_str()), login->login().password().c_str());
			}
			curl_easy_setopt(unit->curl, CURLOPT_POST, 1);
			curl_easy_setopt(unit->curl, CURLOPT_POSTFIELDS, param);
		} else {
			static char buf[CONFIG_FIXEDARRAY];
			strcpy(buf, "http://message.rekoo.net/validate/login");
			curl_easy_setopt(unit->curl, CURLOPT_URL, buf);
			DEBUG_LOG("url: %s", buf);

			static map<string, string> argTable;
			argTable.clear();
			Tencent_AddArgToTable(&argTable, "channel", channel);
			Tencent_AddArgToTable(&argTable, "gamekey", Config_GameKey());
			Tencent_AddArgToTable(&argTable, "gid", Config_GID());
			Tencent_AddArgToTable(&argTable, "time", (int)Time_TimeStamp());
			Tencent_AddArgToTable(&argTable, "token", login->login().password().c_str());
			Tencent_AddArgToTable(&argTable, "uid", login->login().account().c_str());

			char str[CONFIG_FIXEDARRAY];
			snprintf(str, sizeof(str), "%s", Tencent_StrArg(&argTable));
			str[sizeof(str) - 1] = '\0';

			Tencent_ProcessArg(&argTable);
			argTable.erase(argTable.find("gamekey"));
			snprintf(buf, sizeof(buf), "%s&sign=%s", Tencent_StrArg(&argTable), md5(str).c_str());
			DEBUG_LOG("postdata: %s", buf);

			curl_easy_setopt(unit->curl, CURLOPT_POST, 1);
			curl_easy_setopt(unit->curl, CURLOPT_POSTFIELDS, buf);
		}

		curl_easy_setopt(unit->curl, CURLOPT_WRITEFUNCTION, CB_CheckUser);
		curl_easy_setopt(unit->curl, CURLOPT_WRITEDATA, unit);
		curl_easy_setopt(unit->curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
		curl_easy_setopt(unit->curl, CURLOPT_TIMEOUT, TIMEOUT);
		curl_easy_setopt(unit->curl, CURLOPT_NOSIGNAL, (long)1);
		curl_multi_add_handle(package.mcurl, unit->curl);

		package.ud[unit->curl] = unit;

		int num;
		curl_multi_perform(package.mcurl, &num);
	}

	static size_t CB_CheckRecharge(void *ptr, size_t size, size_t nmemb, void *userdata) {
		size_t total = size * nmemb;
		if (userdata == NULL)
			return total;

		struct Unit *unit = (struct Unit *)userdata;
		if (unit->done)
			return total;

		if (total > 0)
			unit->buf.append((const char *)ptr, total);

		DEBUG_LOG("CB_CheckRecharge: %s", unit->buf.c_str());

		int status = 0;
		if (unit->recharge.info().platform() == "appstore") {
			static char v[CONFIG_FIXEDARRAY];
			if (ConfigUtil_ReadStrFromJson(unit->buf.c_str(), "status", v, sizeof(v))) {
				if (atoi(v) == 0) {
					status = 1;
				} else {
					status = -1;
				}
				unit->done = true;
			}
		} else if (unit->recharge.info().platform() == "QQ"
				|| unit->recharge.info().platform() == "WX") {
			static char v[CONFIG_FIXEDARRAY];
			if (ConfigUtil_ReadStrFromJson(unit->buf.c_str(), "ret", v, sizeof(v))) {
				if (atoi(v) == 0) {
					status = 1;
				} else {
					status = -1;
				}
				unit->done = true;
			}
		}

		if (status == 0) {
			// data is not completed
		} else {
			if (status == 1) {
				if (unit->recharge.info().platform() == "appstore") {
					static const char *product[] = {
						"com.yulongxuan.ts1",
						"com.yulongxuan.ts2",
						"com.yulongxuan.ts3",
						"com.yulongxuan.ts4",
						"com.yulongxuan.ts5",
						"com.yulongxuan.ts6"
					};
					static int value[] = {
						6,
						30,
						50,
						108,
						328,
						648
					};
					size_t size = sizeof(product) / sizeof(product[0]);

					int v = -1;
					for (size_t i = 0; i < size; i++) {
						if (unit->recharge.recharge().product() == product[i]) {
							v = value[i];
							break;
						}
					}
					unit->recharge.set_rmb(v);
					DCAgent_SendProtoToGCAgent(&unit->recharge);
					DEBUG_LOG("Successful to recharge: %s, order size: %u, order: %s", unit->buf.c_str(), unit->recharge.recharge().order().size(), unit->recharge.recharge().order().c_str());
				} else if (unit->recharge.info().platform() == "QQ"
						|| unit->recharge.info().platform() == "WX") {
					static char v[CONFIG_FIXEDARRAY];
					if (ConfigUtil_ReadStrFromJson(unit->buf.c_str(), "balance", v, sizeof(v))) {
						int balance = atoi(v);
						if (balance > 0) {
							unit->recharge.set_rmb(balance);
							DCAgent_SendProtoToGCAgent(&unit->recharge);
							DEBUG_LOG("Successful to recharge, balance: %d", balance);
						}
					} else {
						unit->done = false;
						return total;
					}
				}
			} else if (status == -1) {
				DEBUG_LOGERROR("Failed to recharge: %s, order size: %u, order: %s", unit->buf.c_str(), unit->recharge.recharge().order().size(), unit->recharge.recharge().order().c_str());
				unit->recharge.set_rmb(-1);
				DCAgent_SendProtoToGCAgent(&unit->recharge);
			}
		}

		return total;
	}

	int Web_CheckRecharge(const DCProto_Recharge *proto) {
		if (proto == NULL)
			return -1;

		struct Unit *unit = NULL;
		if (proto->info().platform() == "appstore") {
			unit = new struct Unit;
			unit->done = false;
			unit->recharge = *proto;;
			unit->curl = curl_easy_init();

			static char buf[CONFIG_FIXEDARRAY * 10];
			strcpy(buf, "https://buy.itunes.apple.com/verifyReceipt");
			// strcpy(buf, "https://sandbox.itunes.apple.com/verifyReceipt");
			curl_easy_setopt(unit->curl, CURLOPT_URL, buf);

			strcpy(buf, proto->recharge().order().c_str());
			const char *v = base64_encode((unsigned char *)buf, strlen(buf));
			SNPRINTF1(buf, "{\"receipt-data\":\"%s\"}", v);
			// DEBUG_LOG("prev order: %s, product: %s, after order: %s", proto->recharge().order().c_str(), proto->recharge().product().c_str(), buf);

			curl_easy_setopt(unit->curl, CURLOPT_POST, 1);
			curl_easy_setopt(unit->curl, CURLOPT_POSTFIELDS, buf);

			curl_easy_setopt(unit->curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(unit->curl, CURLOPT_SSL_VERIFYHOST, 0);
		} else if (proto->info().platform() == "QQ"
				|| proto->info().platform() == "WX") {
			unit = new struct Unit;
			unit->done = false;
			unit->recharge = *proto;;
			unit->curl = curl_easy_init();

			static char key[CONFIG_FIXEDARRAY];
			snprintf(key, sizeof(key), "%s&", Config_AppKey(proto->info().platform().c_str()));
			key[sizeof(key) - 1] = '\0';
			DEBUG_LOG("key: %s", key);

			static char url[CONFIG_FIXEDARRAY];
			URL_Encoding("/mpay/get_balance_m", url);
			DEBUG_LOG("url: %s", url);

			static map<string, string> argTable;
			argTable.clear();
			Tencent_AddArgToTable(&argTable, "appid", Config_AppID(proto->info().platform().c_str()));
			Tencent_AddArgToTable(&argTable, "openid", proto->info().openid().c_str());
			Tencent_AddArgToTable(&argTable, "openkey", proto->info().openkey().c_str());
			Tencent_AddArgToTable(&argTable, "pf", proto->info().pf().c_str());
			Tencent_AddArgToTable(&argTable, "pfkey", proto->info().pfkey().c_str());
			Tencent_AddArgToTable(&argTable, "ts", (int)Time_TimeStamp());
			Tencent_AddArgToTable(&argTable, "zoneid", "1");
			Tencent_AddArgToTable(&argTable, "format", "json");
			Tencent_AddArgToTable(&argTable, "pay_token", proto->info().pay_token().c_str());

			static char arg[CONFIG_FIXEDARRAY];
			snprintf(arg, sizeof(arg), "%s", Tencent_StrArg(&argTable));
			arg[sizeof(arg) - 1] = '\0';
			DEBUG_LOG("arg: %s", arg);
			static char temp[CONFIG_FIXEDARRAY];
			URL_Encoding(arg, temp);

			static char source[CONFIG_FIXEDARRAY];
			snprintf(source, sizeof(source), "GET&%s&%s", url, temp);
			source[sizeof(source) - 1] = '\0';
			DEBUG_LOG("source: %s", source);

			static char sig[CONFIG_FIXEDARRAY];
			package.HMAC_SHA1.HMAC_SHA1((BYTE *)source, strlen(source), (BYTE *)key, strlen(key), (BYTE *)temp);
			strcpy(sig, base64_encode((const unsigned char *)temp, strlen(temp)));
			DEBUG_LOG("sig: %s", sig);

			URL_Encoding(sig, temp);
			Tencent_ProcessArg(&argTable);
			static char data[CONFIG_FIXEDARRAY];
			snprintf(data, sizeof(data), "http://msdktest.qq.com/mpay/get_balance_m?%s&sig=%s", Tencent_StrArg(&argTable), temp);
			data[sizeof(data) - 1] = '\0';
			DEBUG_LOG("final: %s", data);
			curl_easy_setopt(unit->curl, CURLOPT_URL, data);

			static char cookie[CONFIG_FIXEDARRAY];
			snprintf(cookie, sizeof(cookie), "session_id=%s;session_type=%s;org_loc=%s", proto->info().session_id().c_str(), proto->info().session_type().c_str(), url);
			cookie[sizeof(cookie) - 1] = '\0';
			DEBUG_LOG("cookie: %s", cookie);
			curl_easy_setopt(unit->curl, CURLOPT_COOKIE, cookie);
		} else {
			return 1;
		}

		curl_easy_setopt(unit->curl, CURLOPT_WRITEFUNCTION, CB_CheckRecharge);
		curl_easy_setopt(unit->curl, CURLOPT_WRITEDATA, unit);
		curl_easy_setopt(unit->curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
		curl_easy_setopt(unit->curl, CURLOPT_TIMEOUT, TIMEOUT);
		curl_easy_setopt(unit->curl, CURLOPT_NOSIGNAL, (long)1);
		curl_multi_add_handle(package.mcurl, unit->curl);

		package.ud[unit->curl] = unit;

		int num;
		curl_multi_perform(package.mcurl, &num);
		return 0;
	}

	static size_t CB_CheckCost(void *ptr, size_t size, size_t nmemb, void *userdata) {
		size_t total = size * nmemb;
		if (userdata == NULL)
			return total;

		struct Unit *unit = (struct Unit *)userdata;
		if (unit->done)
			return total;

		if (total > 0)
			unit->buf.append((const char *)ptr, total);

		DEBUG_LOG("CB_CheckCost: %s", unit->buf.c_str());

		int status = 0;
		if (unit->recharge.info().platform() == "QQ"
				|| unit->recharge.info().platform() == "WX") {
			static char v[CONFIG_FIXEDARRAY];
			if (ConfigUtil_ReadStrFromJson(unit->buf.c_str(), "ret", v, sizeof(v))) {
				if (atoi(v) == 0) {
					status = 1;
				} else {
					status = -1;
				}
				unit->done = true;
			}
		}

		if (status == 0) {
			// data is not completed
		} else {
			if (status == 1) {
				// nothing
			} else if (status == -1) {
				DEBUG_LOGERROR("Failed to cost: %s", unit->buf.c_str());
			}
		}

		return total;
	}

	void Web_CheckCost(const DCProto_Cost *proto) {
		if (proto == NULL)
			return;

		struct Unit *unit = NULL;
		if (proto->info().platform() == "QQ"
				|| proto->info().platform() == "WX") {
			unit = new struct Unit;
			unit->done = false;
			unit->cost = *proto;;
			unit->curl = curl_easy_init();

			static char key[CONFIG_FIXEDARRAY];
			snprintf(key, sizeof(key), "%s&", Config_AppKey(proto->info().platform().c_str()));
			key[sizeof(key) - 1] = '\0';
			DEBUG_LOG("key: %s", key);

			static char url[CONFIG_FIXEDARRAY];
			URL_Encoding("/mpay/pay_m", url);
			DEBUG_LOG("url: %s", url);

			static map<string, string> argTable;
			argTable.clear();
			Tencent_AddArgToTable(&argTable, "appid", Config_AppID(proto->info().platform().c_str()));
			Tencent_AddArgToTable(&argTable, "openid", proto->info().openid().c_str());
			Tencent_AddArgToTable(&argTable, "openkey", proto->info().openkey().c_str());
			Tencent_AddArgToTable(&argTable, "pf", proto->info().pf().c_str());
			Tencent_AddArgToTable(&argTable, "pfkey", proto->info().pfkey().c_str());
			Tencent_AddArgToTable(&argTable, "ts", (int)Time_TimeStamp());
			Tencent_AddArgToTable(&argTable, "zoneid", "1");
			Tencent_AddArgToTable(&argTable, "format", "json");
			Tencent_AddArgToTable(&argTable, "pay_token", proto->info().pay_token().c_str());
			Tencent_AddArgToTable(&argTable, "amt", (int)proto->v());

			static char arg[CONFIG_FIXEDARRAY];
			snprintf(arg, sizeof(arg), "%s", Tencent_StrArg(&argTable));
			arg[sizeof(arg) - 1] = '\0';
			DEBUG_LOG("arg: %s", arg);
			static char temp[CONFIG_FIXEDARRAY];
			URL_Encoding(arg, temp);

			static char source[CONFIG_FIXEDARRAY];
			snprintf(source, sizeof(source), "GET&%s&%s", url, temp);
			source[sizeof(source) - 1] = '\0';
			DEBUG_LOG("source: %s", source);

			static char sig[CONFIG_FIXEDARRAY];
			package.HMAC_SHA1.HMAC_SHA1((BYTE *)source, strlen(source), (BYTE *)key, strlen(key), (BYTE *)temp);
			strcpy(sig, base64_encode((const unsigned char *)temp, strlen(temp)));
			DEBUG_LOG("sig: %s", sig);

			URL_Encoding(sig, temp);
			Tencent_ProcessArg(&argTable);
			static char data[CONFIG_FIXEDARRAY];
			snprintf(data, sizeof(data), "http://msdktest.qq.com/mpay/pay_m?%s&sig=%s", Tencent_StrArg(&argTable), temp);
			data[sizeof(data) - 1] = '\0';
			DEBUG_LOG("final: %s", data);
			curl_easy_setopt(unit->curl, CURLOPT_URL, data);

			static char cookie[CONFIG_FIXEDARRAY];
			snprintf(cookie, sizeof(cookie), "session_id=%s;session_type=%s;org_loc=%s", proto->info().session_id().c_str(), proto->info().session_type().c_str(), url);
			cookie[sizeof(cookie) - 1] = '\0';
			DEBUG_LOG("cookie: %s", cookie);
			curl_easy_setopt(unit->curl, CURLOPT_COOKIE, cookie);
		} else {
			return;
		}

		curl_easy_setopt(unit->curl, CURLOPT_WRITEFUNCTION, CB_CheckCost);
		curl_easy_setopt(unit->curl, CURLOPT_WRITEDATA, unit);
		curl_easy_setopt(unit->curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
		curl_easy_setopt(unit->curl, CURLOPT_TIMEOUT, TIMEOUT);
		curl_easy_setopt(unit->curl, CURLOPT_NOSIGNAL, (long)1);
		curl_multi_add_handle(package.mcurl, unit->curl);

		package.ud[unit->curl] = unit;

		int num;
		curl_multi_perform(package.mcurl, &num);
	}

	void Web_Update() {
		int num;
		curl_multi_perform(package.mcurl, &num);
		ClearCurls(false);
	}
