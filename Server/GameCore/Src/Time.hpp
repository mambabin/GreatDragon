#ifndef _TIME_HPP_
#define _TIME_HPP_

#include <sys/types.h>
#include <ctime>
#include <cstdio>

void Time_Init();

// thread safety
void Time_Begin();

void Time_Flush();

int32_t Time_DeltaTime();

// thread safety
int32_t Time_FrameTime();

// thread safety
int64_t Time_ElapsedTime();

// thread safety
time_t Time_TimeStamp();

// thread safety
void Time_LocalTime(time_t t, tm *m);

int Time_Random(int begin, int end);

bool Time_CanPass(float rate);

int Time_RandomSelect(const int *rate, size_t size);

int Time_WeekDay();
bool Time_PassHour(time_t prevTime, time_t curTime, int h);
bool Time_WithinHour(time_t cur, int begin, int end);
int Time_WeekDelta(time_t prevTime, time_t curTime);
int Time_DayDelta(time_t prev, time_t cur);
int Time_TMToTimeNull(const char *str);

bool Time_TheSameDay(int32_t preTime, int32_t curTime);
void Time_ActiveEndSendAward(int index);
// sizeof(s) >= 20
static inline void Time_ToDateTime(int t, char *s) {
	time_t c = (time_t)t;
	tm cur;
	Time_LocalTime(c, &cur);
	sprintf(s, "%04d-%02d-%02d %02d:%02d:%02d", cur.tm_year + 1900, cur.tm_mon + 1, cur.tm_mday, cur.tm_hour, cur.tm_min, cur.tm_sec);
}

bool Time_Interval(time_t cur, int beginYear, int beginMonth, int beginDay, int beginHour, int beginMin, int beginSec, int endYear, int endMonth, int endDay, int endHour, int endMin, int endSec);

#endif
