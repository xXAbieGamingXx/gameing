#pragma once

#include <stdint.h>

#include "util/vecmath.h"

#include "entities/platform.h"
#include "entities/player_spawn.h"

enum entity_type {
	ENTITY_PLAYER_SPAWN,
	ENTITY_PLATFORM,
	ENTITY_COUNT
};

enum split_axis {
	SPLIT_X,
	SPLIT_Y,
	SPLIT_Z,
	LEAF
};

struct entity {
	int16_t instance_id;
	vec3_t* position;
	vec3_t prev_position;
	enum entity_type type;
	int32_t* params;
	struct vertex* vertices;
	uint32_t vertex_count;
	uint32_t* indices;
	uint32_t tri_count;
	void (*cb)(struct entity*);
	void (*collide)(struct entity*);
	void* entity_values; // ptr to entity specific struct
	float rx, rz; // only used if 3d model
	bool visible;
};

struct entity_desc {
	char* name;
	int params;
};

struct entity_fptrs {
	void (*cb)(struct entity*);
	void (*collide)(struct entity*);
	void (*init)(struct entity*);
};

struct aabb_node {
	enum split_axis axis;
	float min[3], max[3];
	uint32_t children[2];
	struct entity* entity;
};

struct model_buffer_index {
	uint32_t index_count;
	uint32_t vertex_count;
	uint32_t index_start;
	uint32_t vertex_start;
};

extern const struct entity_desc entity_descs[ENTITY_COUNT];

#ifndef MAP_TOOL
extern const struct entity_fptrs entity_fptrs[ENTITY_COUNT];
#endif

extern const char* model_paths[ENTITY_COUNT];

extern uint8_t model_instances[ENTITY_COUNT];

extern struct model_buffer_index model_buffer_indices[ENTITY_COUNT];

extern uint16_t total_instances;

void get_brush_pos(struct entity* entity);
