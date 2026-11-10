#pragma once
#include <stdint.h>

#define OCTREE_MAX_DEPTH 8
#define OCTREE_LEAF_TRIS 24

struct octree_node { // 0 is root
	float min[3], max[3];
	uint32_t children[8]; // if value is 0, no child
	uint32_t first_tri, tri_count;
};
