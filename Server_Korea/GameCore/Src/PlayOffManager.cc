#include "PlayOffManager.hpp"
#include "PlayOff.pb.h"
#include "Debug.hpp"
#include "Config.hpp"
#include "Time.hpp"
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>
#include <ctime>

using namespace std;

static struct{
	AllPlayOff all;
}package;

void PlayOffManager_Init() {
	string src = Config_DataPath() + string("/PlayOff.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.all.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	/*
	PlayOff *info = package.all.mutable_playoff(0);
	info->set_time(0, 1413356100);
	*/
}

const PlayOff * PlayOffManager_PlayOff(int32_t id) {
	if (id < 0 || id >= package.all.playoff_size())
		return NULL;

	const PlayOff *info = &package.all.playoff(id);
	return info->id() == -1 ? NULL : info;
}

int PlayOffManager_TotalPass(int begin, int over) {
	if (begin == 2)
		begin = 4;
	int n = 0;
	while (over < begin) {
		n++;
		over *= 2;
	}
	return n;
}

int PlayOffManager_Count(int over) {
	if (over == 2)
		return 3;
	else if (over == 1)
		return 3;
	else
		return 1;
}

bool PlayOffManager_BeginAndEnd(int32_t id, int day, int pass, int *begin, int *end) {
	const PlayOff *info = PlayOffManager_PlayOff(id);
	if (info == NULL)
		return false;
	if (day < 0 || day >= info->time_size())
		return false;

	int n = day == 0 ? info->limit() : info->over(day - 1);
	for (int i = 0; i < pass; i++)
		n /= 2;
	if (n <= 0)
		return false;

	*begin = n;
	*end = n / 2;
	return true;
}

int PlayOffManager_CurData(int32_t id, int *day, int *pass, int *turn, int *overTime) {
	const PlayOff *info = PlayOffManager_PlayOff(id);
	if (info == NULL)
		return -1;

	int playofftime = Config_PlayOffTime() / 1000;
	int playoffinterval = Config_PlayOffInterval() / 1000;
	int playoffwait = Config_PlayOffWait() / 1000;

	int cur = (int)Time_TimeStamp();
	for (int i = 0; i < info->time_size(); i++) {
		int h = info->time(i);
		if (cur < h)
			continue;
		int count = PlayOffManager_Count(info->over(i));
		int p = PlayOffManager_TotalPass(i == 0 ? info->limit() : info->over(i - 1), info->over(i));
		int delta = cur - h;
		for (int j = 0; j < p; j++) {
			for (int k = 0; k < count; k++) {
				if (j == 0 && k == 0) {
					if (delta >= 0
							&& delta < j * (playofftime + playoffinterval + playoffwait) * count + playofftime + playoffinterval) {
						*day = i;
						*pass = j;
						*turn = k;
						*overTime = h + j * (playofftime + playoffinterval + playoffwait) * count + playofftime + playoffinterval;
						if (delta >= j * (playofftime + playoffinterval + playoffwait) * count
								&& delta < j * (playofftime + playoffinterval + playoffwait) * count + playoffinterval)
							return 0;
						else
							return 1;
					}
				} else {
					if (delta >= j * (playofftime + playoffinterval + playoffwait) * count + (playofftime + playoffinterval + playoffwait) * (k - 1) + (playofftime + playoffinterval)
							&& delta < j * (playofftime + playoffinterval + playoffwait) * count + (playofftime + playoffinterval + playoffwait) * k + (playofftime + playoffinterval)) {
						*day = i;
						*pass = j;
						*turn = k;
						*overTime = h + j * (playofftime + playoffinterval + playoffwait) * count + (playofftime + playoffinterval + playoffwait) * k + (playofftime + playoffinterval);
						if (delta >= j * (playofftime + playoffinterval + playoffwait) * count + (playofftime + playoffinterval + playoffwait) * k
								&& delta < j * (playofftime + playoffinterval + playoffwait) * count + (playofftime + playoffinterval + playoffwait) * k + playoffinterval)
							return 0;
						else
							return 1;
					}
				}
			}
		}
	}
	return -1;
}

int PlayOffManager_NextIndex(int playoff, int day, int pass, bool win) {
	const PlayOff *info = PlayOffManager_PlayOff(playoff);
	int index = 0;
	for (int i = 0; i <= day; i++) {
		for (int j = i == 0 ? info->limit() : info->over(i - 1), p = 0; j > info->over(i); j /= 2, p++) {
			if (i == day) {
				if (p <= pass) {
					index++;
					if (info->over(day) == 2
							&& j == 4
							&& win)
						index++;
				}
			} else {
				index++;
			}
		}
	}
	return index;
}

int PlayOffManager_CurIndex(int playoff, int day, int pass) {
	const PlayOff *info = PlayOffManager_PlayOff(playoff);
	int index = 0;
	for (int i = 0; i <= day; i++) {
		if (info->over(i) == 1) {
			if (pass == 0)
				index += 1;
			else if (pass == 1)
				index += 0;
		} else {
			for (int j = i == 0 ? info->limit() : info->over(i - 1), p = 0; j > info->over(i); j /= 2, p++) {
				if (i == day) {
					if (p < pass) {
						index++;
					}
				} else {
					index++;
				}
			}
		}
	}
	return index;
}


