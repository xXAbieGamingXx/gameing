#pragma once
#include <stdint.h>
#include "entities.h"

#define AABB_MAX_DEPTH 64

// builds a tree over entities with geometry, returns node count, *out is malloced
uint32_t aabb_build(struct entity* entities, uint32_t entity_count, struct aabb_node** out);

// recompute bounds after entities move
void aabb_refit(struct aabb_node* nodes, uint32_t node_count);

// fills out with entities whose bounds overlap min/max, returns hit count
uint32_t aabb_query(struct aabb_node* nodes, uint32_t node_count, float min[3], float max[3], struct entity** out, uint32_t max_out);
