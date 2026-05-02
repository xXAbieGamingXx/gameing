struct vertex {
	float x, y, z;
	int16_t u, v;
};

struct triangle_desc {
	struct vertex vertices[3];
};

struct triangle_data {
	struct triangle_desc* triangles;
	uint32_t entries;
	uint16_t* texids;
};

// struct vertex;
// struct triangle_desc;
// struct triangle_data;

struct triangle_data* get_triangles(char* file);
