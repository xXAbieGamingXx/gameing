#define VECMATH_GENERICS
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

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
