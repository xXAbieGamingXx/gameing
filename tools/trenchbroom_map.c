#define VECMATH_GENERICS
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "util/vecmath.h"

#define UV_TILE_SCALE 1024.0f
#define MAX_PLANES_PER_BRUSH 64
#define MAX_VERTS_PER_FACE 64
#define INTERSECT_EPS 1e-6f
#define INSIDE_EPS 1e-3f
#define DEDUP_EPS 1e-3f

typedef struct {
	vec3_t n;
	float d;
	bool src_uv_is_11;
	uint16_t texid;
} face_plane_t;

static int plane_intersect(face_plane_t a, face_plane_t b, face_plane_t c, vec3_t* out){
	vec3_t bxc = vec3_cross(b.n, c.n);
	float denom = vec3_dot(a.n, bxc);
	if(fabsf(denom) < INTERSECT_EPS) return 0;
	vec3_t cxa = vec3_cross(c.n, a.n);
	vec3_t axb = vec3_cross(a.n, b.n);
	vec3_t sum = vec3_add(
		vec3_add(vec3_mulf(bxc, a.d), vec3_mulf(cxa, b.d)),
		vec3_mulf(axb, c.d));
	*out = vec3_mulf(sum, 1.0f / denom);
	return 1;
}

static int is_inside_all(vec3_t p, face_plane_t* planes, int n, int si, int sj, int sk){
	for(int m = 0; m < n; m++){
		if(m == si || m == sj || m == sk) continue;
		if(vec3_dot(planes[m].n, p) - planes[m].d < -INSIDE_EPS) return 0;
	}
	return 1;
}

static void compute_face_basis(vec3_t n, vec3_t* u_basis_out, vec3_t* v_basis_out){
	vec3_t ref = (fabsf(n.z) > 0.9f) ? vec3(1.0f, 0.0f, 0.0f) : vec3(0.0f, 0.0f, 1.0f);
	vec3_t u_basis = vec3_normalize(vec3_cross(ref, n));
	vec3_t v_basis = vec3_normalize(vec3_cross(n, u_basis));
	if(u_basis.x + u_basis.y + u_basis.z < 0.0f){
		u_basis = vec3_mulf(u_basis, -1.0f);
		v_basis = vec3_mulf(v_basis, -1.0f);
	}
	*u_basis_out = u_basis;
	*v_basis_out = v_basis;
}

static vec3_t lerp_v3(vec3_t p, vec3_t q, float t){
	return vec3(p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t, p.z + (q.z - p.z) * t);
}

#define POLY_MAX_VERTS 64

typedef struct {
	vec3_t v[POLY_MAX_VERTS];
	int n;
} poly_t;

// Sutherland-Hodgman split of poly against plane dot(p, axis) = value.
// Vertices with dot < value go to *neg; dot > value go to *pos. Edge crossings
// are added to both sides so neighboring pieces share boundary vertices.
static void split_poly_plane(const poly_t* in, vec3_t axis, float value, poly_t* neg, poly_t* pos){
	const float eps = 1e-4f;
	neg->n = 0;
	pos->n = 0;
	if(in->n < 3) return;
	for(int i = 0; i < in->n; i++){
		vec3_t a = in->v[i];
		vec3_t b = in->v[(i + 1) % in->n];
		float da = vec3_dot(a, axis) - value;
		float db = vec3_dot(b, axis) - value;
		if(da <= eps && neg->n < POLY_MAX_VERTS) neg->v[neg->n++] = a;
		if(da >= -eps && pos->n < POLY_MAX_VERTS) pos->v[pos->n++] = a;
		bool crosses = (da > eps && db < -eps) || (da < -eps && db > eps);
		if(crosses){
			float t = da / (da - db);
			vec3_t q = lerp_v3(a, b, t);
			if(neg->n < POLY_MAX_VERTS) neg->v[neg->n++] = q;
			if(pos->n < POLY_MAX_VERTS) pos->v[pos->n++] = q;
		}
	}
}

