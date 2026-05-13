#define VECMATH_GENERICS
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "util/vecmath.h"

struct vertex {
	float x, y, z;
	uint16_t u, v;
};

struct triangle_desc {
	struct vertex vertices[3];
};

struct triangle_data {
	struct triangle_desc* triangles;
	uint32_t entries;
	uint16_t* texids;
};

#define READ_TRIANGLE_LENGTH 50 

uint16_t texids[10000] = {0};

struct triangle_data* get_triangles(char* file){
	FILE *flvl;
	flvl = fopen(file, "rb");
	fseek(flvl, 0L, SEEK_END);
	uint32_t length = ftell(flvl);
	rewind(flvl);
	char* buffer = malloc(length);
	fgets(buffer, length, flvl);
	fclose(flvl);
	uint32_t entries = length / READ_TRIANGLE_LENGTH;
	struct triangle_desc* triangles = malloc(entries * sizeof(struct triangle_desc));
	uint32_t index = 0;
	for(uint32_t i = 0; i < entries; i++){
		memcpy(&triangles[i], &buffer[index], sizeof(struct triangle_desc));
		index += READ_TRIANGLE_LENGTH; // skip over texid for now
	}
	free(buffer);
	struct triangle_data* triangle_data = malloc(sizeof(struct triangle_data));
	triangle_data->triangles = triangles;
	triangle_data->entries = entries;
	triangle_data->texids = texids;
	return triangle_data;
}
