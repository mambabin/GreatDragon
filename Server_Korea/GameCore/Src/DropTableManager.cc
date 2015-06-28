#include "DropTableManager.hpp"
#include "ItemInfo.hpp"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllDropTables tables;
}pool;

void DropTableManager_Init() {
	string src = Config_DataPath() + string("/DropTables.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!pool.tables.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
}

const DropTable * DropTableManager_DropTable(int32_t id) {
	if (id < 0 || id >= (int32_t)pool.tables.dropTables_size())
		return NULL;
	if (pool.tables.dropTables(id).id() == -1)
		return NULL;
	return &pool.tables.dropTables(id);
}
