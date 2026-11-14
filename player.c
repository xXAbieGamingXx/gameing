#define VECMATH_GENERICS
#include <float.h>
#include <math.h>
#include "util/vecmath.h"
#include "input.h"
#include "sokol_app.h"
#include "octree.h"
#include "parse_map.h"
#include "aabb.h"
#include "constants.h"
#include "collision.h"

struct Player {
	float x, y, z;
	float ry, rz;
};

#define WALKING_SPEED 70/TICKS_PER_SEC
#define PLAYER_HEIGHT 56 // eye to feet, quake bean dimensions
#define PLAYER_RADIUS 16
#define PLAYER_SKIN 0.1f // gap from walls
#define SLIDE_ITERATIONS 3
#define MAX_TOUCHING 32 // entities overlapping player per tick

struct Player player = {0};

void player_add_vector(vec3_t vec) {
	player.x += vec.x;
	player.y += vec.y;
	player.z += vec.z;
}

vec3_t player_direction() {
	return vec3(cos(vm_radians(player.rz)) * cos(vm_radians(player.ry)),
		sin(vm_radians(player.rz)) * cos(vm_radians(player.ry)),
		-sin(vm_radians(player.ry)));
}

struct collision_test check_capsule_collision(vec3_t a, vec3_t b, float radius, mat33_t triangle) {
	vec3_t edge1 = vec3_sub(triangle.y, triangle.x);
	vec3_t edge2 = vec3_sub(triangle.z, triangle.x);
	vec3_t normal = vec3_normalize(vec3_cross(edge1, edge2));
	vec3_t axis = vec3_sub(b, a);

	vec3_t reference = triangle.x;
	float denom = vec3_dot(normal, axis);
	if(fabsf(denom) > FLT_EPSILON) {
		float t = vec3_dot(normal, vec3_sub(triangle.x, a)) / denom;
		reference = closest_point_on_triangle(triangle, vec3_add(a, vec3_mulf(axis, t)));
	}

	vec3_t center = closest_point_on_segment(a, b, reference);
	vec3_t closest = closest_point_on_triangle(triangle, center);
	vec3_t diff = vec3_sub(center, closest);
	float dist_sq = vec3_lengthsq(diff);

	if(dist_sq >= radius * radius) {
		return (struct collision_test) { .collision = false };
	}

	float dist = sqrtf(dist_sq);
	vec3_t push_dir = dist > FLT_EPSILON ? vec3_divf(diff, dist) : normal;

	return (struct collision_test) {
		.collision = true,
		.vector = vec3_mulf(push_dir, radius - dist),
	};
}

struct collision_test check_capsule_cast(vec3_t a, vec3_t b, float radius, vec3_t movement, mat33_t triangle) {
	vec3_t edge1 = vec3_sub(triangle.y, triangle.x);
	vec3_t edge2 = vec3_sub(triangle.z, triangle.x);
	vec3_t normal = vec3_normalize(vec3_cross(edge1, edge2));

	if(vec3_dot(movement, normal) >= 0) {
		return (struct collision_test) { .collision = false };
	}

	float da = vec3_dot(normal, vec3_sub(a, triangle.x));
	float db = vec3_dot(normal, vec3_sub(b, triangle.x));
	vec3_t nearest = da < db ? a : b;
	if(fminf(da, db) < radius) { // already touching or behind
		return (struct collision_test) { .collision = false };
	}

	vec3_t lead = vec3_sub(nearest, vec3_mulf(normal, radius));
	struct collision_test results = check_triangle_collision(lead, movement, triangle);
	results.normal = normal;
	return results;
}

#ifndef TEST
static vec3_t capsule_feet() {
	return vec3(player.x, player.y, player.z - PLAYER_HEIGHT + PLAYER_RADIUS);
}

static vec3_t capsule_top() {
	return vec3(player.x, player.y, player.z - PLAYER_RADIUS);
}

static int octree_path(struct octree_node* nodes, uint32_t path[OCTREE_MAX_DEPTH + 1]) {
	uint32_t index = 0;
	int count = 0;
	while(true) {
		path[count++] = index;
		struct octree_node* node = &nodes[index];
		vec3_t center = vec3((node->min[0]+node->max[0])/2, (node->min[1]+node->max[1])/2, (node->min[2]+node->max[2])/2);

		uint8_t next_node = (player.x >= center.x) + (player.y >= center.y) * 2 + (player.z >= center.z) * 4;
		if(node->children[next_node] == 0 || count > OCTREE_MAX_DEPTH) { // effectively a leaf
			return count;
		}
		index = node->children[next_node];
	}
}

