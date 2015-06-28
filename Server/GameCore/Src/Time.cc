#include "Time.hpp"
#include "Config.hpp"
#include "FactionPool.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdlib>
#include <time.h>
#include <cmath>

/*
static struct{
	timeval init;
	timeval begin;
	int32_t delta;
}package;

void Time_Init() {
	srand(Time_TimeStamp());

	gettimeofday(&package.init, NULL);
	gettimeofday(&package.begin, NULL);
	package.delta = 0;
}

void Time_Begin() {
	timeval now;
	gettimeofday(&now, NULL);

	package.delta = (int32_t)((now.tv_sec - package.begin.tv_sec) * 1000 + (now.tv_usec - package.begin.tv_usec) / 1000);
	package.begin = now;
}

int32_t Time_DeltaTime() {
	return package.delta;
}

int32_t Time_FrameTime() {
	timeval now;
	gettimeofday(&now, NULL);

	return (int32_t)((now.tv_sec - package.begin.tv_sec) * 1000 + (now.tv_usec - package.begin.tv_usec) / 1000);
}

int64_t Time_ElapsedTime() {
	timeval now;
	gettimeofday(&now, NULL);

	return (now.tv_sec - package.init.tv_sec) * 1000 + (now.tv_usec - package.init.tv_usec) / 1000;
}

int Time_Random(int begin, int end) {
	return begin + random() % (end - begin);
}

bool Time_CanPass(float rate) {
	int base = 0xFFFF;
	int v = Time_Random(0, base);
	return v < (base * rate);
}
*/

#define MAX_COUNT 50

static struct{
	timespec init;
	timespec begin;
	int32_t delta;
	int count;
	int64_t elapsedTime;
	time_t stamp;
	pthread_mutex_t mutex;
	int hourDelta;
}package;

void Time_Init() {
	srand(Time_TimeStamp());

	clock_gettime(CLOCK_MONOTONIC, &package.init);
	clock_gettime(CLOCK_MONOTONIC, &package.begin);
	package.delta = 0;
	package.count = MAX_COUNT;
	package.elapsedTime = 0;
	package.stamp = time(NULL);

	time_t base = 3600 * 12; 
	tm m1 = *gmtime(&base);
	int h1 = m1.tm_hour > 0 ? m1.tm_hour : m1.tm_mday * 24; 
	tm m2 = *localtime(&base);
	int h2 = m2.tm_hour > 0 ? m2.tm_hour : m2.tm_mday * 24; 
	package.hourDelta = h2 - h1;

	int res = pthread_mutex_init(&package.mutex, NULL);
	assert(res == 0);
}

static void DoFlush() {
	package.count = 0;
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	package.elapsedTime = (int64_t)((now.tv_sec - package.init.tv_sec) * 1000 + (now.tv_nsec - package.init.tv_nsec) / 1000000);
}

void Time_Flush() {
	pthread_mutex_lock(&package.mutex);
	DoFlush();
	pthread_mutex_unlock(&package.mutex);
}

void Time_Begin() {
	pthread_mutex_lock(&package.mutex);
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	package.delta = (int32_t)((now.tv_sec - package.begin.tv_sec) * 1000 + (now.tv_nsec - package.begin.tv_nsec) / 1000000);
	package.begin = now;
	package.count = MAX_COUNT;
	package.elapsedTime = 0;

	package.stamp = time(NULL);
	pthread_mutex_unlock(&package.mutex);
}

int32_t Time_DeltaTime() {
	return package.delta;
}

int32_t Time_FrameTime() {
	pthread_mutex_lock(&package.mutex);
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	int32_t ret = (int32_t)((now.tv_sec - package.begin.tv_sec) * 1000 + (now.tv_nsec - package.begin.tv_nsec) / 1000000);
	pthread_mutex_unlock(&package.mutex);
	return ret;
}

int64_t Time_ElapsedTime() {
	pthread_mutex_lock(&package.mutex);
	int64_t ret = 0;
	if (++package.count < MAX_COUNT) {
		ret  = package.elapsedTime;
	} else {
		DoFlush();
		ret = package.elapsedTime;
	}
	pthread_mutex_unlock(&package.mutex);
	return ret;
}

