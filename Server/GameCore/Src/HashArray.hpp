#ifndef _HASH_ARRAY_
#define _HASH_ARRAY_

#include <cstddef>

struct HashArray;

// int p[3];
// struct HashArray *ha = HashArray_Create(3, sizeof(p[0]));
struct HashArray * HashArray_Create(int max, size_t delta);
void HashArray_Destroy(struct HashArray *ha);

int HashArray_Count(struct HashArray *ha);

bool HashArray_Has(struct HashArray *ha, void *ud);

int HashArray_All(struct HashArray *ha, void **ud, size_t size);

int HashArray_Add(struct HashArray *ha, void *ud);
void HashArray_Del(struct HashArray *ha, void *ud);
void HashArray_Clear(struct HashArray *ha);

void HashArray_Dump(struct HashArray *ha);

#endif
