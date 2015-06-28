#ifndef _A_STAR_HPP_
#define _A_STAR_HPP_

#include "MapInfo.pb.h"
#include "Math.hpp"

// -1: Failure.
// >0: Step count.
int AStar_FindPath(const MapInfo *mapInfo, int32_t map, const Vector2i *start, const Vector2i *end, int maxStep, Vector2i *path, size_t size);

#endif
