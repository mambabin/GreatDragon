#include "ConfigUtil.hpp"
#include "VersionInfo.pb.h"
#include "CheckVersion.pb.h"
#include <MD5.hpp>
#include <zmq.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <fstream>

using namespace std;

#define CONFIGFILE "Config-Server.txt"
#define TEMPDIR "Temp/"
#define MAXFILES (1024 * 10)
#define MAXNAME 128
#define MAXFILESIZE (1024 * 1024 * 50)

static struct{
	int port;
	char versionInfo[MAXNAME];
	char res[MAXNAME];
	char checkVersion[MAXNAME];
}config;

static struct{
	void *context;
	void *socket;
}net;

struct List{
	char list[MAXFILES][MAXNAME];
	int count;
};

static struct{
	VersionInfo versionInfo;
	struct List out;
	int cur;
}status;


static void InitStatus(){
	status.versionInfo.Clear();
	status.out.count = 0;
	status.cur = 0;
}

static bool InitZMQ(){
	net.context = net.socket = NULL;

	if((net.context = zmq_init(1)) == NULL){
		fprintf(stderr, "Failed to create zmq context: %s\n", zmq_strerror(errno));
		return false;
	}

	if((net.socket = zmq_socket(net.context, ZMQ_REP)) == NULL){
		fprintf(stderr, "Failed to create zmq socket: %s\n", zmq_strerror(errno));
		return false;
	}

	char buffer[128];
	sprintf(buffer, "tcp://*:%d", config.port);
	if(zmq_bind(net.socket, buffer) == -1){
		fprintf(stderr, "Failed to bind zmq socket: %s\n", zmq_strerror(errno));
		return false;
	}

	return true;
}

static void DestroyZMQ(){
	if(net.socket != NULL){
		zmq_close(net.socket);
		net.socket = NULL;
	}
	if(net.context != NULL){
		zmq_term(net.context);
		net.context = NULL;
	}
}

