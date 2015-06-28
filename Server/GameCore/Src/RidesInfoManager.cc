#include "RidesInfoManager.hpp"
#include "RidesInfo.hpp"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <cassert>
#include <string>

using namespace std;

static struct
{
	AllRideses pool;
}package;

void RidesInfoManager_Init() 
{
	{

		string src = Config_DataPath() + string("/Rides.bytes");
		fstream in(src.c_str(), ios_base::in | ios_base::binary);
		if (in.fail()) {
			DEBUG_LOGERROR("Failed to open %s", src.c_str());
			exit(EXIT_FAILURE);
		}

		if (!package.pool.ParseFromIstream(&in)) {
			DEBUG_LOGERROR("Failed to parse %s", src.c_str());
			exit(EXIT_FAILURE);
		}
	}
}

const RidesInfo * RidesInfoManager_RidesInfo(int32_t id, int32_t star)
{
	if (id < 0 || id >= package.pool.rides_size())
		return NULL;

	RidesGroup rg = package.pool.rides(id);
	if(star < 0 || star >= (int32_t)rg.rides_size())
		return NULL;

	return &rg.rides(star);
}
