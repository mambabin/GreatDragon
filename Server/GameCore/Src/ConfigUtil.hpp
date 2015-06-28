#include <cstdio>

bool ConfigUtil_ReadStr(FILE *file, const char *key, char *value, size_t size);

bool ConfigUtil_Key(FILE *file, int n, char *key, size_t size);

int ConfigUtil_ExtraToken(char *str, char *tokens[], size_t size, const char *delim);

bool ConfigUtil_WriteStr(FILE *file, const char *key, const char *value);

bool ConfigUtil_ReadStrFromJson(const char *json, const char *key, char *value, size_t size);
