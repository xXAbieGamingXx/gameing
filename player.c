#define VECMATH_GENERICS
#include "util/vecmath.h"
#include "input.h"
#include "sokol_app.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

#define WALKING_SPEED 10

struct Player player = {0};

void player_add_vector(vec3_t vec){
	player.x += vec.x;
	player.y += vec.y;
	player.z += vec.z;
}

vec3_t player_direction(){
	float cos_rz = cos(vm_radians(player.rz));  // horizontal rotation
	return vec3(cos_rz * cos(vm_radians(player.ry)),  // ry is pitch, affects Z only
		sin(vm_radians(player.rz)) * cos(vm_radians(player.ry)),
		-sin(vm_radians(player.ry)));
}

void move_player(double delta_time){
	float direction = key_down(SAPP_KEYCODE_W) - key_down(SAPP_KEYCODE_S);

	vec3_t x_movement = player_direction();
	x_movement.z = 0;
	float factor = pow(x_movement.x, 2) + pow(x_movement.y, 2);
	float multiplier = direction * WALKING_SPEED * delta_time / factor;

	x_movement.x *= multiplier;
	x_movement.y *= multiplier;
	player_add_vector(x_movement);

	direction = key_down(SAPP_KEYCODE_A) - key_down(SAPP_KEYCODE_D);
	player.rz += 90;
	vec3_t y_movement = player_direction();
	player.rz -= 90;
	y_movement.z = 0;
	// factor will be the same
	multiplier = direction * WALKING_SPEED * delta_time / factor;

	y_movement.x *= multiplier;
	y_movement.y *= multiplier;
	player_add_vector(y_movement);
}
