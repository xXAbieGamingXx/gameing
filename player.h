#pragma once
#define VECMATH_GENERICS
#include <stdbool.h>
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

struct collision_test{
	bool collision;
	vec3_t vector;
};

extern struct Player player;

extern void player_add_vector(vec3_t vec);

extern vec3_t player_direction(); // direction vector

extern void move_player(double delta_time);

struct collision_test check_triangle_collision(vec3_t ray_origin, vec3_t ray, mat33_t triangle);
