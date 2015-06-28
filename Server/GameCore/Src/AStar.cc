#include "AStar.hpp"
#include "MapInfoManager.hpp"
#include "MapInfo.pb.h"
#include "Math.hpp"
#include <cmath>

struct Node{
	Vector2i coord;
	int g;
	int h;
	int f;
	struct Node *parent;

	struct Node *left;
	struct Node *right;
};

struct Set{
	struct Node *root;
	int count;
};

#define MAXNODES 1024

static struct{
	struct Node nodes[MAXNODES];
	int cur;

	struct Set open;
	struct Set close;
}pool;


static inline void Set_Reset(struct Set *set) {
	set->root = NULL;
	set->count = 0;
}

static struct Node * Set_Search(const struct Set *set, const struct Node *node) {
	struct Node *cur = set->root;

	while (cur != NULL) {
		if (cur->coord.x() == node->coord.x() && cur->coord.y() == node->coord.y())
			return cur;

		if (node->f < cur->f)
			cur = cur->left;
		else
			cur = cur->right;
	}

	return NULL;
}

static struct Node * Set_Min(const struct Set *set) {
	struct Node *cur = set->root;
	struct Node *prev = NULL;

	while (cur != NULL) {
		prev = cur;
		cur = cur->left;
	}

	return prev;
}

static void Set_Add(struct Set *set, struct Node *node) {
	node->left = node->right = NULL;

	struct Node *cur = set->root;
	struct Node *prev = NULL;

	while (cur != NULL) {
		prev = cur;

		if (node->f < cur->f)
			cur = cur->left;
		else
			cur = cur->right;
	}

	if (prev == NULL) {
		set->root = node;
	}
	else {
		if (node->f < prev->f)
			prev->left = node;
		else
			prev->right = node;
	}

	set->count++;
}

static void Set_Del(struct Set *set, const struct Node *node) {
	struct Node *cur = set->root;
	struct Node *prev = NULL;

	while (cur != NULL) {
		if (cur->coord.x() == node->coord.x() && cur->coord.y() == node->coord.y())
			break;

		prev = cur;

		if (node->f < cur->f)
			cur = cur->left;
		else
			cur = cur->right;
	}

	if (cur == NULL)
		return;

	if (cur->right == NULL) {
		if (prev == NULL)
			set->root = cur->left;
		else if (cur == prev->left)
			prev->left = cur->left;
		else
			prev->right = cur->left;
	}
	else if (cur->left == NULL) {
		if (prev == NULL)
			set->root = cur->right;
		else if (cur == prev->left)
			prev->left = cur->right;
		else
			prev->right = cur->right;
	}
	else {
		struct Node *leaf = cur->left;

		while (leaf->right != NULL)
			leaf = leaf->right;
		leaf->right = cur->right;

		if (prev == NULL)
			set->root = cur->left;
		else if (cur == prev->left)
			prev->left = cur->left;
		else
			prev->right = cur->left;
	}

	set->count--;
}

static inline bool Set_Empty(struct Set *set) {
	return set->root == NULL;
}

static inline bool Set_Count(struct Set *set) {
	return set->count;
}

static struct Node * GenNode(const Vector2i *coord, int g, int h, struct Node *parent) {
	if (pool.cur >= MAXNODES)
		return NULL;

	struct Node *node = &pool.nodes[pool.cur++];
	node->coord.set_x(coord->x());
	node->coord.set_y(coord->y());
	node->g = g;
	node->h = h;
	node->f = g + h;
	node->parent = parent;
	node->left = node->right = NULL;

	return node;
}

static inline int CalculateH(const Vector2i *cur, const Vector2i *end) {
	return 10 * (abs(cur->x() - end->x()) + abs(cur->y() - end->y()));
}

static int ProcessBlock(const Vector2i *coord, int g, int h, struct Node *cur) {
	struct Node *node = GenNode(coord, g, h, cur);
	if (node == NULL)
		return -1;

	if (Set_Search(&pool.close, node) != NULL)
		return 0;

	struct Node *node_ = Set_Search(&pool.open, node);
	if (node_ == NULL) {
		Set_Add(&pool.open, node);
	}
	else {
		if (node->g < node_->g) {
			Set_Del(&pool.open, node_);
			node_->g = node->g;
			node_->f = node_->g + node_->h;
			node_->parent = cur;
			Set_Add(&pool.open, node_);
		}
	}

	return 0;
}

