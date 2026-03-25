#define VECMATH_GENERICS
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float rx, ry, rz;
} Player;

struct Player player = {0};

vec3 player_direction(){
	return vec3(cos(vm_radians(player.ry))*cos(vm_radians(player.rz)), cos(vm_radians(player.ry))*sin(vm_radians(player.rz)), -sin(vm_radians(player.ry)));
}
