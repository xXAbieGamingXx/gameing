#include <stdint.h>

struct vertex {
	float x, y, z;
	float u, v;
	float texid;
};

struct triangle_data {
	struct vertex* vertices;
	uint32_t vertex_count;
	uint32_t* indices;
	uint32_t tri_count;
};

struct triangle_data* get_triangles(char* file);