static int ProcessNearBlocks(const MapInfo *mapInfo, int32_t map, const BlockInfo *blockInfo, struct Node *cur, const Vector2i *end) {
	bool up = false, right = false, bottom = false, left = false;

	// Up.
	if (cur->coord.y() + 1 < blockInfo->logicLength()) {
		Vector2i coord;
		coord.set_x(cur->coord.x());
		coord.set_y(cur->coord.y() + 1);
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 10, CalculateH(&coord, end), cur) == -1)
				return -1;;
			up = true;
		}
	}

	// Right.
	if (cur->coord.x() + 1 < blockInfo->logicWidth()) {
		Vector2i coord;
		coord.set_x(cur->coord.x() + 1);
		coord.set_y(cur->coord.y());
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 10, CalculateH(&coord, end), cur) == -1)
				return -1;
			right = true;
		}
	}

	// Bottom.
	if (cur->coord.y() - 1 >= 0) {
		Vector2i coord;
		coord.set_x(cur->coord.x());
		coord.set_y(cur->coord.y() - 1);
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 10, CalculateH(&coord, end), cur) == -1)
				return -1;
			bottom = true;
		}
	}

	// Left.
	if (cur->coord.x() - 1 >= 0) {
		Vector2i coord;
		coord.set_x(cur->coord.x() - 1);
		coord.set_y(cur->coord.y());
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 10, CalculateH(&coord, end), cur) == -1)
				return -1;
			left = true;
		}
	}

	// Up-Right.
	if (up && right) {
		Vector2i coord;
		coord.set_x(cur->coord.x() + 1);
		coord.set_y(cur->coord.y() + 1);
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 14, CalculateH(&coord, end), cur) == -1)
				return -1;
		}
	}

	// Bottom-Right.
	if (bottom && right) {
		Vector2i coord;
		coord.set_x(cur->coord.x() + 1);
		coord.set_y(cur->coord.y() - 1);
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 14, CalculateH(&coord, end), cur) == -1)
				return -1;
		}
	}

	// Bottom-Left.
	if (bottom && left) {
		Vector2i coord;
		coord.set_x(cur->coord.x() - 1);
		coord.set_y(cur->coord.y() - 1);
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 14, CalculateH(&coord, end), cur) == -1)
				return -1;
		}
	}

	// Up-Left.
	if (up && left) {
		Vector2i coord;
		coord.set_x(cur->coord.x() - 1);
		coord.set_y(cur->coord.y() + 1);
		if (!MapInfoManager_Unwalkable(mapInfo, &coord, map)) {
			if (ProcessBlock(&coord, cur->g + 14, CalculateH(&coord, end), cur) == -1)
				return -1;
		}
	}

	return 0;
}

static void Reset() {
	pool.cur = 0;
	Set_Reset(&pool.open);
	Set_Reset(&pool.close);
}

int AStar_FindPath(const MapInfo *mapInfo, int32_t map, const Vector2i *start, const Vector2i *end, int maxStep, Vector2i *path, size_t size) {
	if (mapInfo == NULL || start == NULL || end == NULL || path == NULL)
		return -1;

	if (MapInfoManager_Unwalkable(mapInfo, start, map))
		return -1;
	if (MapInfoManager_Unwalkable(mapInfo, end, map))
		return -1;

	const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
	assert(blockInfo != NULL);

	Reset();

	Set_Add(&pool.open, GenNode(start, 0, CalculateH(start, end), NULL));

	while (!Set_Empty(&pool.open)) {
		struct Node *cur = Set_Min(&pool.open);
		Set_Del(&pool.open, cur);
		Set_Add(&pool.close, cur);

		if (Set_Count(&pool.close) > maxStep)
			return -1;

		if (cur->coord.x() == end->x() && cur->coord.y() == end->y()) {
			struct Node *t = cur;
			int num = 0;

			do{
				t = t->parent;
				num++;
			}while (t != NULL);

			if ((int)size < num)
				return -1;

			for (int i = num - 1; i >= 0; i--) {
				path[i] = cur->coord;
				cur = cur->parent;
			}

			return num;
		}

		if (ProcessNearBlocks(mapInfo, map, blockInfo, cur, end) == -1)
			return -1;
	}

	return -1;
}
