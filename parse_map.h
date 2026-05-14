#include <stdint.h>

struct vertex {
	float x, y, z;
	float u, v;
};

struct triangle_data {
	struct vertex* vertices;
	uint32_t vertex_count;
	uint32_t* indices;
	uint16_t* texids;
	uint32_t tri_count;
};

struct triangle_data* get_triangles(char* file);