// Writes a triangle whose three vertices all lie in the same uint16 wrap tile
// (tile_u, tile_v). Subtracts the tile origin so each emitted UV lands in
// [0, 65535]; vertices on the upper boundary clamp to 65535 instead of wrapping
// to 0, which keeps GPU interpolation moving the correct way across the seam.
static void write_triangle_tile(FILE* flvl, vec3_t a, vec3_t b, vec3_t c,
                                vec3_t u_basis, vec3_t v_basis,
                                int tile_u, int tile_v, uint16_t id){
	vec3_t verts[3] = { a, b, c };
	uint16_t u[3], v[3];
	const float TILE_PERIOD = 65536.0f;
	float u_off = (float)tile_u * TILE_PERIOD;
	float v_off = (float)tile_v * TILE_PERIOD;
	for(int i = 0; i < 3; i++){
		float fu = vec3_dot(verts[i], u_basis) * UV_TILE_SCALE - u_off;
		float fv = vec3_dot(verts[i], v_basis) * UV_TILE_SCALE - v_off;
		if(fu < 0.0f) fu = 0.0f;
		if(fu > 65535.0f) fu = 65535.0f;
		if(fv < 0.0f) fv = 0.0f;
		if(fv > 65535.0f) fv = 65535.0f;
		u[i] = (uint16_t)fu;
		v[i] = (uint16_t)fv;
	}
	float pa[3] = { a.x, a.y, a.z };
	float pb[3] = { b.x, b.y, b.z };
	float pc[3] = { c.x, c.y, c.z };
	fwrite(pa, sizeof(float), 3, flvl);
	fwrite(&u[0], sizeof(uint16_t), 1, flvl);
	fwrite(&v[0], sizeof(uint16_t), 1, flvl);
	fwrite(pb, sizeof(float), 3, flvl);
	fwrite(&u[1], sizeof(uint16_t), 1, flvl);
	fwrite(&v[1], sizeof(uint16_t), 1, flvl);
	fwrite(pc, sizeof(float), 3, flvl);
	fwrite(&u[2], sizeof(uint16_t), 1, flvl);
	fwrite(&v[2], sizeof(uint16_t), 1, flvl);
	fwrite(&id, sizeof(uint16_t), 1, flvl);
}

// Recursively splits a polygon along uint16 UV wrap boundaries so each piece
// fits in one (tile_u, tile_v) cell, then triangulates the leaves. Without
// this, a triangle spanning a wrap (e.g. fu=64000 -> 70000) stores as 64000
// -> 4464 and the GPU interpolates backwards across the entire texture
// instead of crossing the repeat seam.
//
// u_lo/u_hi use an eps-shifted floor so vertices that land on a boundary are
// snapped to the side they came from; otherwise rounding noise on a vertex
// sitting exactly on the split plane would disagree with the splitter, which
// (correctly) reports no crossing, and we would recurse forever.
static void emit_poly_projected(FILE* flvl, const poly_t* poly,
                                vec3_t u_basis, vec3_t v_basis, uint16_t id){
	if(poly->n < 3) return;
	const float TILE_WORLD = 65536.0f / UV_TILE_SCALE;
	const float SPAN_EPS = 1e-3f;

	float u_min, u_max;
	u_min = u_max = vec3_dot(poly->v[0], u_basis);
	for(int i = 1; i < poly->n; i++){
		float u = vec3_dot(poly->v[i], u_basis);
		if(u < u_min) u_min = u;
		if(u > u_max) u_max = u;
	}
	int u_lo = (int)floorf((u_min + SPAN_EPS) / TILE_WORLD);
	int u_hi = (int)floorf((u_max - SPAN_EPS) / TILE_WORLD);
	if(u_hi > u_lo){
		float boundary = (float)(u_lo + 1) * TILE_WORLD;
		poly_t neg, pos;
		split_poly_plane(poly, u_basis, boundary, &neg, &pos);
		emit_poly_projected(flvl, &neg, u_basis, v_basis, id);
		emit_poly_projected(flvl, &pos, u_basis, v_basis, id);
		return;
	}

	float v_min, v_max;
	v_min = v_max = vec3_dot(poly->v[0], v_basis);
	for(int i = 1; i < poly->n; i++){
		float v = vec3_dot(poly->v[i], v_basis);
		if(v < v_min) v_min = v;
		if(v > v_max) v_max = v;
	}
	int v_lo = (int)floorf((v_min + SPAN_EPS) / TILE_WORLD);
	int v_hi = (int)floorf((v_max - SPAN_EPS) / TILE_WORLD);
	if(v_hi > v_lo){
		float boundary = (float)(v_lo + 1) * TILE_WORLD;
		poly_t neg, pos;
		split_poly_plane(poly, v_basis, boundary, &neg, &pos);
		emit_poly_projected(flvl, &neg, u_basis, v_basis, id);
		emit_poly_projected(flvl, &pos, u_basis, v_basis, id);
		return;
	}

	for(int i = 1; i < poly->n - 1; i++){
		write_triangle_tile(flvl, poly->v[0], poly->v[i], poly->v[i + 1],
		                    u_basis, v_basis, u_lo, v_lo, id);
	}
}

