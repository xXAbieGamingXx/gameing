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

struct collision_test {
	bool collision;
	vec3_t vector;
	float t; // fraction of ray travelled
	vec3_t normal;
};

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

struct collision_test check_triangle_collision(vec3_t ray_origin, vec3_t ray, mat33_t triangle) {
	const float epsilon = FLT_EPSILON;
	vec3_t edge1 = vec3_sub(triangle.y, triangle.x);
	vec3_t edge2 = vec3_sub(triangle.z, triangle.x);

	vec3_t pvec = vec3_cross(ray, edge2);
	float det = vec3_dot(edge1, pvec);

	if(det < epsilon) {
		return (struct collision_test) { .collision = false };
	}

	vec3_t tvec = vec3_sub(ray_origin, triangle.x);

	float u = vec3_dot(tvec, pvec);

	if(u < 0.0 || u > det) {
		return (struct collision_test) { .collision = false };
	}

	vec3_t qvec = vec3_cross(tvec, edge1);
	
	float v = vec3_dot(ray, qvec);

	if(v < 0.0 || u + v > det) {
		return (struct collision_test) { .collision = false };
	}

	float t = vec3_dot(edge2, qvec) / det;

	if(t < 0.0 || t > 1.0) {
		return (struct collision_test) { .collision = false };
	}

	u /= det;
	v /= det;

	vec3_t vector = vec3_add(vec3_add(vec3_mulf(triangle.x, 1.0f - u - v), vec3_mulf(triangle.y, u)), vec3_mulf(triangle.z, v));

	return (struct collision_test) {
		.collision = true,
		.vector = vector,
		.t = t,
	};
}

static vec3_t closest_point_on_segment(vec3_t a, vec3_t b, vec3_t p) {
	vec3_t ab = vec3_sub(b, a);
	float len_sq = vec3_dot(ab, ab);
	if(len_sq < FLT_EPSILON) {
		return a;
	}
	float t = vecmath_clamp(vec3_dot(vec3_sub(p, a), ab) / len_sq, 0.0f, 1.0f);
	return vec3_add(a, vec3_mulf(ab, t));
}

static vec3_t closest_point_on_triangle(mat33_t triangle, vec3_t p) {
	vec3_t edge1 = vec3_sub(triangle.y, triangle.x);
	vec3_t edge2 = vec3_sub(triangle.z, triangle.x);
	vec3_t normal = vec3_normalize(vec3_cross(edge1, edge2));

	// project onto plane
	vec3_t projected = vec3_sub(p, vec3_mulf(normal, vec3_dot(vec3_sub(p, triangle.x), normal)));

	vec3_t c0 = vec3_cross(vec3_sub(projected, triangle.x), vec3_sub(triangle.y, triangle.x));
	vec3_t c1 = vec3_cross(vec3_sub(projected, triangle.y), vec3_sub(triangle.z, triangle.y));
	vec3_t c2 = vec3_cross(vec3_sub(projected, triangle.z), vec3_sub(triangle.x, triangle.z));
	if(vec3_dot(c0, normal) <= 0 && vec3_dot(c1, normal) <= 0 && vec3_dot(c2, normal) <= 0) {
		return projected;
	}

	vec3_t p0 = closest_point_on_segment(triangle.x, triangle.y, p);
	vec3_t p1 = closest_point_on_segment(triangle.y, triangle.z, p);
	vec3_t p2 = closest_point_on_segment(triangle.z, triangle.x, p);
	float d0 = vec3_lengthsq(vec3_sub(p, p0));
	float d1 = vec3_lengthsq(vec3_sub(p, p1));
	float d2 = vec3_lengthsq(vec3_sub(p, p2));

	if(d0 <= d1 && d0 <= d2) {
		return p0;
	}
	if(d1 <= d2) {
		return p1;
	}
	return p2;
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
