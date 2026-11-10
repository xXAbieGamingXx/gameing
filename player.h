#pragma once
#define VECMATH_GENERICS
#include <stdbool.h>
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

struct collision_test {
	bool collision;
	vec3_t vector;
	float t; // fraction of ray travelled
	vec3_t normal;
};

extern struct Player player;

struct octree_node;
struct triangle_data;

void player_add_vector(vec3_t vec);

vec3_t player_direction(); // direction vector

void player_frame(struct octree_node* nodes, struct triangle_data* triangles);

struct collision_test check_triangle_collision(vec3_t ray_origin, vec3_t ray, mat33_t triangle);

struct collision_test check_capsule_collision(vec3_t a, vec3_t b, float radius, mat33_t triangle);

struct collision_test check_capsule_cast(vec3_t a, vec3_t b, float radius, vec3_t movement, mat33_t triangle);
