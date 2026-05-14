#define VECMATH_GENERICS
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "parse_map.h"

struct triangle_data* get_triangles(char* file){
	FILE* flvl = fopen(file, "rb");

	uint32_t vertex_count, tri_count;
	fread(&vertex_count, sizeof(uint32_t), 1, flvl);
	fread(&tri_count, sizeof(uint32_t), 1, flvl);

	uint16_t* texids = malloc(tri_count * sizeof(uint16_t));
	fread(texids, sizeof(uint16_t), tri_count, flvl);

	uint32_t* indices = malloc(tri_count * 3 * sizeof(uint32_t));
	fread(indices, sizeof(uint32_t), tri_count * 3, flvl);

	struct vertex* vertices = malloc(vertex_count * sizeof(struct vertex));
	fread(vertices, sizeof(struct vertex), vertex_count, flvl);

	fclose(flvl);

	struct triangle_data* td = malloc(sizeof(struct triangle_data));
	td->vertices = vertices;
	td->vertex_count = vertex_count;
	td->indices = indices;
	td->texids = texids;
	td->tri_count = tri_count;
	return td;
}
