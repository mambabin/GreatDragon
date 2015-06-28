#include "SignalHandler.hpp"
#include "Debug.hpp"
#include <csignal>
#include <ctime>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

static bool over = false;

void SignalHandler_NormalExit(int sigNum) {
	const char *name = NULL;
	switch (sigNum) {
		case SIGABRT:
			name = "SIGABRT";
			break;
		case SIGFPE:
			name = "SIGFPE";
			break;
		case SIGILL:
			name = "SIGILL";
			break;
		case SIGINT:
			name = "SIGINT";
			break;
		case SIGSEGV:
			name = "SIGSEGV";
			break;
		case SIGTERM:
			name = "SIGTERM";
			break;
		default:
			name = "unknown";
			break;
	}
	DEBUG_LOGRECORD("Exit by signal: %d(%s)", sigNum, name);
	over = true;
}

void SignalHandler_ExitAndLogCallStack(int sigNum) {
	DEBUG_LOG("");

	if (sigNum == SIGSEGV) {
		DEBUG_LOGERROR("SIGSEGV exception -- Invalid memory reference");
	}
	else if (sigNum == SIGABRT) {
		DEBUG_LOGERROR("SIGABRT exception -- Abort signal from abort()");
	}
	else if (sigNum == SIGFPE) {
		DEBUG_LOGERROR("SIGFPE exception -- Floatint point exception");
	}
	else {
		DEBUG_LOGERROR("Another exception, signum = %d", sigNum);
	}

	Debug_LogCallStack();
	exit(1);
}

bool SignalHandler_IsOver() {
	return over;
}

