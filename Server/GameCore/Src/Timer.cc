#include "Timer.hpp"
#include "Time.hpp"
#include "Debug.hpp"
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctime>
#include <vector>
#include <list>
#include <string>

using namespace std;

#define TYPE_HOURS 0
#define TYPE_TIMET 1

struct Event{
	int type;
	Timer_Event event;
	void *arg;
	string name;
	int64_t process;

	int wday;
	vector<int> h;
	int lastH;

	time_t t;
	int interval;
	bool done;
};

static struct{
	list<struct Event> events[2];
}package;

void Timer_Init(int thread) {
	package.events[thread].clear();
}

void Timer_AddEvent(int wday, int h[], size_t size, Timer_Event event, void *arg, const char *name, int thread) {
	if (event == NULL || size <= 0)
		return;

	struct Event unit;
	unit.type = TYPE_HOURS;
	unit.event = event;
	unit.arg = arg;
	if (name != NULL)
		unit.name = name;
	unit.process = 0;
	unit.wday = wday;
	for (size_t i = 0; i < size; i++)
		unit.h.push_back(h[i]);
	unit.lastH = -1;
	package.events[thread].push_back(unit);
}

void Timer_AddEvent(time_t t, int interval, Timer_Event event, void *arg, const char *name, int thread) {
	if (event == NULL)
		return;
	if (t == 0)
		t = Time_TimeStamp();

	struct Event unit;
	unit.type = TYPE_TIMET;
	unit.event = event;
	unit.arg = arg;
	if (name != NULL)
		unit.name = name;
	unit.process = 0;
	unit.t = t;
	unit.interval = interval;
	unit.done = false;
	package.events[thread].push_back(unit);
}

static void RunEvent(struct Event *unit) {
	if (unit->name.empty()) {
		unit->event(unit->arg);
	} else {
		Time_Flush();
		int64_t begin = Time_ElapsedTime();
		unit->event(unit->arg);
		Time_Flush();
		int64_t process = Time_ElapsedTime() - begin;
		if (process > unit->process) {
			DEBUG_LOGRECORD("Event \"%s\" cost %lldms", unit->name.c_str(), (long long)process);
			unit->process = process;
		}
	}
}

void Timer_Update(int thread) {
	if (package.events[thread].empty())
		return;

	time_t cur = Time_TimeStamp();
	tm curTM;
	Time_LocalTime(cur, &curTM);

	for (list<struct Event>::iterator unit = package.events[thread].begin(); unit != package.events[thread].end(); ) {
		if (unit->type == TYPE_HOURS) {
			if (unit->wday == -1 || unit->wday == curTM.tm_wday) {
				for (size_t j = 0; j < unit->h.size(); j++) {
					int h = unit->h[j];
					if (h == curTM.tm_hour && unit->lastH != h) {
						RunEvent(&(*unit));
						unit->lastH = h;
						break;
					}
				}
				bool out = true;
				for (size_t j = 0; j < unit->h.size(); j++) {
					int h = unit->h[j];
					if (h == curTM.tm_hour) {
						out = false;
						break;
					}
				}
				if (out) {
					unit->lastH = -1;
				}
			}
		} else if (unit->type == TYPE_TIMET) {
			if (unit->done) {
				package.events[thread].erase(unit++);
				continue;
			} else if (!unit->done && cur >= unit->t) {
				if (unit->interval == -1) {
					unit->done = true;
					if (cur - unit->t > 60) {
						continue;
					}
				}else {
					unit->t = cur + (time_t)unit->interval;
				}
				
				RunEvent(&(*unit));
			}
		}
		unit++;
	}
}
