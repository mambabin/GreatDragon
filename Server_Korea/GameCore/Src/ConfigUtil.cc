#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "Config.hpp"

#define MAXLINE (1024 * 1024)
#define MAX_KEY (1024)

bool ConfigUtil_ReadStr(FILE *file, const char *key, char *value, size_t size) {
	if (file == NULL || key == NULL || value == NULL)
		return false;

	fseek(file, 0, SEEK_SET);

	char line[MAXLINE];
	while (fgets(line, sizeof(line), file) != NULL) {
		if (line[0] == '#')
			continue;

		if (strstr(line, key) == &line[0]) {
			int last = strlen(key);
			if (line[last] != '=' && line[last] != ' ')
				continue;

			while (line[last] == '=' || line[last] == ' ')
				last++;

			if (strlen(line + last) >= size)
				return false;

			char *dest = value;
			while (line[last] != '\0' && line[last] != '\n')
				*dest++ = line[last++];
			*dest = '\0';

			return true;
		}
	}

	return false;
}

bool ConfigUtil_Key(FILE *file, int n, char *key, size_t size) {
	if (file == NULL || n < 0 || key == NULL || size <= 0)
		return false;

	fseek(file, 0, SEEK_SET);

	int i = 0;
	char line[MAXLINE];
	while (fgets(line, sizeof(line), file) != NULL) {
		if (line[0] == '#')
			continue;
		char *pos = strchr(line, '=');
		if (pos == NULL)
			continue;
		if (i == n) {
			while (*(pos - 1) == ' ')
				pos--;
			if ((size_t)(pos - line) >= size)
				return false;
			strncpy(key, line, pos - line);
			key[pos - line] = '\0';
			return true;
		}
		i++;
	}

	return false;
}

int ConfigUtil_ExtraToken(char *str, char *tokens[], size_t size, const char *delim) {
	if (str == NULL || tokens == NULL || delim == NULL)
		return -1;

	int count = 0;
	char *token = strtok(str, delim);
	while (token != NULL) {
		if (count >= (int)size)
			return -1;

		tokens[count++] = token;
		token = strtok(NULL, delim);
	}
	return count;
}

bool ConfigUtil_WriteStr(FILE *file, const char *key, const char *value) {
	if (file == NULL || key == NULL || value == NULL)
		return false;

	fseek(file, 0, SEEK_END);
	fputs(key, file);
	fputs(" = ", file);
	fputs(value, file);

	return true;
}

bool ConfigUtil_ReadStrFromJson(const char *json, const char *key, char *value, size_t size) {
	if (json == NULL || key == NULL || value == NULL || size <= 0)
		return false;

	char finalKey[MAX_KEY + 3];
	SNPRINTF1(finalKey, "\"%s\"", key);
	const char *pos = strstr(json, finalKey);
	if (pos == NULL)
		return false;

	pos += strlen(finalKey);
	while (*pos != '\0') {
		char c = *pos;
		if (!(c == ' ' || c == ':' || c == '"'))
			break;
		pos++;
	}
	if (*pos == '\0')
		return false;

	char *v = value;
	while (*pos != '\0') {
		char c = *pos;
		if (c == '"' || c == ',' || c == '}')
			break;
		if (v - value >= (int)size - 1)
			break;
		*(v++) = *(pos++);
	}
	if (*pos == '\0')
		return false;

	*v = '\0';
	return true;
}
