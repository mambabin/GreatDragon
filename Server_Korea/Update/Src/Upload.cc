#include "ConfigUtil.hpp"
#include <zmq.h>
#include "VersionInfo.pb.h"
#include <MD5.hpp>
#include <zip.h>
#include <uuid.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <cmath>
#include <fstream>

using namespace std;

#define CONFIGFILE "Config-Upload.txt"
#define MAXSERVERS 32
#define MAXNAME 128
#define MAXFILES (1024 * 10)
#define MAXFILESIZE (1024 * 1024 * 50)

static struct{
	char ipBuffer[1024];
	char *ip[MAXSERVERS];
	int port[MAXSERVERS];
	int maxTry;
	int count;
	char listFile[MAXNAME];
	char out[MAXNAME];
	char versionInfo[MAXNAME];
	int litterVersion;
}config;

static struct{
	void *context;
	void *socket;
}net;

struct List{
	char buffer[MAXNAME * MAXFILES];
	char *list[MAXFILES];
	int count;
};

static bool ReadConfig(){
	FILE *file = fopen(CONFIGFILE, "r");
	if(file == NULL){
		fprintf(stderr, "Failed to open config file: %s\n", CONFIGFILE);
		return false;
	}

	char buffer[1024], key[128];

	strcpy(key, "ServerIP");
	if(!ConfigUtil_ReadStr(file, key, config.ipBuffer, sizeof(config.ipBuffer))
			|| (config.count = ConfigUtil_ExtraToken(config.ipBuffer, config.ip, MAXSERVERS, " ,")) == -1){
		
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}

	char *portBuffer[MAXSERVERS];
	strcpy(key, "ServerPort");
	if(!ConfigUtil_ReadStr(file, key, buffer, sizeof(buffer))
			|| ConfigUtil_ExtraToken(buffer, portBuffer, MAXSERVERS, " ,") != config.count){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}
	for(int i = 0; i < config.count; i++)
		config.port[i] = atoi(portBuffer[i]);

	strcpy(key, "MaxTry");
	if(!ConfigUtil_ReadStr(file, key, buffer, sizeof(buffer))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}
	config.maxTry = atoi(buffer);

	strcpy(key, "ListFile");
	if(!ConfigUtil_ReadStr(file, key, config.listFile, sizeof(config.listFile))){
		config.listFile[0] = '\0';
	}

	strcpy(key, "Out");
	if(!ConfigUtil_ReadStr(file, key, config.out, sizeof(config.out))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}
	int len = strlen(config.out);
	if(config.out[len - 1] != '/'){
		config.out[len] = '/';
		config.out[len + 1] = '\0';
	}

	strcpy(key, "VersionInfo");
	if(!ConfigUtil_ReadStr(file, key, config.versionInfo, sizeof(config.versionInfo))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}

	strcpy(key, "LitterVersion");
	if(!ConfigUtil_ReadStr(file, key, buffer, sizeof(buffer))){
		fprintf(stderr, "Failed to read key: %s\n", key);
		fclose(file);
		return false;
	}
	config.litterVersion = atoi(buffer);

	fclose(file);
	return true;
}

static bool ReadList(const char *key, struct List *list){
	FILE *file = stdin;
	if(config.listFile[0] != '\0'){
		file = fopen(config.listFile, "r");
		if(file == NULL){
			fprintf(stderr, "Failed to open file: %s\n", config.listFile);
			return false;
		}
	}

	if(!ConfigUtil_ReadStr(file, key, list->buffer, sizeof(list->buffer))
			|| (list->count = ConfigUtil_ExtraToken(list->buffer, list->list, sizeof(list->list) / sizeof(list->list[0]), " ,")) == -1){
		fprintf(stderr, "Failed to read list: %s\n", key);
		if(file != stdin)
			fclose(file);
		return false;
	}

	if(file != stdin)
		fclose(file);
	return true;
}

static void ExtraName(const char *full, char *name){
	int len = strlen(full);
	int pos = len - 1;
	while(pos >= 0 && full[pos] != '/')
		pos--;
	if(pos < 0)
		pos = 0;
	if(full[pos] == '/')
		pos++;

	int i = 0;
	while(pos < len)
		name[i++] = full[pos++];
	name[i] = '\0';
}

