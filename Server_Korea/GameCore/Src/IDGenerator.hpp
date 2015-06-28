#ifndef _ID_GENERATOR_HPP_
#define _ID_GENERATOR_HPP_

#include "Config.hpp"
#include <sys/types.h>
#include <vector>

struct IDGenerator;

struct IDGenerator * IDGenerator_Create(int32_t max, int64_t interval);
void IDGenerator_Destroy(struct IDGenerator *idGenerator);

bool IDGenerator_IsValid(struct IDGenerator *idGenerator, int32_t id);

int32_t IDGenerator_Gen(struct IDGenerator *idGenerator);
void IDGenerator_Release(struct IDGenerator *idGenerator, int32_t id);

template<typename T>
static inline T * SearchBlock(std::vector<T *> &pool, int32_t id) {
	int block = id / Config_PoolStep();
	for (int i = (int)pool.size(); i <= block; i++)
		pool.push_back(new T[Config_PoolStep()]);
	int index = id % Config_PoolStep();
	return &pool[block][index];
}

template<typename T>
static inline void ClearBlock(std::vector<T *> &pool) {
	for (size_t i = 0; i < pool.size(); i++)
		if (pool[i] != NULL)
			delete[] pool[i];
	pool.clear();
}

#endif
