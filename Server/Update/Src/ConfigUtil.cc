#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#define MAXLINE (1024 * 1024 * 10)

bool ConfigUtil_ReadStr(FILE *file, const char *key, char *value, size_t size){
	if(file == NULL || key == NULL || value == NULL)
		return false;

	fseek(file, 0, SEEK_SET);

	char *line = new char[MAXLINE];
	while(fgets(line, MAXLINE, file) != NULL){
		if(line[0] == '#')
			continue;

		if(strstr(line, key) == &line[0]){
			int last = strlen(key);
			if(line[last] != '=' && line[last] != ' ')
				continue;

			while(line[last] == '=' || line[last] == ' ')
				last++;

			if(strlen(line + last) >= size){
				delete[] line;
				return false;
			}

			char *dest = value;
			while(line[last] != '\0' && line[last] != '\n')
				*dest++ = line[last++];
			*dest = '\0';

			delete[] line;
			return true;
		}
	}

	delete[] line;
	return false;
}

int ConfigUtil_ExtraToken(char *str, char *tokens[], size_t size, const char *delim){
	if(str == NULL || tokens == NULL || delim == NULL)
		return -1;

	int count = 0;
	char *token = strtok(str, delim);
	while(token != NULL){
		if(count >= (int)size)
			return -1;

		tokens[count++] = token;
		token = strtok(NULL, delim);
	}
	return count;
}

bool ConfigUtil_WriteStr(FILE *file, const char *key, const char *value){
	if(file == NULL || key == NULL || value == NULL)
		return false;

	fseek(file, 0, SEEK_END);
	fputs(key, file);
	fputs(" = ", file);
	fputs(value, file);

	return true;
}
