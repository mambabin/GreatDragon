#include "IDGenerator.hpp"
#include "Time.hpp"
#include <sys/types.h>
#include <cstddef>
#include <cassert>

using namespace std;

struct ID{
	int32_t id;
	int64_t release;
};

struct IDGenerator{
	int32_t max;
	int64_t interval;
	int32_t cur;
	bool *ids;
	struct ID *idles;
	size_t first;
	size_t last;
};

struct IDGenerator * IDGenerator_Create(int32_t max, int64_t interval) {
	if (max < 0)
		return NULL;

	struct IDGenerator *ret = new struct IDGenerator;
	ret->max = max;
	ret->interval = interval;
	ret->cur = 0;
	ret->ids = new bool[max];
	for (int i = 0; i < max; i++)
		ret->ids[i] = false;
	ret->idles = new struct ID[max];
	ret->first = max;
	ret->last = max;

	return ret;
}

void IDGenerator_Destroy(struct IDGenerator *idGenerator) {
	if (idGenerator->ids != NULL)
		delete[] idGenerator->ids;
	if (idGenerator->idles != NULL)
		delete[] idGenerator->idles;
	if (idGenerator != NULL)
		delete idGenerator;
}

bool IDGenerator_IsValid(struct IDGenerator *idGenerator, int32_t id) {
	if (idGenerator == NULL || id < 0 || id >= idGenerator->cur || !idGenerator->ids[id])
		return false;

	return true;
}

int32_t IDGenerator_Gen(struct IDGenerator *idGenerator) {
	if (idGenerator == NULL)
		return -1;

	if (idGenerator->first != idGenerator->last) {
		if (++idGenerator->first >= (size_t)idGenerator->max)
			idGenerator->first = 0;

		struct ID *id = &idGenerator->idles[idGenerator->first];
		int64_t cur = Time_ElapsedTime();

		if (cur - id->release >= idGenerator->interval) {
			assert(!idGenerator->ids[id->id]);
			idGenerator->ids[id->id] = true;
			return id->id;
		}
		else {
			if (idGenerator->first == 0)
				idGenerator->first = idGenerator->max;
			else
				idGenerator->first--;
		}
	}

	if (idGenerator->cur < idGenerator->max) {
		assert(!idGenerator->ids[idGenerator->cur]);
		idGenerator->ids[idGenerator->cur] = true;
		return idGenerator->cur++;
	}

	return -1;
}

void IDGenerator_Release(struct IDGenerator *idGenerator, int32_t id) {
	if (!IDGenerator_IsValid(idGenerator, id))
		return;

	if (++idGenerator->last >= (size_t)idGenerator->max)
		idGenerator->last = 0;
	assert(idGenerator->first != idGenerator->last);

	int64_t cur = Time_ElapsedTime();

	idGenerator->idles[idGenerator->last].id = id;
	idGenerator->idles[idGenerator->last].release = cur;

	idGenerator->ids[id] = false;
}