static void write_triangle_11(FILE* flvl, vec3_t a, vec3_t b, vec3_t c, uint16_t id){
	uint16_t u[3] = { 0, 65535, 0 };
	uint16_t v[3] = { 0, 0,     65535 };
	float pa[3] = { a.x, a.y, a.z };
	float pb[3] = { b.x, b.y, b.z };
	float pc[3] = { c.x, c.y, c.z };
	fwrite(pa, sizeof(float), 3, flvl);
	fwrite(&u[0], sizeof(uint16_t), 1, flvl);
	fwrite(&v[0], sizeof(uint16_t), 1, flvl);
	fwrite(pb, sizeof(float), 3, flvl);
	fwrite(&u[1], sizeof(uint16_t), 1, flvl);
	fwrite(&v[1], sizeof(uint16_t), 1, flvl);
	fwrite(pc, sizeof(float), 3, flvl);
	fwrite(&u[2], sizeof(uint16_t), 1, flvl);
	fwrite(&v[2], sizeof(uint16_t), 1, flvl);
	fwrite(&id, sizeof(uint16_t), 1, flvl);
}

static void emit_brush(FILE* flvl, face_plane_t* planes, int n){
	for(int i = 0; i < n; i++){
		vec3_t verts[MAX_VERTS_PER_FACE];
		int nv = 0;
		for(int j = 0; j < n; j++){
			if(j == i) continue;
			for(int k = j + 1; k < n; k++){
				if(k == i) continue;
				vec3_t p;
				if(!plane_intersect(planes[i], planes[j], planes[k], &p)) continue;
				if(!is_inside_all(p, planes, n, i, j, k)) continue;
				bool dup = false;
				for(int q = 0; q < nv; q++){
					if(vec3_lengthsq(vec3_sub(verts[q], p)) < DEDUP_EPS * DEDUP_EPS){
						dup = true;
						break;
					}
				}
				if(!dup && nv < MAX_VERTS_PER_FACE) verts[nv++] = p;
			}
		}
		if(nv < 3) continue;

		vec3_t centroid = vec3(0.0f, 0.0f, 0.0f);
		for(int q = 0; q < nv; q++) centroid = vec3_add(centroid, verts[q]);
		centroid = vec3_mulf(centroid, 1.0f / (float)nv);

		// planes[i].n is unit and points into the brush; sorting CCW around it
		// (with v = cross(n, u)) gives CW winding when viewed from outside,
		// matching the source .map convention.
		vec3_t nrm = planes[i].n;
		vec3_t u = vec3_normalize(vec3_sub(verts[0], centroid));
		vec3_t v = vec3_cross(nrm, u);

		float angles[MAX_VERTS_PER_FACE];
		for(int q = 0; q < nv; q++){
			vec3_t d = vec3_sub(verts[q], centroid);
			angles[q] = atan2f(vec3_dot(d, v), vec3_dot(d, u));
		}
		for(int q = 0; q < nv - 1; q++){
			int mn = q;
			for(int r = q + 1; r < nv; r++) if(angles[r] < angles[mn]) mn = r;
			if(mn != q){
				float ta = angles[q]; angles[q] = angles[mn]; angles[mn] = ta;
				vec3_t tv = verts[q]; verts[q] = verts[mn]; verts[mn] = tv;
			}
		}

		if(planes[i].src_uv_is_11){
			for(int q = 1; q < nv - 1; q++){
				write_triangle_11(flvl, verts[0], verts[q], verts[q + 1], planes[i].texid);
			}
		}else{
			vec3_t u_basis, v_basis;
			compute_face_basis(planes[i].n, &u_basis, &v_basis);
			poly_t face;
			face.n = (nv <= POLY_MAX_VERTS) ? nv : POLY_MAX_VERTS;
			for(int q = 0; q < face.n; q++) face.v[q] = verts[q];
			emit_poly_projected(flvl, &face, u_basis, v_basis, planes[i].texid);
		}
	}
}