time_t Time_TimeStamp() {
	pthread_mutex_lock(&package.mutex);
	time_t ret = package.stamp;
	pthread_mutex_unlock(&package.mutex);
	return ret;
}

void Time_LocalTime(time_t t, tm *m) {
	if (m == NULL)
		return;
	pthread_mutex_lock(&package.mutex);
	*m = *localtime(&t);
	pthread_mutex_unlock(&package.mutex);
}

int Time_Random(int begin, int end) {
	if (begin == end)
		return begin;
	return begin + random() % (end - begin);
}

bool Time_CanPass(float rate) {
	if (rate >= 1.0f)
		return true;
	int base = 0xFFFF;
	int v = Time_Random(0, base);
	return v < (base * rate);
}

int Time_RandomSelect(const int *rate, size_t size) {
	if (rate == NULL || size <= 0)
		return 0;

	int pool[CONFIG_FIXEDARRAY];
	if (size > CONFIG_FIXEDARRAY)
		return 0;

	for (size_t i = 0; i < size; i++)
		pool[i] = (i <= 0 ? rate[i] : pool[i - 1] + rate[i]);

	int final = Time_Random(0, pool[size - 1]);
	for (size_t i = 0; i < size; i++)
		if (final < pool[i])
			return i;

	return 0;
}

int Time_WeekDay() {
	time_t t = Time_TimeStamp();
	struct tm local;
   	Time_LocalTime(t, &local);
	int ret = local.tm_wday - 1;
	return ret > 0 ? ret : 6;
}

bool Time_PassHour(time_t prevTime, time_t curTime, int h) {
	if (h == 0)
		return Time_DayDelta(prevTime, curTime) > 0;
	tm prev;
   	Time_LocalTime(prevTime, &prev);
	tm cur;
	Time_LocalTime(curTime, &cur);
	if (prev.tm_year != cur.tm_year)
		return true;
	if (prev.tm_yday + 1 < cur.tm_yday)
		return true;
	if (prev.tm_yday + 1 == cur.tm_yday) {
		if (prev.tm_hour < h)
			return true;
		if (cur.tm_hour >= h)
			return true;
	}
	else if (prev.tm_yday == cur.tm_yday) {
		if (prev.tm_hour < h && cur.tm_hour >= h)
			return true;
	}
	return false;
}

bool Time_WithinHour(time_t cur, int begin, int end) {
	tm curTM;
	Time_LocalTime(cur, &curTM);

	/*
	if (begin > end) {
		int value = begin;
		begin = end;
		end = value;
	
		if (curTM->tm_hour >= begin && curTM->tm_hour < end) {
			return false;
		}
		return true;
	}

	if (curTM->tm_hour >= begin && curTM->tm_hour < end) {
		return true;
	}
	return false;
	*/

	if (end > begin) {
		return curTM.tm_hour >= begin && curTM.tm_hour < end;
	} else {
		return !(curTM.tm_hour >= end && curTM.tm_hour < begin);
	}
}

static inline time_t ToLocalTime(time_t src) {
	if (package.hourDelta < 0)
		return src - ((-package.hourDelta) * 3600);
	else
		return src + package.hourDelta * 3600;
}

int Time_WeekDelta(time_t prevTime, time_t curTime) {
	int d1 = (int)(ToLocalTime(prevTime) / (3600 * 24));
	int d2 = (int)(ToLocalTime(curTime) / (3600 * 24));
	if (d2 - d1 >= 7)
		return (d2 - d1) / 7;
	tm prev;
	Time_LocalTime(prevTime, &prev);
	tm cur;
	Time_LocalTime(curTime, &cur);
	int w1 = prev.tm_wday == 0 ? 6 : prev.tm_wday - 1;
	int w2 = cur.tm_wday == 0 ? 6 : cur.tm_wday - 1;
	return w2 < w1 ? 1 : 0;
	// return (int)(ToLocalTime(curTime) / (60 * 10)) - (int)(ToLocalTime(prevTime) / (60 * 10));
}

