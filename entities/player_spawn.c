#include "player.h"
#include "entities.h"

void player_spawn_init(struct entity* entity) {
	entity->visible = true;
	player.x = entity->position->x;
	player.y = entity->position->y;
	player.z = entity->position->z;
	player.rz = entity->params[0];
}
