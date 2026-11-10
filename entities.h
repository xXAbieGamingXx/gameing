#pragma once

#include <stdint.h>

#include "entities/platform.h"

enum entity_type {
	ENTITY_PLAYER_SPAWN,
	ENTITY_PLATFORM,
	ENTITY_COUNT
};

struct entity {
	enum entity_type type;
	int32_t* params;
	struct vertex* vertices;
	uint32_t vertex_count;
	uint32_t* indices;
	uint32_t tri_count;
	void (*cb)(struct entity*);
	void* entity_values; // ptr to entity specific struct
};

struct entity_desc {
	char* name;
	int params;
};

struct entity_fptrs {
	void (*cb)(struct entity*);
	void (*init)(struct entity*);
};

static const struct entity_desc entity_descs[ENTITY_COUNT] = { // must match order of enum
	{ .name = "player_spawn", .params = 3 },
	{ .name = "func_platform", .params = 3 },
};

#ifndef MAP_TOOL

static const struct entity_fptrs entity_fptrs[ENTITY_COUNT] =  {
	{ .cb = NULL, .init = NULL },
	{ .cb = platform_cb, .init = platform_init },
};

#endif
