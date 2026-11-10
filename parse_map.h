#pragma once
#include <stdint.h>
#include "util/vecmath.h"
#include "octree.h"
#include "entities.h"

struct vertex {
	float x, y, z;
	float u, v;
	float texid;
};

struct entity_data {
	enum entity_type type;
	int32_t* params;
	struct vertex* vertices; // points into triangle_data.ent_vertices
	uint32_t vertex_count;
	uint32_t* indices; // points into triangle_data.ent_indices, already offset
	uint32_t tri_count;
};

struct triangle_data {
	struct vertex* vertices;
	uint32_t vertex_count;
	uint32_t* indices;
	uint32_t tri_count;
	struct octree_node* nodes;
	uint32_t node_count;
	struct entity_data* entities;
	uint32_t entity_count;
	struct vertex* ent_vertices;
	uint32_t ent_vertex_count;
	uint32_t* ent_indices;
	uint32_t ent_tri_count;
};

struct triangle_data* get_triangles(char* file);

void get_triangle_vertices(struct triangle_data* data, uint32_t tri, struct vertex* out);

mat33_t get_triangle_mat33(struct triangle_data* data, uint32_t tri);
