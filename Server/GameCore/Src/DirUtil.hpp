#ifndef _DIR_UTIL_HPP_
#define _DIR_UTIL_HPP_

#define MAX_FILE_NAME 128

#include <sys/types.h>
#include <vector>
#include <string>

int DirUtil_AllFiles(const char *dir, std::vector<std::string> *files);

void DirUtil_CreateDir(const char *dir);

#endif
