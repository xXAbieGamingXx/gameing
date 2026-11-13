#define VECMATH_GENERICS
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "parse_map.h"

struct triangle_data* get_triangles(char* file) {
	FILE* flvl = fopen(file, "rb");

	uint32_t vertex_count, tri_count, node_count;
	fread(&vertex_count, sizeof(uint32_t), 1, flvl);
	fread(&tri_count, sizeof(uint32_t), 1, flvl);
	fread(&node_count, sizeof(uint32_t), 1, flvl);

	uint32_t* indices = malloc(tri_count * 3 * sizeof(uint32_t));
	fread(indices, sizeof(uint32_t), tri_count * 3, flvl);

	struct vertex* vertices = malloc(vertex_count * sizeof(struct vertex));
	fread(vertices, sizeof(struct vertex), vertex_count, flvl);

	struct octree_node* nodes = malloc(node_count * sizeof(struct octree_node));
	fread(nodes, sizeof(struct octree_node), node_count, flvl);

	uint32_t entity_count = 0;
	fread(&entity_count, sizeof(uint32_t), 1, flvl);
	struct entity_data* entities = malloc(entity_count * sizeof(struct entity_data));
	struct vertex* ent_vertices = NULL;
	uint32_t* ent_indices = NULL;
	uint32_t ent_vertex_count = 0, ent_tri_count = 0;
	uint32_t* vert_offsets = malloc(entity_count * sizeof(uint32_t));
	uint32_t* tri_offsets = malloc(entity_count * sizeof(uint32_t));
	for(uint32_t i = 0; i < entity_count; i++) {
		struct entity_data* e = &entities[i];
		uint16_t type;
		fread(&type, sizeof(uint16_t), 1, flvl);
		e->type = type;
		float origin[3];
		fread(origin, sizeof(float), 3, flvl);
		e->origin = vec3(origin[0], origin[1], origin[2]);
		e->params = malloc(entity_descs[type].params * sizeof(int32_t));
		fread(e->params, sizeof(int32_t), entity_descs[type].params, flvl);
		fread(&e->vertex_count, sizeof(uint32_t), 1, flvl);
		fread(&e->tri_count, sizeof(uint32_t), 1, flvl);

		vert_offsets[i] = ent_vertex_count;
		tri_offsets[i] = ent_tri_count;
		ent_vertex_count += e->vertex_count;
		ent_tri_count += e->tri_count;

		ent_indices = realloc(ent_indices, ent_tri_count * 3 * sizeof(uint32_t));
		uint32_t* idx = &ent_indices[tri_offsets[i] * 3];
		fread(idx, sizeof(uint32_t), e->tri_count * 3, flvl);
		for(uint32_t j = 0; j < e->tri_count * 3; j++) {
			idx[j] += vert_offsets[i]; // rebase into shared buffer
		}

		ent_vertices = realloc(ent_vertices, ent_vertex_count * sizeof(struct vertex));
		fread(&ent_vertices[vert_offsets[i]], sizeof(struct vertex), e->vertex_count, flvl);
	}

	// point into shared arrays after realloc settles
	for(uint32_t i = 0; i < entity_count; i++) {
		entities[i].vertices = &ent_vertices[vert_offsets[i]];
		entities[i].indices = &ent_indices[tri_offsets[i] * 3];
	}
	free(vert_offsets);
	free(tri_offsets);

	fclose(flvl);

	struct triangle_data* data = malloc(sizeof(struct triangle_data));
	data->vertices = vertices;
	data->vertex_count = vertex_count;
	data->indices = indices;
	data->tri_count = tri_count;
	data->nodes = nodes;
	data->node_count = node_count;
	data->entities = entities;
	data->entity_count = entity_count;
	data->ent_vertices = ent_vertices;
	data->ent_vertex_count = ent_vertex_count;
	data->ent_indices = ent_indices;
	data->ent_tri_count = ent_tri_count;
	data->aabb_nodes = NULL;
	data->aabb_node_count = 0;
	return data;
}

void get_triangle_vertices(struct triangle_data* data, uint32_t tri, struct vertex* out) {
	for(int i = 0; i < 3; i++) {
		out[i] = data->vertices[data->indices[tri * 3 + i]];
	}
}

mat33_t get_triangle_mat33(struct triangle_data* data, uint32_t tri) {
	struct vertex v[3];
	get_triangle_vertices(data, tri, v);
	return mat33(vec3(v[0].x, v[0].y, v[0].z), vec3(v[1].x, v[1].y, v[1].z), vec3(v[2].x, v[2].y, v[2].z));
}

mat33_t get_entity_triangle_mat33(struct triangle_data* data, struct entity* entity, uint32_t tri) {
	vec3_t v[3];
	for(int i = 0; i < 3; i++) {
		struct vertex* vertex = &data->ent_vertices[entity->indices[tri * 3 + i]]; // indices are into ent_vertices
		v[i] = vec3(vertex->x, vertex->y, vertex->z);
	}
	return mat33(v[0], v[1], v[2]);
}
