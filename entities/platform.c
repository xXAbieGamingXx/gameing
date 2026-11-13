#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "entities.h"
#include "constants.h"
#include "platform.h"
#include "parse_map.h"

#define PLATFORM_SPEED (150.0f/TICKS_PER_SEC)
#define PLATFORM_PAUSE 1*TICKS_PER_SEC + 1
#define PLATFORM_X 0
#define PLATFORM_Y 1
#define PLATFORM_Z 2

struct platform_values {
	float start_x, start_y, start_z; // top middle of cuboid
	uint8_t ticks_stopped;
	bool backtracking;
};

void platform_init(struct entity* entity) {
	entity->rx = 0.0f;
	entity->rz = 0.0f;
	entity->visible = true;
	struct platform_values* values = malloc(sizeof(struct platform_values));
	memset(values, 0, sizeof(struct platform_values));
	entity->entity_values = (void*)values;
	get_brush_pos(entity);

	values->start_x = entity->position->x;
	values->start_y = entity->position->y;
	values->start_z = entity->position->z;


	entity->params[0] += entity->position->x;
	entity->params[1] += entity->position->y;
	entity->params[2] += entity->position->z;
}

void platform_cb(struct entity* entity) {
	struct platform_values* values = (struct platform_values*)entity->entity_values;
	if(values->ticks_stopped > 0) {
		values->ticks_stopped++;
		if(values->ticks_stopped >= PLATFORM_PAUSE) {
			values->ticks_stopped = 0;
			values->backtracking = !values->backtracking;
		}
	} else {
		struct platform_values* values = entity->entity_values;
		vec3_t destination;

		if(values->backtracking) {
			destination = vec3(values->start_x, values->start_y, values->start_z);
		} else {
			destination = vec3(entity->params[0], entity->params[1], entity->params[2]);
		}
		vec3_t movement = vec3_sub(destination, *entity->position);
		movement = vec3_normalize(movement);
		if(vec3_distance(*entity->position, destination) <= PLATFORM_SPEED) {
			movement = vec3_mulf(movement, vec3_distance(*entity->position, destination));
			values->ticks_stopped = 1;
		} else {
			movement = vec3_mulf(movement, PLATFORM_SPEED);
		}
		for(int i = 0; i < entity->vertex_count; i++) {
			entity->vertices[i].x += movement.x;
			entity->vertices[i].y += movement.y;
			entity->vertices[i].z += movement.z;
		}
		entity->position->x += movement.x;
		entity->position->y += movement.y;
		entity->position->z += movement.z;
	}
}