static bool ExtraExt(const char *full, char *ext){
	int len = strlen(full);
	int pos = len - 1;
	while(pos >= 0 && full[pos] != '.')
		pos--;
	if(pos < 0)
		return false;

	int i = 0;
	while(pos < len)
		ext[i++] = full[pos++];
	ext[i] = '\0';

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

static size_t AddLitter(const char *src, size_t srcSize, char *dest, size_t destSize){
	switch(config.litterVersion){
		case 0:
			{
				if(destSize < srcSize + 10)
					return 0;

				*(short *)dest = 0;

				srand(time(NULL));
				for(int i = 2; i < 10; i++)
					dest[i] = (char)(rand() % 256);

				memcpy(dest + 10, src, srcSize);
				return srcSize + 10;
			}

		default:
			break;
	}

	return 0;
}

static bool UpdateVersionInfo(VersionInfo *versionInfo){
	fstream f(config.versionInfo, ios::binary | ios::out);
	if(f.fail()){
		fprintf(stderr, "Failed to open file: %s\n", config.versionInfo);
		return false;
	}

	if(!versionInfo->SerializeToOstream(&f)){
		fprintf(stderr, "Failed to serialize to file: %s\n", config.versionInfo);
		return false;
	}
	f.close();

	return true;
}

static int ExtraLitter(const char *data, char *litter, size_t size){
	short version = *(short *)data;
	switch(version){
		case 0:
			{
				if((int)size < 10)
					return -1;

				memcpy(litter, data, 10);
				return 10;
			}

		default:
			break;
	}

	return -1;
}

static bool Gen(struct List *list, struct List *del, VersionInfo *versionInfo, struct List *out){
	for(int i = 0; i < del->count; i++){
		for(int j = 0; j < versionInfo->origin_size(); j++){
			if(versionInfo->origin(j).compare(del->list[i]) == 0){
				if(j < versionInfo->origin_size() - 1){
					versionInfo->mutable_origin()->SwapElements(j, versionInfo->origin_size() - 1);
					versionInfo->mutable_guid()->SwapElements(j, versionInfo->origin_size() - 1);
					versionInfo->mutable_md5()->SwapElements(j, versionInfo->origin_size() - 1);
					versionInfo->mutable_size()->SwapElements(j, versionInfo->origin_size() - 1);
					versionInfo->mutable_litter()->SwapElements(j, versionInfo->origin_size() - 1);
				}
				versionInfo->mutable_origin()->RemoveLast();
				versionInfo->mutable_guid()->RemoveLast();
				versionInfo->mutable_md5()->RemoveLast();
				versionInfo->mutable_size()->RemoveLast();
				versionInfo->mutable_litter()->RemoveLast();
				break;
			}
		}
	}

	uuid_t *uuid = NULL;
	uuid_rc_t rc;
	if((rc = uuid_create(&uuid)) != UUID_RC_OK){
		fprintf(stderr, "Error of uuid_create: %s\n", uuid_error(rc));
		return false;
	}

	for(int i = 0; i < list->count; i++){
		char name[MAXNAME], origin[MAXNAME], uu[MAXNAME], *ptr = uu;
		size_t size = sizeof(uu);

		ExtraName(list->list[i], origin);

		for(;;){
			if((rc = uuid_make(uuid, UUID_MAKE_V1)) != UUID_RC_OK){
				fprintf(stderr, "Error of uuid_make: %s\n", uuid_error(rc));
				uuid_destroy(uuid);
				return false;
			}
			if((rc = uuid_export(uuid, UUID_FMT_STR, &ptr, &size)) != UUID_RC_OK){
				fprintf(stderr, "Error of uuid_export: %s\n", uuid_error(rc));
				uuid_destroy(uuid);
				return false;
			}

			strcpy(name, config.out);
			strcat(name, uu);
			strcat(name, ".zip");

			FILE *file = fopen(name, "r");
			if(file == NULL)
				break;
			fclose(file);
		}

		static char src[MAXFILESIZE], dest[MAXFILESIZE];
		int srcLen = ReadFile(list->list[i], "rb", src, sizeof(src));
		if(srcLen == -1){
			uuid_destroy(uuid);
			return false;
		}

		size_t destLen = AddLitter(src, srcLen, dest, sizeof(dest));
		if(destLen == 0){
			fprintf(stderr, "Failed to add litter: %s\n", list->list[i]);
			uuid_destroy(uuid);
			return false;
		}

		char litter[1024];
		int litterLen = ExtraLitter(dest, litter, sizeof(litter));
		if(litterLen == -1){
			fprintf(stderr, "Failed to extra litter\n");
			return false;
		}

		int pos = 0;
		for(; pos < versionInfo->origin_size(); pos++){
			if(versionInfo->origin(pos) == origin)
				break;
		}
		if(pos >= versionInfo->origin_size()){
			versionInfo->add_origin(origin);
			versionInfo->add_guid(uu);
			versionInfo->add_md5(md5(string(dest, destLen)));
			versionInfo->add_size(destLen);
			versionInfo->add_litter(string(litter, litterLen));
		}
		else{
			versionInfo->set_guid(pos, uu);
			versionInfo->set_md5(pos, md5(string(dest, destLen)));
			versionInfo->set_size(pos, destLen);
			versionInfo->set_litter(pos, string(litter, litterLen));
		}

		HZIP hz = CreateZip(name, NULL);

		if(ZipAdd(hz, uu, dest, destLen) != ZR_OK){
			fprintf(stderr, "Failed to zip: %s\n", list->list[i]);
			uuid_destroy(uuid);
			CloseZip(hz);
			remove(name);
			return false;
		}

		ExtraName(name, out->list[out->count]);
		out->list[out->count + 1] = out->list[out->count] + strlen(out->list[out->count]) + 1;
		out->count++;

		CloseZip(hz);
	}

	versionInfo->set_version((int64_t)time(NULL));

	uuid_destroy(uuid);
	return true;
}

static bool InitZMQ(){
	net.context = net.socket = NULL;

	if((net.context = zmq_init(1)) == NULL){
		fprintf(stderr, "Failed to create zmq context: %s\n", zmq_strerror(errno));
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

static bool Connect(int index){
	if(net.socket != NULL)
		zmq_close(net.socket);

	if((net.socket = zmq_socket(net.context, ZMQ_REQ)) == NULL){
		fprintf(stderr, "Failed to create zmq socket: %s\n", zmq_strerror(errno));
		return false;
	}

	char buffer[128];
	sprintf(buffer, "tcp://%s:%d", config.ip[index], config.port[index]);
	if(zmq_connect(net.socket, buffer) == -1){
		fprintf(stderr, "Failed to connect to server: %s, err: \n", buffer, zmq_strerror(errno));
		return false;
	}

	return true;
}

// 0: "VersionInfo.bytes" + data
// 1: "Out" + data
// 2: "Res" + data

static bool Run(VersionInfo *versionInfo, struct List *out){
	const char *header[] = {"VersionInfo.bytes", "Out", "Res"};
	int res = 0;
	int retry = 0;

	for(int i = 0; i < sizeof(header) / sizeof(header[0]); i++){
		switch(i){
			case 0:
				{
					zmq_msg_t msg;
					zmq_msg_init_size(&msg, strlen(header[i]) + 1 + versionInfo->ByteSize());
					memcpy(zmq_msg_data(&msg), header[i], strlen(header[i]) + 1);

					if(!versionInfo->SerializeToArray((char *)zmq_msg_data(&msg) + strlen(header[i]) + 1, versionInfo->ByteSize())){
						fprintf(stderr, "Failed to serialize VersionInfo\n");
						return false;
					}

					int rc = zmq_send(net.socket, &msg, 0);
					zmq_msg_close(&msg);
					if(rc == -1){
						fprintf(stderr, "Failed to send msg: %s\n", zmq_strerror(errno));
						return false;
					}
				}
				break;

			case 1:
				{
					int len = 0;
					for(int j = 0; j < out->count; j++)
						len += strlen(out->list[j]) + 1;

					zmq_msg_t msg;
					zmq_msg_init_size(&msg, strlen(header[i]) + 1 + len);
					memcpy(zmq_msg_data(&msg), header[i], strlen(header[i]) + 1);
					memcpy((char *)zmq_msg_data(&msg) + strlen(header[i]) + 1, out->buffer, len);

					int rc = zmq_send(net.socket, &msg, 0);
					zmq_msg_close(&msg);
					if(rc == -1){
						fprintf(stderr, "Failed to send msg: %s\n", zmq_strerror(errno));
						return false;
					}
				}
				break;

			case 2:
				{
					if(out->count == 0)
						return true;

					char full[MAXNAME];
					strcpy(full, config.out);
					strcat(full, out->list[res]);

					static char buffer[MAXFILESIZE];
					int size = ReadFile(full, "rb", buffer, sizeof(buffer));
					if(size == -1)
						return false;

					zmq_msg_t msg;
					zmq_msg_init_size(&msg, strlen(header[i]) + 1 + size);
					memcpy(zmq_msg_data(&msg), header[i], strlen(header[i]) + 1);
					memcpy((char *)zmq_msg_data(&msg) + strlen(header[i]) + 1, buffer, size);

					int rc = zmq_send(net.socket, &msg, 0);
					zmq_msg_close(&msg);
					if(rc == -1){
						fprintf(stderr, "Failed to send msg: %s\n", zmq_strerror(errno));
						return false;
					}

					if(++res < out->count)
						i--;
				}
				break;

			default:
				assert(0);
		}

		zmq_msg_t msg;
		zmq_msg_init(&msg);
		int rc = zmq_recv(net.socket, &msg, 0);
		if(rc == -1){
			switch(errno){
				case EINTR:
					fprintf(stderr, "Exit while uploading\n");
					exit(EXIT_FAILURE);

				default:
					fprintf(stderr, "Failed to recv msg: %s\n", zmq_strerror(errno));
					zmq_msg_close(&msg);
					return false;
			}
		}

		if(strcmp((char *)zmq_msg_data(&msg), "Success") == 0){
		}
		else if(strcmp((char *)zmq_msg_data(&msg), "Failure") == 0){
			if(config.maxTry == -1 || retry < config.maxTry){
				fprintf(stderr, "Failure on server, try again\n");
				if(config.maxTry != -1)
					retry++;
				i = -1;
			}
			else{
				fprintf(stderr, "Failure on server\n");
				zmq_msg_close(&msg);
				return false;
			}
		}
		else{
			fprintf(stderr, "Recv wrong reply\n");
			zmq_msg_close(&msg);
			return false;
		}
		zmq_msg_close(&msg);
	}

	return true;
}

static bool Upload(VersionInfo *versionInfo, struct List *out){
	if(config.count > 0){
		if(!InitZMQ())
			return false;

		for(int i = 0; i < config.count; i++){
			if(!Connect(i))
				return false;

			if(!Run(versionInfo, out))
				return false;
		}

		if(!UpdateVersionInfo(versionInfo))
			return false;

		DestroyZMQ();
	}
	else{
		if(!UpdateVersionInfo(versionInfo))
			return false;
	}

	return true;
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

	ClearDir(config.out);

	static struct List update;
	update.count = 0;
	if(!ReadList("Update", &update))
		return EXIT_FAILURE;

	static struct List del;
	del.count = 0;
	if(!ReadList("Del", &del))
		return EXIT_FAILURE;

	fstream f(config.versionInfo, ios::binary | ios::in);
	if(f.fail()){
		fprintf(stderr, "Failed to open file: %s\n", config.versionInfo);
		return EXIT_FAILURE;
	}
	VersionInfo versionInfo;
	if(!versionInfo.ParseFromIstream(&f)){
		fprintf(stderr, "Failed to parse file: %s\n", config.versionInfo);
		return EXIT_FAILURE;
	}
	f.close();

	static struct List out;
	out.count = 0;
	out.list[0] = out.buffer;
	if(!Gen(&update, &del, &versionInfo, &out))
		return EXIT_FAILURE;

	if(!Upload(&versionInfo, &out))
		return EXIT_FAILURE;

	return 0;
}

