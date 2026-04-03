#pragma once
#include "util/vecmath.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

extern struct Player player;

extern void player_add_vector(vec3_t vec);

extern vec3_t player_direction(); // direction vector

extern void move_player(double delta_time);
