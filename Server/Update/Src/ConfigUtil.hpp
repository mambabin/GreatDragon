#include <cstdio>

bool ConfigUtil_ReadStr(FILE *file, const char *key, char *value, size_t size);

int ConfigUtil_ExtraToken(char *str, char *tokens[], size_t size, const char *delim);

bool ConfigUtil_WriteStr(FILE *file, const char *key, const char *value);
