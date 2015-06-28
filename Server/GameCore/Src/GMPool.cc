#include "GMPool.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <set>
#include <cstddef>

using namespace std;

struct GMComp {
	bool operator()(const GMMessage& lhs, const GMMessage& rhs) {
		return lhs.id < rhs.id;
	}
};

static struct{
	set<GMMessage, GMComp> pool;
}package;

void GMPool_Init() {
	package.pool.clear();
}

void GMPool_Add(int32_t id, string account) {
	GMMessage gm;
	gm.id = id;
	gm.account = account;
	//gm.flag = true;
	package.pool.insert(gm);
}

void GMPool_Del(int32_t id) {
	GMMessage gm;
	gm.id = id;
	package.pool.erase(gm);
}

bool GMPool_Has(int32_t id) {
	GMMessage gm;
	gm.id = id;
	return package.pool.find(gm) != package.pool.end();
}

int GMPool_GMs(int32_t *gms, size_t size) {
	if (gms == NULL || size <= 0)
		return -1;

	int count = 0;
	for (set<GMMessage, GMComp>::iterator it = package.pool.begin(); it != package.pool.end(); it++) {
		if (count >= (int)size)
			return -1;
//		if (it->flag) {
			gms[count++] = it->id;
//		}
	}

	return count;
}

bool GMPool_HasShowChat(int32_t id) {
	GMMessage gm;
	gm.id = id;
	if (package.pool.find(gm) != package.pool.end()) {
	//	return package.pool.find(gm)->flag;
		return true;
	}
	return false;
}

void GMPool_SetChatFlag(int32_t id, bool flag) {
/*	
	GMMessage gm;
	gm.id = id;
	set<GMMessage, GMComp>::iterator it = package.pool.find(gm);
	if (package.pool.find(gm) != package.pool.end()) {
		gm.account = it->account;
		gm.flag = flag;
		package.pool.erase(it);
		package.pool.insert(gm);
		return;
	}
	*/
}

string GMPool_IDByAccountID(int32_t id) {
	GMMessage gm;
	gm.id = id;
	if (package.pool.find(gm) != package.pool.end()) {
		return package.pool.find(gm)->account;
	}
	return "";

}

int32_t GMPool_AccountIDByID(std::string account) {
	for (set<GMMessage, GMComp>::iterator iter = package.pool.begin(); iter != package.pool.end(); ++iter) {
		if (iter->account == account) {
			return iter->id;
		}
	}
	return -1;
}



