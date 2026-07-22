#define VECMATH_GENERICS
#define STB_IMAGE_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "util/vecmath.h"
#include "textures.h"
#include "util/stb/stb_image.h"
#define MAX_PLANES_PER_BRUSH 64
#define MAX_VERTS_PER_FACE 64
#define INTERSECT_EPS 1e-6f
#define INSIDE_EPS 1e-3f
#define DEDUP_EPS 1e-3f
#define UV_FLOAT_EPS 1e-4f

struct face_plane{
	vec3_t n;
	float d;
	bool src_uv_is_11;
	uint16_t texid;
};

struct vert{ float x, y, z; float u, v; float texid; };
struct triangle{ struct vert v[3]; uint16_t texid; };

static struct triangle* raw_tris = NULL;
static uint32_t n_raw_tris = 0;
static uint32_t cap_raw_tris = 0;
static int tex_w[TEXTURE_COUNT];
static int tex_h[TEXTURE_COUNT];

static void push_tri(struct vert a, struct vert b, struct vert c, uint16_t id){
	if(n_raw_tris == cap_raw_tris){
		cap_raw_tris = cap_raw_tris ? cap_raw_tris * 2 : 4096;
		raw_tris = realloc(raw_tris, cap_raw_tris * sizeof(struct triangle));
	}
	raw_tris[n_raw_tris] = (struct triangle){{a, b, c}, id};
	n_raw_tris++;
}

