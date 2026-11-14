#pragma once
#include <stdbool.h>
#include "util/vecmath.h"

struct collision_test {
	bool collision;
	vec3_t vector;
	float t; // fraction of ray travelled
	vec3_t normal;
};

struct collision_test check_triangle_collision(vec3_t ray_origin, vec3_t ray, mat33_t triangle);

vec3_t closest_point_on_segment(vec3_t a, vec3_t b, vec3_t p);

vec3_t closest_point_on_triangle(mat33_t triangle, vec3_t p);
