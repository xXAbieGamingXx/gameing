#define VECMATH_GENERICS
#include <float.h>
#include <math.h>
#include "collision.h"

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

vec3_t closest_point_on_segment(vec3_t a, vec3_t b, vec3_t p) {
	vec3_t ab = vec3_sub(b, a);
	float len_sq = vec3_dot(ab, ab);
	if(len_sq < FLT_EPSILON) {
		return a;
	}
	float t = vecmath_clamp(vec3_dot(vec3_sub(p, a), ab) / len_sq, 0.0f, 1.0f);
	return vec3_add(a, vec3_mulf(ab, t));
}

vec3_t closest_point_on_triangle(mat33_t triangle, vec3_t p) {
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
