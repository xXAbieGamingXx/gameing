#include <stddef.h>
#include <stdint.h>

#include "entities.h"
#include "parse_map.h"

const struct entity_desc entity_descs[ENTITY_COUNT] = { // must match order of enum
	{ .name = "player_spawn", .params = 3 },
	{ .name = "func_platform", .params = 3 },
};

#ifndef MAP_TOOL

const struct entity_fptrs entity_fptrs[ENTITY_COUNT] = {
	{ .cb = NULL, .collide = NULL, .init = player_spawn_init },
	{ .cb = platform_cb, .collide = NULL, .init = platform_init },
};

#endif

const char* model_paths[ENTITY_COUNT] = {
	"assets/models/Untitled.model", NULL
};

struct model_buffer_index model_buffer_indices[ENTITY_COUNT] = {0};

uint8_t model_instances[ENTITY_COUNT] = {0};

uint16_t total_instances = 0;

void get_brush_pos(struct entity* entity) { // NOTE: works only for cuboids
	bool set_x = false;
	bool set_y = false;
	for(int i = 0; i < entity->vertex_count; i++) {
		if(entity->position->x == 0) {
			entity->position->x = entity->vertices[i].x;
			entity->position->y = entity->vertices[i].y;
			entity->position->z = entity->vertices[i].z;
		} else {
			if(!set_x && entity->vertices[i].x != entity->position->x) {
				entity->position->x = (entity->vertices[i].x + entity->position->x)/2;
				set_x = true;
			}

			if(!set_y && entity->vertices[i].y != entity->position->y) {
				entity->position->y = (entity->vertices[i].y + entity->position->y)/2;
				set_y = true;
			}
			
			if(entity->vertices[i].z > entity->position->z) {
				entity->position->z = entity->vertices[i].z;
			}
		}
	}
}
