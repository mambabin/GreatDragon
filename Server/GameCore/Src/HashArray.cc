#include "HashArray.hpp"
#include <cstdlib>
#include <cassert>
#include <cstdio>

struct Node{
	void *ud;
	struct Node *next;
};

struct HashArray{
	struct Node **head;
	int max;
	struct Node *pool;
	struct Node *idle;
	int total;
	int count;
	size_t delta;
};

struct HashArray * HashArray_Create(int max, size_t delta) {
	if (max <= 0 || delta <= 0)
		return NULL;

	struct HashArray *ha = (struct HashArray *)malloc(sizeof(struct HashArray));
	if (ha == NULL)
		return NULL;

	ha->delta = delta;

	ha->max = max;
	ha->head = (struct Node **)malloc(sizeof(struct Node *) * ha->max);
	if (ha->head == NULL) {
		free(ha);
		return NULL;
	}
	for (int i = 0; i < ha->max; i++)
		ha->head[i] = NULL;

	ha->total = max * 2;
	ha->pool = (struct Node *)malloc(sizeof(struct Node) * ha->total);
	if (ha->pool == NULL) {
		free(ha->head);
		free(ha);
	}
	ha->idle = ha->pool;
	for (int i = 0; i < ha->total - 1; i++)
		ha->idle[i].next = &ha->idle[i + 1];
	ha->idle[ha->total - 1].next = NULL;

	ha->count = 0;

	return ha;
}

void HashArray_Destroy(struct HashArray *ha) {
	if (ha == NULL)
		return;
 
	free(ha->head);
	free(ha->pool);
	free(ha);
}

int HashArray_Count(struct HashArray *ha) {
	if (ha == NULL)
		return -1;
	
	return ha->count;
}

static inline int Hash(void *ptr, size_t delta, int size) {
	return (int)(((size_t)ptr / delta) % size);
}


bool HashArray_Has(struct HashArray *ha, void *ud) {
	if (ha == NULL || ud == NULL)
		return false;

	int hash = Hash(ud, ha->delta, ha->max);
	for (struct Node *node = ha->head[hash]; node != NULL; node = node->next) {
		if (node->ud == ud)
			return true;
	}

	return false;
}

int HashArray_All(struct HashArray *ha, void **ud, size_t size) {
	if (ha == NULL || ud == NULL || size <= 0)
		return -1;

	if (ha->count > (int)size)
		return -1;

	int count = 0;
	for (int i = 0; i < ha->max; i++) {
		for (struct Node *node = ha->head[i]; node != NULL; node = node->next) {
			ud[count++] = node->ud;
		};
	}

	return count;
}

static bool Expand(struct HashArray *ha) {
	int newTotal = ha->total * 2;
	struct Node *cur = (struct Node *)realloc(ha->pool, sizeof(struct Node) * newTotal);
	if (cur == NULL)
		return false;

	if (cur != ha->pool) {
		for (int i = 0; i < ha->max; i++) {
			if (ha->head[i] != NULL)
				ha->head[i] = (struct Node *)(cur + (ha->head[i] - ha->pool));
		}

		for (int i = 0; i < ha->total; i++) {
			struct Node *node = &cur[i];
			if (node->next != NULL)
				node->next = (struct Node *)(cur + (node->next - ha->pool));
		}
	}

	for (int i = ha->total; i < newTotal - 1; i++)
		cur[i].next = cur + i + 1;
	cur[newTotal - 1].next = NULL;

	ha->pool = cur;
	ha->idle = cur + ha->total;
	ha->total = newTotal;

	return true;
}

int HashArray_Add(struct HashArray *ha, void *ud) {
	if (ha == NULL || ud == NULL)
		return -1;

	if (ha->idle == NULL) {
		if (!Expand(ha))
			return -1;
	}
	struct Node *idle = ha->idle;
	ha->idle = ha->idle->next;

	int hash = Hash(ud, ha->delta, ha->max);

	idle->next = ha->head[hash];
	ha->head[hash] = idle;
	idle->ud = ud;

	ha->count++;

	return 0;
}

void HashArray_Del(struct HashArray *ha, void *ud) {
	if (ha == NULL || ud == NULL)
		return;

	int hash = Hash(ud, ha->delta, ha->max);
	for (struct Node *node = ha->head[hash], *prev = NULL; node != NULL;) {
		if (node->ud == ud) {
			if (prev == NULL) {
				ha->head[hash] = node->next;
				node->next = ha->idle;
				ha->idle = node;
				node = ha->head[hash];
			}
			else {
				prev->next = node->next;
				node->next = ha->idle;
				ha->idle = node;
				node = prev->next;
			}
			ha->count--;
			continue;
		}

		prev = node;
		node = node->next;
	}
}

void HashArray_Clear(struct HashArray *ha) {
	if (ha == NULL)
		return;

	for (int i = 0; i < ha->max; i++)
		ha->head[i] = NULL;

	ha->idle = ha->pool;
	for (int i = 0; i < ha->total - 1; i++)
		ha->idle[i].next = &ha->idle[i + 1];
	ha->idle[ha->total - 1].next = NULL;

	ha->count = 0;
}

void HashArray_Dump(struct HashArray *ha) {
	if (ha == NULL)
		return;

	printf("--head--\n");
	for (int i = 0; i < ha->max; i++) {
		if (ha->head[i] == NULL)
			continue;

		printf("[%d]", i);
		for (struct Node *node = ha->head[i]; node != NULL; node = node->next) {
			printf("(self: %p, ud: %p next: %p) ", node, node->ud, node->next);
		}
		printf("\n");
	}

	printf("--idle--\n");
	for (struct Node *node = ha->idle; node != NULL; node = node->next) {
		printf("(self: %p, next: %p) ", node, node->next);
	}
	printf("\n");
}
