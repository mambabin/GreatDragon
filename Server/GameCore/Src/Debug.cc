#include "Debug.hpp"
#include "Config.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <execinfo.h>

using namespace std;

static struct Debug{
	// FILE *file;
	int outFD;
	pthread_mutex_t mutex;
}debug;

void Debug_Init() {
	/*
	debug.file = fopen("Log.txt", "w");
	assert(debug.file != NULL);
	*/
	debug.outFD = open("Log.txt", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert(debug.outFD >= 0);

	int res = dup2(debug.outFD, STDOUT_FILENO);
	assert(res >= 0);
	res = dup2(debug.outFD, STDERR_FILENO);
	assert(res >= 0);

	res = pthread_mutex_init(&debug.mutex, NULL);
	assert(res == 0);
}

void Debug_Log(const char *file, int line, const char *format, ...) {
	if (!Config_LogInfo())
		return;
	if (format == NULL || format[0] == '\0')
		return;

	pthread_mutex_lock(&debug.mutex);
	va_list args;
	va_start(args, format);

	time_t cur;
	time(&cur);
	char *sCur = ctime(&cur);
	sCur[strlen(sCur) - 1] = '\0';

	fprintf(stdout, "[Info %s %s %d]", sCur, file, line);
	vfprintf(stdout, format, args);
	fprintf(stdout, "\n");
	fflush(stdout);

	va_end(args);
	pthread_mutex_unlock(&debug.mutex);
	/*
	va_start(args, format);

	fprintf(debug.file, "[Info %s %s %d]", sCur, file, line);
	vfprintf(debug.file, format, args);
	fprintf(debug.file, "\n");
	fflush(debug.file);

	va_end(args);
	*/
}

void Debug_LogError(const char *file, int line, const char *format, ...) {
	if (!Config_LogError())
		return;
	if (format == NULL || format[0] == '\0')
		return;

	pthread_mutex_lock(&debug.mutex);
	va_list args;
	va_start(args, format);

	time_t cur;
	time(&cur);
	char *sCur = ctime(&cur);
	sCur[strlen(sCur) - 1] = '\0';

	fprintf(stdout, "[Error %s %s %d]", sCur, file, line);
	vfprintf(stdout, format, args);
	fprintf(stdout, "\n");
	fflush(stdout);

	va_end(args);
	pthread_mutex_unlock(&debug.mutex);
	/*
	va_start(args, format);

	fprintf(debug.file, "[Error %s %s %d]", sCur, file, line);
	vfprintf(debug.file, format, args);
	fprintf(debug.file, "\n");
	fflush(debug.file);

	va_end(args);
	*/
}

void Debug_LogRecord(const char *file, int line, const char *format, ...) {
	if (!Config_LogRecord())
		return;
	if (format == NULL || format[0] == '\0')
		return;

	pthread_mutex_lock(&debug.mutex);
	va_list args;
	va_start(args, format);

	time_t cur;
	time(&cur);
	char *sCur = ctime(&cur);
	sCur[strlen(sCur) - 1] = '\0';

	fprintf(stdout, "[Record %s %s %d]", sCur, file, line);
	vfprintf(stdout, format, args);
	fprintf(stdout, "\n");
	fflush(stdout);

	va_end(args);
	pthread_mutex_unlock(&debug.mutex);
	/*
	va_start(args, format);

	fprintf(debug.file, "[Record %s %s %d]", sCur, file, line);
	vfprintf(debug.file, format, args);
	fprintf(debug.file, "\n");
	fflush(debug.file);

	va_end(args);
	*/
}

void Debug_LogCallStack() {
	void *array[16] = {NULL};
	size_t size = backtrace(array, sizeof(array) / sizeof(array[0]));
	char **contents = (char **)backtrace_symbols(array, size);

	DEBUG_LOGERROR("----CallStack-----");
	for (size_t i = 0; i < size; i++)
		DEBUG_LOGERROR("[%u]%s", i, contents[i]);
	DEBUG_LOGERROR("------------------");

	free(contents);
}
