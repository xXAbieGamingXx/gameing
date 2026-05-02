#define VECMATH_GENERICS
#include <float.h>
#include "util/vecmath.h"
#include "input.h"
#include "sokol_app.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

#define WALKING_SPEED 10

struct Player player = {0};

struct collision_test {
	bool collision;
	vec3_t vector;
};

void player_add_vector(vec3_t vec){
	player.x += vec.x;
	player.y += vec.y;
	player.z += vec.z;
}

vec3_t player_direction(){
	return vec3(cos(vm_radians(player.rz)) * cos(vm_radians(player.ry)),
		sin(vm_radians(player.rz)) * cos(vm_radians(player.ry)),
		-sin(vm_radians(player.ry)));
}

#ifndef TEST
void move_player(double delta_time){
	float direction = key_down(SAPP_KEYCODE_W) - key_down(SAPP_KEYCODE_S);

	vec3_t x_movement = player_direction();
	x_movement.z = 0;
	float factor = x_movement.x * x_movement.x + x_movement.y * x_movement.y;
	float multiplier = 0.0f;
	if (factor > 1e-6f) {
		multiplier = direction * WALKING_SPEED * delta_time / factor;
	}

	x_movement.x *= multiplier;
	x_movement.y *= multiplier;
	player_add_vector(x_movement);

	direction = key_down(SAPP_KEYCODE_A) - key_down(SAPP_KEYCODE_D);
	vec3_t y_movement = vec3(-x_movement.y, x_movement.x, 0.0f);
	// factor will be the same
	multiplier = direction * WALKING_SPEED * delta_time / factor;

	y_movement.x *= multiplier;
	y_movement.y *= multiplier;
	player_add_vector(y_movement);
}
#endif

struct collision_test check_triangle_collision(vec3_t ray_origin, vec3_t ray, mat33_t triangle){
	const float epsilon = FLT_EPSILON;
	vec3_t edge1 = vec3_sub(triangle.y, triangle.x);
	vec3_t edge2 = vec3_sub(triangle.z, triangle.x);

	vec3_t pvec = vec3_cross(ray, edge2);
	float det = vec3_dot(edge1, pvec);

	if(det < epsilon){
		return (struct collision_test){.collision = false};
	}

	vec3_t tvec = vec3_sub(ray_origin, triangle.x);

	float u = vec3_dot(tvec, pvec);

	if(u < 0.0 || u > det){
		return (struct collision_test){.collision = false};
	}

	vec3_t qvec = vec3_cross(tvec, edge1);
	
	float v = vec3_dot(ray, qvec);

	if(v < 0.0 || u + v > det){
		return (struct collision_test){.collision = false};
	}

	u /= det;
	v /= det;

	vec3_t vector = vec3_add(vec3_add(vec3_mulf(triangle.x, 1.0f - u - v), vec3_mulf(triangle.y, u)), vec3_mulf(triangle.z, v));

	return (struct collision_test){
		.collision = true,
		.vector = vector,
	};
}
