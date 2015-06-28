#include "TransformInfoManager.hpp"
#include "TransformInfo.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>

using namespace std;

static struct{
	// id.quality.level
	vector< vector< vector<TransformInfo> > > transforms;
}package;

void TransformInfoManager_Init() {
	string src = Config_DataPath() + string("/Transforms.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	AllTransforms allTransforms;
	if (!allTransforms.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < allTransforms.transforms_size(); i++) {
		const TransformInfo *unit = &allTransforms.transforms(i);
		if (unit->id() == -1)
			continue;

		if (unit->id() >= (int)package.transforms.size())
			package.transforms.resize(unit->id() + 1);

		vector< vector<TransformInfo> > *family = &package.transforms[unit->id()];
		if (unit->quality() >= (int)family->size())
			family->resize(unit->quality() + 1);

		vector<TransformInfo> *quality = &(*family)[unit->quality()];
		if (unit->level() >= (int)quality->size()) {
			TransformInfo temp;
			temp.set_id(-1);
			quality->resize(unit->level() + 1, temp);
		}
		(*quality)[unit->level()] = *unit;
	}
}

const TransformInfo * TransformInfoManager_TransformInfo(int32_t id, int quality, int level) {
	if (id < 0 || id >= (int)package.transforms.size())
		return NULL;

	vector< vector<TransformInfo> > *family = &package.transforms[id];
	if (quality < 0 || quality >= (int)family->size())
		return NULL;

	vector<TransformInfo> *quality_ = &(*family)[quality];
	if (level < 0 || level >= (int)quality_->size())
		return NULL;

	const TransformInfo *info = &(*quality_)[level];
	return info->id() == -1 ? NULL : info;
}
