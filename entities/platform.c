#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "entities.h"
#include "constants.h"
#include "platform.h"
#include "parse_map.h"

// #define PLATFORM_SPEED (150.0f/TICKS_PER_SEC)
#define PLATFORM_SPEED 3.0f
#define PLATFORM_PAUSE 1*TICKS_PER_SEC + 1
#define PLATFORM_X 0
#define PLATFORM_Y 1
#define PLATFORM_Z 2

struct platform_values {
	float x, y, z; // x and y are middle, z is top
	float start_x, start_y, start_z; // x and y are middle, z is top
	uint8_t ticks_stopped;
	bool backtracking;
};

void platform_init(struct entity* entity) {
	struct platform_values* values = malloc(sizeof(struct platform_values));
	memset(values, 0, sizeof(struct platform_values));
	entity->entity_values = (void*)values;
	bool set_x = false;
	bool set_y = false;
	for(int i = 0; i < entity->vertex_count; i++) {
		if(values->x == 0) {
			values->x = entity->vertices[i].x;
			values->y = entity->vertices[i].y;
			values->z = entity->vertices[i].z;
		} else {
			if(!set_x && entity->vertices[i].x != values->x) {
				values->x = (entity->vertices[i].x + values->x)/2;
				set_x = true;
			}

			if(!set_y && entity->vertices[i].y != values->y) {
				values->y = (entity->vertices[i].y + values->y)/2;
				set_y = true;
			}
			
			if(entity->vertices[i].z > values->z) {
				values->z = entity->vertices[i].z;
			}
		}
	}

	values->start_x = values->x;
	values->start_y = values->y;
	values->start_z = values->z;


	entity->params[0] += values->x;
	entity->params[1] += values->y;
	entity->params[2] += values->z;
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
		vec3_t position = vec3(values->x, values->y, values->z);
		vec3_t destination;

		if(values->backtracking) {
			destination = vec3(values->start_x, values->start_y, values->start_z);
		} else {
			destination = vec3(entity->params[0], entity->params[1], entity->params[2]);
		}
		vec3_t movement = vec3_sub(destination, position);
		movement = vec3_normalize(movement);
		if(vec3_distance(position, destination) <= PLATFORM_SPEED) {
			movement = vec3_mulf(movement, vec3_distance(position, destination));
			values->ticks_stopped = 1;
		} else {
			movement = vec3_mulf(movement, PLATFORM_SPEED);
		}
		for(int i = 0; i < entity->vertex_count; i++) {
			entity->vertices[i].x += movement.x;
			entity->vertices[i].y += movement.y;
			entity->vertices[i].z += movement.z;
		}
		values->x += movement.x;
		values->y += movement.y;
		values->z += movement.z;
	}
}