int main(int argc, char** argv){
	if(argc != 3){
		printf(".map to .lvl converter takes two args");
		return 1;
	}
	char* map_path = argv[1];
	char* lvl_path = argv[2];
	FILE* fmap = fopen(map_path, "r");
	FILE* flvl = fopen(lvl_path, "wb");
	if(!fmap || !flvl){
		printf("failed to open files\n");
		return 1;
	}

	face_plane_t brush[MAX_PLANES_PER_BRUSH];
	int n_brush = 0;
	int depth = 0;
	char line[256];

	while(fgets(line, sizeof line, fmap) != NULL){
		if(line[0] == '/' && line[1] == '/') continue;
		int p = 0;
		while(line[p] == ' ' || line[p] == '\t') p++;
		if(line[p] == '{'){
			depth++;
			if(depth == 2) n_brush = 0;
			continue;
		}
		if(line[p] == '}'){
			if(depth == 2){
				emit_brush(flvl, brush, n_brush);
				n_brush = 0;
			}
			depth--;
			continue;
		}
		if(depth != 2 || line[p] != '(') continue;

		float coords[9];
		int index = p + 1;
		for(int i = 0; i < 9; i++){
			index++;
			if(line[index] == ')'){
				index += 4;
			}
			char number[20] = {0};
			for(int j = 0; line[index] != ' '; j++){
				number[j] = line[index];
				index++;
			}
			coords[i] = strtof(number, NULL);
		}
		index += 3;
		while(line[index] != ' ') index++;

		float src_uv[2] = {0.0f, 0.0f};
		for(int k = 0; k < 2; k++){
			index++;
			char number[20] = {0};
			for(int j = 0; line[index] != ' '; j++){
				number[j] = line[index];
				index++;
			}
			src_uv[k] = strtof(number, NULL);
		}

		vec3_t a = vec3(coords[0], coords[1], coords[2]);
		vec3_t b = vec3(coords[3], coords[4], coords[5]);
		vec3_t c = vec3(coords[6], coords[7], coords[8]);
		// Quake .map convention: face points are CW when viewed from outside,
		// so cross(b-a, c-a) gives a normal that points INTO the brush.
		vec3_t nrm = vec3_normalize(vec3_cross(vec3_sub(b, a), vec3_sub(c, a)));
		if(n_brush < MAX_PLANES_PER_BRUSH){
			brush[n_brush].n = nrm;
			brush[n_brush].d = vec3_dot(nrm, a);
			brush[n_brush].src_uv_is_11 = (src_uv[0] == 1.0f && src_uv[1] == 1.0f);
			brush[n_brush].texid = 0;
			n_brush++;
		}
	}

	fclose(fmap);
	fclose(flvl);
	return 0;
}