static int plane_intersect(struct face_plane a, struct face_plane b, struct face_plane c, vec3_t* out){
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

static int is_inside_all(vec3_t p, struct face_plane* planes, int n, int si, int sj, int sk){
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
	// For vertical faces, ensure v points in -Z so V increases downward on the
	// wall and the texture appears right-side up (V=0 at top, V=1 at bottom).
	// Without this, cross product results give opposite v.z signs for opposite
	// wall normals, making half the walls appear upside down.
	if(fabsf(n.z) < 0.9f && v_basis.z > 0.0f){
		v_basis = vec3_mulf(v_basis, -1.0f);
	}
	*u_basis_out = u_basis;
	*v_basis_out = v_basis;
}

// src_uv_is_11 faces: stretch the texture across each triangle with UVs clamped
// to [0, 1]. The GPU sampler (wrap=repeat) will simply show one copy of the texture.
static void write_triangle_11(vec3_t a, vec3_t b, vec3_t c, uint16_t id){
	float fid = (float)id;
	push_tri(
		(struct vert){a.x, a.y, a.z, 0.0f, 0.0f, fid},
		(struct vert){b.x, b.y, b.z, 1.0f, 0.0f, fid},
		(struct vert){c.x, c.y, c.z, 0.0f, 1.0f, fid},
		id
	);
}

// Projects each vertex onto the face UV basis. UV 1.0 = one texture repeat;
// tile size equals the texture's pixel dimensions so 1 texel = 1 world unit.
static void write_triangle_projected(vec3_t a, vec3_t b, vec3_t c,
                                     vec3_t u_basis, vec3_t v_basis, uint16_t id){
	float tile_u = (float)tex_w[id];
	float tile_v = (float)tex_h[id];
	vec3_t verts[3] = { a, b, c };
	float us[3], vs[3];
	float fid = (float)id;
	for(int i = 0; i < 3; i++){
		us[i] = vec3_dot(verts[i], u_basis) / tile_u;
		vs[i] = vec3_dot(verts[i], v_basis) / tile_v;
	}
	push_tri(
		(struct vert){a.x, a.y, a.z, us[0], vs[0], fid},
		(struct vert){b.x, b.y, b.z, us[1], vs[1], fid},
		(struct vert){c.x, c.y, c.z, us[2], vs[2], fid},
		id
	);
}

// Minimum ratio between the smaller of the two main-pair faces and the
// next-largest face for a brush to be treated as a plane-like slab. A true
// cube has ratio 1; a slab thin in one axis grows the ratio with min(a,b)/t.
#define PLANE_AREA_RATIO 2.0f
// Two opposite faces of a brush have inward normals that are antiparallel,
// so dot < this threshold means "opposite faces of a slab pair".
#define PLANE_ANTIPARALLEL_DOT -0.99f

static void emit_brush(struct face_plane* planes, int n){
	vec3_t face_verts[MAX_PLANES_PER_BRUSH][MAX_VERTS_PER_FACE];
	int face_nv[MAX_PLANES_PER_BRUSH] = {0};
	vec3_t face_centroid[MAX_PLANES_PER_BRUSH];
	float face_area[MAX_PLANES_PER_BRUSH] = {0};

	for(int i = 0; i < n; i++){
		vec3_t* verts = face_verts[i];
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
		if(nv < 3){
			face_nv[i] = 0;
			continue;
		}

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

		float area = 0.0f;
		for(int q = 1; q < nv - 1; q++){
			vec3_t e1 = vec3_sub(verts[q], verts[0]);
			vec3_t e2 = vec3_sub(verts[q + 1], verts[0]);
			area += 0.5f * vec3_length(vec3_cross(e1, e2));
		}

		face_nv[i] = nv;
		face_centroid[i] = centroid;
		face_area[i] = area;
	}

	// Find the largest pair of opposite (antiparallel) faces — these are the
	// "main" sides of a slab. For a true slab their combined area dominates the
	// strips; for a cube it doesn't, and we leave the brush alone.
	int main_a = -1, main_b = -1;
	float best_pair_area = 0.0f;
	for(int i = 0; i < n; i++){
		if(face_nv[i] < 3) continue;
		for(int j = i + 1; j < n; j++){
			if(face_nv[j] < 3) continue;
			if(vec3_dot(planes[i].n, planes[j].n) > PLANE_ANTIPARALLEL_DOT) continue;
			float pair_area = face_area[i] + face_area[j];
			if(pair_area > best_pair_area){
				best_pair_area = pair_area;
				main_a = i;
				main_b = j;
			}
		}
	}

	int keep_only = -1;
	if(main_a >= 0){
		float max_other = 0.0f;
		for(int i = 0; i < n; i++){
			if(i == main_a || i == main_b) continue;
			if(face_area[i] > max_other) max_other = face_area[i];
		}
		float min_main = face_area[main_a] < face_area[main_b] ? face_area[main_a] : face_area[main_b];
		if(min_main > PLANE_AREA_RATIO * max_other){
			// planes[i].n points INTO the brush, so outward = -planes[i].n.
			// We want outward to point toward the origin, i.e.
			//   dot(-planes[i].n, origin - centroid) > 0
			// which reduces to dot(planes[i].n, centroid) > 0. Pick the larger
			// of the two — handles slabs that don't straddle the origin and
			// gracefully degrades to "either side" for ones that do.
			float dot_a = vec3_dot(planes[main_a].n, face_centroid[main_a]);
			float dot_b = vec3_dot(planes[main_b].n, face_centroid[main_b]);
			keep_only = (dot_a > dot_b) ? main_a : main_b;
		}
	}

	for(int i = 0; i < n; i++){
		if(face_nv[i] < 3) continue;
		if(keep_only >= 0 && i != keep_only) continue;

		vec3_t* verts = face_verts[i];
		int nv = face_nv[i];

		if(planes[i].src_uv_is_11){
			for(int q = 1; q < nv - 1; q++){
				write_triangle_11(verts[0], verts[q], verts[q + 1], planes[i].texid);
			}
		}else{
			vec3_t u_basis, v_basis;
			compute_face_basis(planes[i].n, &u_basis, &v_basis);
			for(int q = 1; q < nv - 1; q++){
				write_triangle_projected(verts[0], verts[q], verts[q + 1],
				                         u_basis, v_basis, planes[i].texid);
			}
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
	if(!fmap){
		printf("failed to open input file\n");
		return 1;
	}

	int dummy;
	for(int i = 0; i < TEXTURE_COUNT; i++){
		if(!stbi_info(texture_paths[i], &tex_w[i], &tex_h[i], &dummy)){
			printf("warn: can't read dims for %s, defaulting to 64\n", texture_paths[i]);
			tex_w[i] = tex_h[i] = 64;
		}
	}

	struct face_plane brush[MAX_PLANES_PER_BRUSH];
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
				emit_brush(brush, n_brush);
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
		char texname[64] = {0};
		for(int j = 0; line[index] != ' ' && j < 63; j++){
			texname[j] = line[index];
			index++;
		}

		uint16_t face_texid = 0;
		for(int t = 0; t < TEXTURE_COUNT; t++){
			if(strcmp(texname, texture_names[t]) == 0){
				face_texid = (uint16_t)t;
				break;
			}
		}

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
			brush[n_brush].texid = face_texid;
			n_brush++;
		}
	}
	fclose(fmap);

	// Build deduplicated vertex buffer
	struct vert* verts = malloc(n_raw_tris * 3 * sizeof(struct vert));
	uint32_t n_verts = 0;
	uint32_t* indices = malloc(n_raw_tris * 3 * sizeof(uint32_t));

	for(uint32_t t = 0; t < n_raw_tris; t++){
		for(int vi = 0; vi < 3; vi++){
			struct vert rv = raw_tris[t].v[vi];
			uint32_t found = n_verts;
			for(uint32_t j = 0; j < n_verts; j++){
				if(fabsf(verts[j].x - rv.x) < DEDUP_EPS &&
				   fabsf(verts[j].y - rv.y) < DEDUP_EPS &&
				   fabsf(verts[j].z - rv.z) < DEDUP_EPS &&
				   fabsf(verts[j].u - rv.u) < UV_FLOAT_EPS &&
				   fabsf(verts[j].v - rv.v) < UV_FLOAT_EPS &&
				   verts[j].texid == rv.texid){
					found = j;
					break;
				}
			}
			if(found == n_verts) verts[n_verts++] = rv;
			indices[t * 3 + vi] = found;
		}
	}

	FILE* flvl = fopen(lvl_path, "wb");
	if(!flvl){
		printf("failed to open output file\n");
		return 1;
	}

	fwrite(&n_verts,    sizeof(uint32_t), 1,               flvl);
	fwrite(&n_raw_tris, sizeof(uint32_t), 1,               flvl);
	fwrite(indices, sizeof(uint32_t),  n_raw_tris * 3, flvl);
	fwrite(verts,   sizeof(struct vert), n_verts,       flvl);

	fclose(flvl);

	printf("wrote %u verts, %u tris\n", n_verts, n_raw_tris);

	free(raw_tris);
	free(verts);
	free(indices);
	return 0;
}
