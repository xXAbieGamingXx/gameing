#define VECMATH_GENERICS
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

struct Player player = {0};

vec3_t player_direction(){
	return vec3(cos(vm_radians(player.ry))*cos(vm_radians(player.rz)), cos(vm_radians(player.ry))*sin(vm_radians(player.rz)), -sin(vm_radians(player.ry)));
}