int Time_DayDelta(time_t prev, time_t cur) {
	int d1 = (int)(ToLocalTime(prev) / (3600 * 24));
	int d2 = (int)(ToLocalTime(cur) / (3600 * 24));
	return d2 - d1;
	// return (int)(ToLocalTime(cur) / (60 * 10)) - (int)(ToLocalTime(prev) / (60 * 10));
}

int Time_TMToTimeNull(const char *str) {
	struct tm TM;
	int value = 0;
	assert(strlen(str) == 19);
	
	for (int i = 0;  i < 4; ++i) {
		value = value * 10 + (str[i] - 48);	
	}
	TM.tm_year = value - 1900;

	value = 0;
	for (int i = 5;  i < 7; ++i) {
		value = value * 10 + (str[i] - 48);	
	}
	TM.tm_mon = value - 1;

	value = 0;
	for (int i = 8;  i < 10; ++i) {
		value = value * 10 + (str[i] - 48);	
	}
	TM.tm_mday = value;

	value = 0;
	for (int i = 11;  i < 13; ++i) {
		value = value * 10 + (str[i] - 48);	
	}
	TM.tm_hour = value;

	value = 0;
	for (int i = 14;  i < 16; ++i) {
		value = value * 10 + (str[i] - 48);	
	}
	TM.tm_min = value;
	
	value = 0;
	for (int i = 17;  i < 19; ++i) {
		value = value * 10 + (str[i] - 48);	
	}
	TM.tm_sec = value;

	DEBUG_LOG("active:%d-%d-%d %d:%d:%d, %d", TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday, TM.tm_hour, TM.tm_min, TM.tm_sec, mktime(&TM));
	return mktime(&TM);
}

bool Time_TheSameDay(int32_t preTime, int32_t curTime) {
	time_t preTime_t = preTime;
	time_t curTime_t = curTime;
	struct tm cur;
	Time_LocalTime(preTime_t, &cur);
	tm preTM = cur;
	Time_LocalTime(curTime_t, &cur);

	if (preTM.tm_year == cur.tm_year && preTM.tm_mon == cur.tm_mon && preTM.tm_mday == cur.tm_mday) {
		return true;
	}
	return false;
}


bool Time_Interval(time_t cur, int beginYear, int beginMonth, int beginDay, int beginHour, int beginMin, int beginSec, int endYear, int endMonth, int endDay, int endHour, int endMin, int endSec) {
	struct tm tmBegin;
	struct tm tmEnd;
	tmBegin.tm_year=beginYear - 1900;
	tmBegin.tm_mon=beginMonth - 1;
	tmBegin.tm_mday=beginDay;
	tmBegin.tm_hour=beginHour;
	tmBegin.tm_min=beginMin;
	tmBegin.tm_sec=beginSec;
	tmEnd.tm_year=endYear - 1900;
	tmEnd.tm_mon=endMonth - 1;
	tmEnd.tm_mday=endDay;
	tmEnd.tm_hour=endHour;
	tmEnd.tm_min=endMin;
	tmEnd.tm_sec=endSec;
	pthread_mutex_lock(&package.mutex);
	time_t beginTime = mktime(&tmBegin);
	time_t endTime = mktime(&tmEnd);
	pthread_mutex_unlock(&package.mutex);
	return beginTime <= cur && cur < endTime;
}

void Time_ActiveEndSendAward(int id) {
	if (id == 3) {
		static vector<struct PlayerEntity *> all;
		all.clear();
		int count = PlayerPool_Players(&all);
		for (int i = 0; i < count; i++) {
			PlayerEntity_ActiveStrongeSolider(all[i]);
		}
	}else if(id == 8) {
		PlayerPool_SendActiveAward(NetProto_Rank::BLESSCOME, true);
	}else if (id == 13) {
		PlayerPool_SendDevilAward(true);
	}else if (id == 16) {
		PlayerPool_SendActiveAward(NetProto_Rank::QIUBITE, true);
	}else if (id == 20) {
		PlayerPool_SendActiveAward(NetProto_Rank::LUCK, true);
	}else if (id == 22) {
	}else if (id == 23) {
		PlayerPool_SendActiveAward(NetProto_Rank::CONSUME, true);
	}else if (id == 29) {
		PlayerPool_SendActiveAward(NetProto_Rank::GROUP_PURCHASE, true);
	}
}


