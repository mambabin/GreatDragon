#include "DesignationInfoManager.hpp"
#include "DesignationInfo.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllDesignationInfo allDesignationes;
}package;

void DesignationInfoManager_Init() {
	string src = Config_DataPath() + string("/Designation.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.allDesignationes.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
}

const DesignationInfo * DesignationInfoManager_DesignationInfo(int32_t id) {
	if (id < 0 || id >= package.allDesignationes.designationInfo_size())
		return NULL;

	const DesignationInfo *info = &package.allDesignationes.designationInfo(id);
	return info->id() == -1 ? NULL : info;
}
