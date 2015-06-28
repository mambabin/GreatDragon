#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TARGET_LOG "Log.txt"
#define INTERVAL 15

static void InitDaemon(){
	int pid = fork();
	if(pid > 0)
		exit(EXIT_SUCCESS);
	else if(pid < 0)
		exit(EXIT_FAILURE);

	setsid();

	pid = fork();
	if(pid > 0)
		exit(EXIT_SUCCESS);
	else if(pid < 0)
		exit(EXIT_FAILURE);

	for(int i = 0; i < NOFILE; i++)
		close(i);

	umask(0);
}

static void Help(const char *program){
	printf("Usage: %s target\n", program);
}

static bool IsRunning(const char *target){
	char buf[1024];
	char *tempname = tmpnam(NULL);
	sprintf(buf, "ps -U `whoami` | grep %s > %s", target, tempname);
	system(buf);
	FILE *file = fopen(tempname, "r");
	fseek(file, 0, SEEK_END);
	bool ret = ftell(file) > 0;
	fclose(file);
	remove(tempname);
	return ret;
}

int main(int argc, char *argv[]){
	if(argc != 2){
		Help(argv[0]);
		exit(1);
	}

	const char *TARGET = argv[1];
	if(!IsRunning(TARGET)){
		Help(argv[0]);
		exit(1);
	}

	InitDaemon();

	for(;;){
		if(!IsRunning(TARGET)){
			char src[1024], buf[1024];
			sprintf(src, "%s", TARGET_LOG);
			time_t cur = time(NULL);
			tm *curTM = localtime(&cur);
			sprintf(buf, "%d-%d-%d-%d-%d-%d-%s", curTM->tm_year + 1900, curTM->tm_mon + 1, curTM->tm_mday, curTM->tm_hour, curTM->tm_min, curTM->tm_sec, TARGET_LOG);
			rename(src, buf);
			sprintf(buf, "./%s", TARGET);
			system(buf);
		}
		sleep(INTERVAL);
	}

	return 0;
}