static bool ReadConfig(){
	FILE *file = fopen(CONFIGFILE, "r");
	if(file == NULL){
		fprintf(stderr, "Failed to open config file: %s\n", CONFIGFILE);
		return false;
	}

	char buffer[1024], key[64];

	strcpy(key, "port");
	if(!ConfigUtil_ReadStr(file, key, buffer, sizeof(buffer))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}
	config.port = atoi(buffer);

	strcpy(key, "Res");
	if(!ConfigUtil_ReadStr(file, key, config.res, sizeof(config.res))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}
	int len = strlen(config.res);
	if(config.res[len - 1] != '/'){
		config.res[len] = '/';
		config.res[len + 1] = '\0';
	}

	strcpy(key, "VersionInfo");
	if(!ConfigUtil_ReadStr(file, key, config.versionInfo, sizeof(config.versionInfo))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}

	strcpy(key, "CheckVersion");
	if(!ConfigUtil_ReadStr(file, key, config.checkVersion, sizeof(config.checkVersion))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

static int ReadFile(const char *name, const char *mode, char *buffer, size_t size){
	FILE *file = fopen(name, mode);
	if(file == NULL){
		fprintf(stderr, "Failed to open file: %s\n", name);
		return -1;
	}

	fseek(file, 0, SEEK_END);
	int len = ftell(file);
	if(len > (int)size){
		fprintf(stderr, "File: %s is too large to read\n", name);
		fclose(file);
		return -1;
	}

	fseek(file, 0, SEEK_SET);
	int num = fread(buffer, 1, len, file);
	if(num != len){
		fprintf(stderr, "Failed to read file: %s\n", name);
		fclose(file);
		return -1;
	}

	fclose(file);
	return num;
}

static int WriteFile(const char *name, const char *mode, const char *buffer, size_t size){
	FILE *file = fopen(name, mode);
	if(file == NULL){
		fprintf(stderr, "Failed to open file: %s\n", name);
		return -1;
	}

	if(fwrite(buffer, 1, size, file) != size){
		fprintf(stderr, "Failed to write file: %s\n", name);
		fclose(file);
		return -1;
	}

	fclose(file);
	return (int)size;
}

static bool UpdateVersionInfo(){
	fstream f(config.versionInfo, ios::binary | ios::in);
	if(!f.fail()){
		VersionInfo backup;
		if(!backup.ParseFromIstream(&f)){
			fprintf(stderr, "Failed to parse prev versioninfo\n");
			return false;
		}
		f.close();

		char backupName[MAXNAME];
		time_t cur = (time_t)backup.version();
		struct tm *curTM = localtime(&cur);
		sprintf(backupName, "%s-%d-%d-%d-%d-%d-%d", config.versionInfo, 1900 + curTM->tm_year, curTM->tm_mon + 1, curTM->tm_mday, curTM->tm_hour, curTM->tm_min, curTM->tm_sec);

		f.open(backupName, ios::binary | ios::out);
		if(f.fail()){
			fprintf(stderr, "Failed to open file: %s\n", backupName);
			return false;
		}
		if(!backup.SerializeToOstream(&f)){
			fprintf(stderr, "Failed to serialize to file: %s\n", backupName);
			return false;
		}
		f.close();
	}

	f.open(config.versionInfo, ios::binary | ios::out);
	if(f.fail()){
		fprintf(stderr, "Failed to open file: %s\n", config.versionInfo);
		return false;
	}

	if(!status.versionInfo.SerializeToOstream(&f)){
		fprintf(stderr, "Failed to serialize to file: %s\n", config.versionInfo);
		return false;
	}
	f.close();

	return true;
}

static bool UpdateCheckVersion(){
	fstream f(config.checkVersion, ios::binary | ios::in);
	if(!f.fail()){
		VersionForCheck backup;
		if(!backup.ParseFromIstream(&f)){
			fprintf(stderr, "Failed to parse prev versioninfo\n");
			return false;
		}
		f.close();

		char backupName[MAXNAME];
		time_t cur = (time_t)backup.version();
		struct tm *curTM = localtime(&cur);
		sprintf(backupName, "%s-%d-%d-%d-%d-%d-%d", config.checkVersion, 1900 + curTM->tm_year, curTM->tm_mon + 1, curTM->tm_mday, curTM->tm_hour, curTM->tm_min, curTM->tm_sec);

		f.open(backupName, ios::binary | ios::out);
		if(f.fail()){
			fprintf(stderr, "Failed to open file: %s\n", backupName);
			return false;
		}
		if(!backup.SerializeToOstream(&f)){
			fprintf(stderr, "Failed to serialize to file: %s\n", backupName);
			return false;
		}
		f.close();
	}

	f.open(config.checkVersion, ios::binary | ios::out);
	if(f.fail()){
		fprintf(stderr, "Failed to open file: %s\n", config.checkVersion);
		return false;
	}

	static char buffer[MAXFILESIZE];
	if(!status.versionInfo.SerializeToArray(buffer, sizeof(buffer))){
		fprintf(stderr, "Failed to serialize VersionInfo\n");
		return false;
	}

	VersionForCheck checkVersion;
	checkVersion.set_version(status.versionInfo.version());
	checkVersion.set_md5(md5(string(buffer, status.versionInfo.ByteSize())));

	if(!checkVersion.SerializeToOstream(&f)){
		fprintf(stderr, "Failed to serialize to file: %s\n", config.checkVersion);
		return false;
	}
	f.close();

	return true;
}

static void RemoveCur(const char *dir){
	for(int i = 0; i < status.out.count; i++){
		char name[MAXNAME];
		strcpy(name, dir);
		strcat(name, status.out.list[i]);
		remove(name);
	}
}

static bool Move(){
	static char buffer[MAXFILESIZE];

	for(int i = 0; i < status.out.count; i++){
		char src[MAXNAME], dest[MAXNAME];
		strcpy(src, TEMPDIR);
		strcat(src, status.out.list[i]);

		int size = ReadFile(src, "rb", buffer, sizeof(buffer));
		if(size == -1)
			return false;

		strcpy(dest, config.res);
		strcat(dest, status.out.list[i]);

		if(WriteFile(dest, "wb", buffer, size) == -1)
			return false;

		remove(src);
	}

	return true;
}

// 0: "VersionInfo.bytes" + data
// 1: "Out" + data
// 2: "Res" + data

static bool ProcessMsg(zmq_msg_t *msg){
	void *data = zmq_msg_data(msg);
	size_t size = zmq_msg_size(msg);

	const char *header[] = {"VersionInfo.bytes", "Out", "Res"};
	int step = 0;
	for(; step < sizeof(header) / sizeof(header[0]); step++){
		if(size >= strlen(header[step]) && strcmp((char *)data, header[step]) == 0)
			break;
	}

	switch(step){
		case 0:
			{
				InitStatus();

				if(!status.versionInfo.ParseFromArray((char *)data + strlen(header[step]) + 1, size - strlen(header[step]) - 1)){
					fprintf(stderr, "Failed to parse VersionInfo\n");
					return false;
				}
			}
			break;

		case 1:
			{
				for(char *str = (char *)data + strlen(header[step]) + 1; str < (char *)data + size; str += strlen(str) + 1)
					strcpy(status.out.list[status.out.count++], str);

				if(status.out.count <= 0)
					return UpdateVersionInfo() && UpdateCheckVersion();
			}
			break;

		case 2:
			{
				if(status.cur >= status.out.count){
					fprintf(stderr, "Too many res\n");
					RemoveCur(config.res);
					RemoveCur(TEMPDIR);
					return false;
				}

				char full[MAXNAME];
				strcpy(full, TEMPDIR);
				strcat(full, status.out.list[status.cur]);

				if(WriteFile(full, "wb", (char *)data + strlen(header[step]) + 1, size - strlen(header[step]) - 1) == -1){
					RemoveCur(config.res);
					RemoveCur(TEMPDIR);
					return false;
				}

				if(++status.cur >= status.out.count){
					if(!Move()){
						fprintf(stderr, "Failed to move res: \n");
						RemoveCur(config.res);
						RemoveCur(TEMPDIR);
						return false;
					}
					if(!UpdateVersionInfo()){
						RemoveCur(config.res);
						return false;
					}
					if(!UpdateCheckVersion()){
						RemoveCur(config.res);
						return false;
					}
					return true;
				}
			}
			break;

		default:
			fprintf(stderr, "Recv wrong header\n");
			return false;
	}

	return true;
}

static void Run(){
	for(;;){
		zmq_msg_t msg;
		zmq_msg_init(&msg);

		int rc = zmq_recv(net.socket, &msg, 0);
		if(rc == -1){
			switch(errno){
				case EINTR:
					return;
				default:
					fprintf(stderr, "Failed to recv msg: %d %s\n", errno, zmq_strerror(errno));
					break;
			}
		}

		char reply[128];
		if(rc == -1 || !ProcessMsg(&msg)){
			strcpy(reply, "Failure");
		}
		else{
			strcpy(reply, "Success");
		}

		zmq_msg_close(&msg);
		zmq_msg_init_size(&msg, strlen(reply) + 1);
		memcpy(zmq_msg_data(&msg), reply, strlen(reply) + 1);
		zmq_send(net.socket, &msg, 0);
		zmq_msg_close(&msg);
	}
}

static void ClearDir(const char *dir){
	DIR *dp = opendir(dir);
	if(dp == NULL)
		return;

	for(struct dirent *ep = readdir(dp); ep != NULL; ep = readdir(dp)){
		if(strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
			continue;

		char file[MAXNAME];
		if(strlen(dir) + strlen(ep->d_name) + 1 >= MAXNAME)
			continue;
		strcpy(file, dir);
		strcat(file, ep->d_name);

		remove(file);
	}
}

int main(int argc, char *argv[]){
	if(!ReadConfig())
		return EXIT_FAILURE;

	if(!InitZMQ()){
		DestroyZMQ();
		return EXIT_FAILURE;
	}

	InitStatus();
	ClearDir(TEMPDIR);
	Run();

	DestroyZMQ();
	return 0;
}
