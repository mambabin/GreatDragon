#include "ConfigUtil.hpp"
#include "VersionInfo.pb.h"
#include "MD5.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <fstream>

using namespace std;

#define CONFIGFILE "Config-Scan.txt"
#define MAXFILES (1024 * 10)
#define MAXNAME 128
#define MAXFILESIZE (1024 * 1024 * 50)

static struct{
	char res[MAXNAME];
	char versionInfo[MAXNAME];
	char listFile[MAXNAME];
}config;

struct List{
	char list[MAXFILES][MAXNAME];
	int count;
};

struct OP{
	struct List update;
	struct List del;
};

static bool ReadConfig(){
	FILE *file = fopen(CONFIGFILE, "r");
	if(file == NULL){
		fprintf(stderr, "Failed to open config file: %s\n", CONFIGFILE);
		return false;
	}

	char buffer[1024], key[64];

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

	strcpy(key, "ListFile");
	if(!ConfigUtil_ReadStr(file, key, config.listFile, sizeof(config.listFile))){
		config.listFile[0] = '\0';
	}

	fclose(file);
	return true;
}

static bool CombineDirFile(const char *dir, const char *file, char *out, size_t size){
	int dirLen = strlen(dir);
	int fileLen = strlen(file);
	if(dirLen + fileLen + 1 >= (int)size)
		return false;

	strcpy(out, dir);
	if(out[dirLen - 1] != '/'){
		out[dirLen] = '/';
		out[dirLen + 1] = '\0';
	}
	strcat(out, file);

	return true;
}

static bool ListFiles(const char *dir, struct List *list){
	DIR *dp = opendir(dir);
	if(dp == NULL){
		fprintf(stderr, "Failed to open dir: %s\n", config.res);
		return false;
	}

	for(struct dirent *ep = readdir(dp); ep != NULL; ep = readdir(dp)){
		if(strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
			continue;

		char file[MAXNAME];
		if(!CombineDirFile(dir, ep->d_name, file, sizeof(file))){
			fprintf(stderr, "Failed to process %s in %s\n", ep->d_name, dir);
			closedir(dp);
			return false;
		}

		struct stat att;
		if(stat(file, &att) == -1){
			fprintf(stderr, "Failed to get att of %s\n", file);
			closedir(dp);
			return false;
		}

		if(S_ISDIR(att.st_mode)){
			if(!ListFiles(file, list)){
				closedir(dp);
				return false;
			}
		}
		else if(S_ISREG(att.st_mode)){
			if(list->count >= MAXFILES){
				fprintf(stderr, "Too many files\n");
				closedir(dp);
				return false;
			}

			strcpy(list->list[list->count], file);
			list->count++;
		}
		else{
			fprintf(stderr, "Invalid file: %s\n", file);
		}
	}

	closedir(dp);
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

static bool FileMD5(const char *name, const char *litter, size_t litterLen, string *hash){
	static char src[MAXFILESIZE], dest[MAXFILESIZE];
	int srcLen = ReadFile(name, "rb", src, sizeof(src));
	if(srcLen == -1)
		return false;

	memcpy(dest, litter, litterLen);
	memcpy(dest + litterLen, src, srcLen);

	*hash = md5(string(dest, litterLen + srcLen));
	return true;
}

static bool Compare(struct List *list, VersionInfo *versionInfo, struct OP *op){
	string hash;

	for(int i = 0; i < list->count; i++){
		char origin[MAXNAME];
		ExtraName(list->list[i], origin);

		bool find = false;

		for(int j = 0; j < versionInfo->origin_size(); j++){
			if(versionInfo->origin(j).compare(origin) == 0){
				find = true;

				if(!FileMD5(list->list[i], versionInfo->litter(j).data(), versionInfo->litter(j).size(), &hash))
					return false;

				if(hash == versionInfo->md5(j))
					break;

				strcpy(op->update.list[op->update.count], list->list[i]);
				op->update.count++;
				break;
			}
		}

		if(!find){
			strcpy(op->update.list[op->update.count], list->list[i]);
			op->update.count++;
		}
	}
	for(int i = 0; i < versionInfo->origin_size(); i++){
		bool find = false;
		for(int j = 0; j < list->count; j++){
			char origin[MAXNAME];
			ExtraName(list->list[j], origin);
			if(versionInfo->origin(i).compare(origin) == 0){
				find = true;
				break;
			}
		}
		if(!find){
			strcpy(op->del.list[op->del.count], versionInfo->origin(i).c_str());
			op->del.count++;
		}
	}

	return true;
}

static bool Gen(struct OP *op){
	FILE *file = stdout;
	if(config.listFile[0] != '\0'){
		file = fopen(config.listFile, "w");
		if(file == NULL){
			fprintf(stderr, "Failed to create file: %s\n", config.listFile);
			return false;
		}
	}

	if(op->update.count <= 0){
		fputs("Update = \n", file);
	}else{
		static char buffer[MAXNAME * MAXFILES];
		buffer[0] = '\0';

		int last = 0;
		for(int i = 0; i < op->update.count; i++){
			strcat(buffer + last, op->update.list[i]);
			last += strlen(op->update.list[i]);
			strcat(buffer + last, ", ");
			last += 2;
		}

		fputs("Update = ", file);
		fputs(buffer, file);
		fputs("\n", file);
	}

	if(op->del.count <= 0){
		fputs("Del = \n", file);
	}else{
		static char buffer[MAXNAME * MAXFILES];
		buffer[0] = '\0';

		int last = 0;
		for(int i = 0; i < op->del.count; i++){
			strcat(buffer + last, op->del.list[i]);
			last += strlen(op->del.list[i]);
			strcat(buffer + last, ", ");
			last += 2;
		}

		fputs("Del = ", file);
		fputs(buffer, file);
		fputs("\n", file);
	}

	if(file != stdout)
		fclose(file);
	return true;
}

int main(int argc, char *argv[]){
	if(!ReadConfig())
		return EXIT_FAILURE;

	static struct List list;
	list.count = 0;

	if(!ListFiles(config.res, &list))
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

	static struct OP op;
	op.update.count = 0;
	op.del.count = 0;

	if(!Compare(&list, &versionInfo, &op))
		return EXIT_FAILURE;

	if(!Gen(&op))
		return EXIT_FAILURE;

	return 0;
}
