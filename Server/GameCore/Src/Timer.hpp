#ifndef _TIMER_HPP_
#define _TIMER_HPP_

#include <sys/types.h>
#include <cstddef>

#define TIMER_THREAD_GC 0
#define TIMER_THREAD_DC 1

typedef void (*Timer_Event)(void *arg);

void Timer_Init(int thread);

// wday:
// -1: EveryDay, 0: Sunday, 1: Monday...
void Timer_AddEvent(int wday, int h[], size_t size, Timer_Event event, void *arg, const char *name = NULL, int thread = TIMER_THREAD_GC);

// t: from t
// interval:
// -1: once, 0: every frame, >0: seconds
void Timer_AddEvent(time_t t, int interval, Timer_Event event, void *arg, const char *name = NULL, int thread = TIMER_THREAD_GC);

void Timer_Update(int thread);

#endif
