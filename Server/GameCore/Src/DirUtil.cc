#include "DirUtil.hpp"
#include "Config.hpp"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <vector>
#include <string>

using namespace std;

int DirUtil_AllFiles(const char *dir, vector<string> *files) {
	if (dir == NULL || files == NULL)
		return -1;

	DIR *dp = opendir(dir);
	if (dp == NULL)
		return -1;

	int count = 0;
	char file[CONFIG_FIXEDARRAY];
	for (struct dirent *ep = readdir(dp); ep != NULL; ep = readdir(dp)) {
		if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
			continue;

		strcpy(file, dir);
		if (file[strlen(file) - 1] != '/')
			strcat(file, "/");
		strcat(file, ep->d_name);

		struct stat att;
		if (stat(file, &att) == -1)
			continue;

		if (S_ISREG(att.st_mode)) {
			files->push_back(file);
			count++;
		}
	}

	return count;
}

void DirUtil_CreateDir(const char *dir) {
	if (dir == NULL)
		return;

	if (access(dir, F_OK) == -1)
		mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
}
