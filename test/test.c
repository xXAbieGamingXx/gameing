#define VECMATH_GENERICS
#define MUNIT_ENABLE_ASSERT_ALIASES
#include <stdio.h>
#include <math.h>
#include "util/vecmath.h"
#include "test/munit.h"
#include "player.h"

static MunitResult test_check_triangle_collision(const MunitParameter params[], void* data){
	(void) params;
	(void) data;
	/* Triangle in the XY plane */
	mat33_t tri = mat33(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	/* Ray pointing down from above the triangle center */
	vec3_t ray_origin = vec3(0.25f, 0.25f, 1.0f);
	vec3_t ray = vec3(0.0f, 0.0f, -1.0f);

	struct collision_test res = check_triangle_collision(ray_origin, ray, tri);
	assert_true(res.collision);
	/* Intersection point should be approximately (0.25, 0.25, 0.0) */
	assert_double_equal((double)res.vector.x, .25d, 3);
	assert_double_equal((double)res.vector.y, .25d, 3);
	assert_double_equal((double)res.vector.z, 0.0d, 3);

	/* Ray away from triangle should not collide */
	vec3_t ray_origin2 = vec3(2.0f, 2.0f, 1.0f);
	struct collision_test res2 = check_triangle_collision(ray_origin2, ray, tri);
	assert_false(res2.collision);

	/* Ray hitting a vertex */
	vec3_t v_origin = vec3(0.0f, 0.0f, 1.0f);
	struct collision_test v_res = check_triangle_collision(v_origin, ray, tri);
	assert_true(v_res.collision);
	assert_double_equal((double)v_res.vector.x, 0.0d, 3);
	assert_double_equal((double)v_res.vector.y, 0.0d, 3);

	/* Ray hitting edge midpoint (between tri.x and tri.y) */
	vec3_t e_origin = vec3(0.5f, 0.0f, 1.0f);
	struct collision_test e_res = check_triangle_collision(e_origin, ray, tri);
	assert_true(e_res.collision);
	assert_double_equal((double)e_res.vector.x, 0.5d, 3);
	assert_double_equal((double)e_res.vector.y, 0.0d, 3);

	/* Ray parallel to triangle plane should not collide */
	vec3_t p_origin = vec3(0.25f, 0.25f, 1.0f);
	vec3_t p_ray = vec3(1.0f, 0.0f, 0.0f);
	struct collision_test p_res = check_triangle_collision(p_origin, p_ray, tri);
	assert_false(p_res.collision);

	/* Ray from below pointing up should not collide due to backface culling */
	vec3_t below_origin = vec3(0.25f, 0.25f, -1.0f);
	vec3_t up_ray = vec3(0.0f, 0.0f, 1.0f);
	struct collision_test below_res = check_triangle_collision(below_origin, up_ray, tri);
	assert_false(below_res.collision);

	/* Flipped triangle (vertex order reversed) — ray from below should collide */
	mat33_t tri_flipped = mat33(vec3(0.0f,0.0f,0.0f), vec3(0.0f,1.0f,0.0f), vec3(1.0f,0.0f,0.0f));
	struct collision_test flip_res = check_triangle_collision(below_origin, up_ray, tri_flipped);
	assert_true(flip_res.collision);
	assert_double_equal((double)flip_res.vector.x, .25d, 3);
	assert_double_equal((double)flip_res.vector.y, .25d, 3);

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{ (char*) "player/triangle_collision", test_check_triangle_collision, NULL, NULL, 0, NULL},
	{ NULL, NULL, NULL, NULL, 0, NULL}
};

static const MunitSuite suite = 
	{(char*) "", tests, NULL, 1, 0};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc+1)]){
	return munit_suite_main(&suite, (void*) "", argc, argv);
}