static uint32_t touch_entities(struct triangle_data* triangles, float reach, struct entity** out) {
	float min[3] = { player.x - PLAYER_RADIUS - reach, player.y - PLAYER_RADIUS - reach, player.z - PLAYER_HEIGHT - reach };
	float max[3] = { player.x + PLAYER_RADIUS + reach, player.y + PLAYER_RADIUS + reach, player.z + reach };
	struct entity* hits[MAX_TOUCHING];
	uint32_t hit_count = aabb_query(triangles->aabb_nodes, triangles->aabb_node_count, min, max, hits, MAX_TOUCHING);

	uint32_t solid_count = 0;
	for(uint32_t i = 0; i < hit_count; i++) {
		if(hits[i]->collide != NULL) {
			hits[i]->collide(hits[i]);
		} else {
			out[solid_count++] = hits[i];
		}
	}
	return solid_count;
}

static void cast_against(mat33_t triangle, vec3_t movement, struct collision_test* nearest) {
	struct collision_test results = check_capsule_cast(capsule_feet(), capsule_top(), PLAYER_RADIUS, movement, triangle);
	if(results.collision && results.t < nearest->t) {
		*nearest = results;
	}
}

static void push_out_of(mat33_t triangle) {
	struct collision_test results = check_capsule_collision(capsule_feet(), capsule_top(), PLAYER_RADIUS, triangle);
	if(results.collision) {
		player_add_vector(results.vector);
	}
}

static struct collision_test cast_player(struct octree_node* nodes, struct triangle_data* triangles, struct entity** solids, uint32_t solid_count, vec3_t movement) {
	uint32_t path[OCTREE_MAX_DEPTH + 1];
	int count = octree_path(nodes, path);
	struct collision_test nearest = { .collision = false, .t = 2.0f };
	for(int n = 0; n < count; n++) {
		struct octree_node* node = &nodes[path[n]];
		for(uint32_t i = node->first_tri; i < node->first_tri + node->tri_count; i++) {
			cast_against(get_triangle_mat33(triangles, i), movement, &nearest);
		}
	}
	for(uint32_t e = 0; e < solid_count; e++) {
		for(uint32_t i = 0; i < solids[e]->tri_count; i++) {
			cast_against(get_entity_triangle_mat33(triangles, solids[e], i), movement, &nearest);
		}
	}
	return nearest;
}

static void push_out_of_triangles(struct octree_node* nodes, struct triangle_data* triangles, struct entity** solids, uint32_t solid_count) {
	uint32_t path[OCTREE_MAX_DEPTH + 1];
	int count = octree_path(nodes, path);
	for(int n = 0; n < count; n++) {
		struct octree_node* node = &nodes[path[n]];
		for(uint32_t i = node->first_tri; i < node->first_tri + node->tri_count; i++) {
			push_out_of(get_triangle_mat33(triangles, i));
		}
	}
	for(uint32_t e = 0; e < solid_count; e++) {
		for(uint32_t i = 0; i < solids[e]->tri_count; i++) {
			push_out_of(get_entity_triangle_mat33(triangles, solids[e], i));
		}
	}
}

void player_frame(struct octree_node* nodes, struct triangle_data* triangles) {
	float direction = key_down(SAPP_KEYCODE_W) - key_down(SAPP_KEYCODE_S);

	vec3_t x_movement = vec3(cos(vm_radians(player.rz)), sin(vm_radians(player.rz)), 0);
	float multiplier = direction * WALKING_SPEED;

	x_movement.x *= multiplier;
	x_movement.y *= multiplier;

	direction = key_down(SAPP_KEYCODE_A) - key_down(SAPP_KEYCODE_D);

	vec3_t y_movement = vec3(cos(vm_radians(player.rz+90)), sin(vm_radians(player.rz+90)), 0);
	multiplier = direction * WALKING_SPEED;

	y_movement.x *= multiplier;
	y_movement.y *= multiplier;

	vec3_t movement = vec3_add(x_movement, y_movement);

	// sliding never travels further than the full movement
	struct entity* solids[MAX_TOUCHING];
	uint32_t solid_count = touch_entities(triangles, sqrtf(vec3_lengthsq(movement)) + PLAYER_SKIN, solids);

	for(int i = 0; i < SLIDE_ITERATIONS; i++) {
		float length = sqrtf(vec3_lengthsq(movement));
		if(length < FLT_EPSILON) {
			break;
		}

		struct collision_test hit = cast_player(nodes, triangles, solids, solid_count, movement);
		if(!hit.collision) {
			player_add_vector(movement);
			break;
		}

		player_add_vector(vec3_mulf(movement, fmaxf(hit.t - PLAYER_SKIN / length, 0.0f)));

		// slide the rest along the wall
		movement = vec3_mulf(movement, 1.0f - hit.t);
		movement = vec3_sub(movement, vec3_mulf(hit.normal, vec3_dot(movement, hit.normal)));
	}

	push_out_of_triangles(nodes, triangles, solids, solid_count);
}
#endif
