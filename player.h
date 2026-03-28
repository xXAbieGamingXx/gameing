#pragma once
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

extern struct Player player;

extern vec3_t player_direction(); // direction vector
