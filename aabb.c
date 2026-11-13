#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "aabb.h"
#include "parse_map.h"

void entity_bounds(struct entity* entity, float min[3], float max[3]) {
	for(int i = 0; i < 3; i++) {
		min[i] = INFINITY;
		max[i] = -INFINITY;
	}
	for(uint32_t i = 0; i < entity->vertex_count; i++) {
		float vertex[3] = { entity->vertices[i].x, entity->vertices[i].y, entity->vertices[i].z };
		for(int j = 0; j < 3; j++) {
			min[j] = fminf(min[j], vertex[j]);
			max[j] = fmaxf(max[j], vertex[j]);
		}
	}
}

void merge_bounds(struct aabb_node* node, struct aabb_node* a, struct aabb_node* b) {
	for(int i = 0; i < 3; i++) {
		node->min[i] = fminf(a->min[i], b->min[i]);
		node->max[i] = fmaxf(a->max[i], b->max[i]);
	}
}

static int sort_axis;

int compare_centers(const void* a, const void* b) {
	struct aabb_node* na = *(struct aabb_node**)a;
	struct aabb_node* nb = *(struct aabb_node**)b;
	float ca = na->min[sort_axis] + na->max[sort_axis];
	float cb = nb->min[sort_axis] + nb->max[sort_axis];
	return (ca > cb) - (ca < cb);
}

uint32_t build_node(struct aabb_node* nodes, uint32_t* next, struct aabb_node** leaves, uint32_t count) {
	uint32_t index = (*next)++;
	struct aabb_node* node = &nodes[index];

	if(count == 1) {
		*node = *leaves[0];
		return index;
	}

	float min[3] = { INFINITY, INFINITY, INFINITY };
	float max[3] = { -INFINITY, -INFINITY, -INFINITY };
	for(uint32_t i = 0; i < count; i++) {
		for(int a = 0; a < 3; a++) {
			float c = leaves[i]->min[a] + leaves[i]->max[a];
			min[a] = fminf(min[a], c);
			max[a] = fmaxf(max[a], c);
		}
	}

	int axis = SPLIT_X;
	for(int i = 1; i < 3; i++) {
		if(max[i] - min[i] > max[axis] - min[axis]) {
			axis = i;
		}
	}

	sort_axis = axis;
	qsort(leaves, count, sizeof(struct aabb_node*), compare_centers);

	uint32_t half = count / 2;
	node->axis = axis;
	node->entity = NULL;
	node->children[0] = build_node(nodes, next, leaves, half);
	node->children[1] = build_node(nodes, next, leaves + half, count - half);
	merge_bounds(node, &nodes[node->children[0]], &nodes[node->children[1]]);
	return index;
}

uint32_t aabb_build(struct entity* entities, uint32_t entity_count, struct aabb_node** out) {
	uint32_t leaf_count = 0;
	for(uint32_t i = 0; i < entity_count; i++) {
		if(entities[i].vertex_count > 0) {
			leaf_count++;
		}
	}

	*out = NULL;
	if(leaf_count == 0) {
		return 0;
	}

	struct aabb_node* leaf_nodes = malloc(leaf_count * sizeof(struct aabb_node));
	struct aabb_node** leaves = malloc(leaf_count * sizeof(struct aabb_node*));
	uint32_t leaf_index = 0;
	for(uint32_t i = 0; i < entity_count; i++) {
		if(entities[i].vertex_count == 0) {
			continue;
		}
		leaf_nodes[leaf_index] = (struct aabb_node) { .axis = LEAF, .entity = &entities[i] };
		entity_bounds(&entities[i], leaf_nodes[leaf_index].min, leaf_nodes[leaf_index].max);
		leaves[leaf_index] = &leaf_nodes[leaf_index];
		leaf_index++;
	}

	uint32_t node_count = leaf_count * 2 - 1;
	struct aabb_node* nodes = malloc(node_count * sizeof(struct aabb_node));
	uint32_t next = 0;
	build_node(nodes, &next, leaves, leaf_count);

	free(leaves);
	free(leaf_nodes);
	*out = nodes;
	return node_count;
}

void aabb_refit(struct aabb_node* nodes, uint32_t node_count) {
	for(uint32_t i = node_count; i-- > 0;) {
		struct aabb_node* node = &nodes[i];
		if(node->axis == LEAF) {
			entity_bounds(node->entity, node->min, node->max);
		} else {
			merge_bounds(node, &nodes[node->children[0]], &nodes[node->children[1]]);
		}
	}
}

bool overlaps(struct aabb_node* node, float min[3], float max[3]) {
	for(int i = 0; i < 3; i++) {
		if(node->min[i] > max[i] || node->max[i] < min[i]) {
			return false;
		}
	}
	return true;
}

uint32_t aabb_query(struct aabb_node* nodes, uint32_t node_count, float min[3], float max[3], struct entity** out, uint32_t max_out) {
	if(node_count == 0) {
		return 0;
	}

	uint32_t stack[AABB_MAX_DEPTH];
	int top = 0;
	uint32_t hits = 0;
	stack[top++] = 0;

	while(top > 0 && hits < max_out) {
		struct aabb_node* node = &nodes[stack[--top]];
		if(!overlaps(node, min, max)) {
			continue;
		}
		if(node->axis == LEAF) {
			out[hits++] = node->entity;
		} else {
			stack[top++] = node->children[1];
			stack[top++] = node->children[0];
		}
	}
	return hits;
}
