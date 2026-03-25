#pragma once
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

extern struct Player player;

vec3 player_direction(); // direction vector
