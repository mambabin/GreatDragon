#include "GoodsInfoManager.hpp"
#include "Debug.hpp"
#include "ItemInfo.hpp"
#include "ItemInfo.hpp"
#include "Time.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

static struct{
	AllGoods pool;
	int size;
	vector<vector<int32_t> > gems;
	int maxGemLevel;

	AllExchangeTable exchangeTable;
}package;

void GoodsInfoManager_Init() {
	{
		string src = Config_DataPath() + string("/Goods.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.pool.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		package.size = package.pool.goods_size();
		package.maxGemLevel = 0;

		for (int i = 0; i < package.pool.goods_size(); i++) {
			const GoodsInfo *goods = &package.pool.goods(i);
			if (goods->id() == -1)
				continue;

			if (goods->type() == GoodsInfo::GEM) {
				if ((int)package.gems.size() <= goods->arg(3))
					package.gems.resize(goods->arg(3) + 1);
				package.gems[goods->arg(3)].push_back(goods->id());
				if (goods->arg(3) > package.maxGemLevel)
					package.maxGemLevel = goods->arg(3);
			}
		}
	}
	{
		string src = Config_DataPath() + string("/ExchangeTable.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.exchangeTable.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
}

const GoodsInfo * GoodsInfoManager_GoodsInfo(int32_t id) {
	// MUST BE THREAD SAFETY
	if (id < 0 || id >= package.size)
		return NULL;

	if (package.pool.goods(id).id() == -1)
		return NULL;
	return &package.pool.goods(id);
}

bool GoodsInfoManager_IsGem(int32_t id) {
	const GoodsInfo *info = GoodsInfoManager_GoodsInfo(id);
	return info != NULL && info->type() == GoodsInfo::GEM;
}

int32_t GoodsInfoManager_RandomGem(int level) {
	if (level < 0 || level >= (int)package.gems.size())
		return -1;
	if (package.gems[level].size() <= 0)
		return -1;
	return package.gems[level][Time_Random(0, (int)package.gems[level].size())];
}

int32_t GoodsInfoManager_Gem(int level, int type) {
	if (level < 0 || level >= (int)package.gems.size())
		return -1;
	for (size_t i = 0; i < package.gems[level].size(); i++) {
		int32_t id = package.gems[level][i];
		const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(id);
		assert(goods != NULL);
		if (goods->arg(0) == type)
			return id;
	}
	return -1;
}

int GoodsInfoManager_MaxGemLevel() {
	return package.maxGemLevel;
}

const ExchangeTable * GoodsInfoManager_ExchangeTable(int index) {
	if (index < 0 || index >= package.exchangeTable.exchangeTable_size())
		return NULL;
	return &package.exchangeTable.exchangeTable(index);
}
