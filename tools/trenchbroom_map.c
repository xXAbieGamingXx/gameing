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
#include "octree.h"
#include "parse_map.h"
#include "util/stb/stb_image.h"

#define MAX_PLANES_PER_BRUSH 64
#define MAX_VERTS_PER_FACE (MAX_PLANES_PER_BRUSH + 4)
#define BIG 65536.0
#define INSIDE_EPS 1e-3f
#define DEDUP_EPS 1e-3f
#define UV_FLOAT_EPS 1e-4f
#define MAX_ENTITY_PARAMS 64


struct face_plane {
	vec3_t n;
	float d;
	vec3_t u_axis, v_axis;
	float u_off, v_off;
	uint16_t texid;
};

struct vert {
	float x, y, z;
	float u, v;
	float texid;
};
struct triangle {
	struct vert v[3]; 
	uint16_t texid;
};

struct tri_list {
	struct triangle* tris;
	uint32_t n, cap;
};

struct map_entity {
	char classname[64];
	uint16_t type;
	int32_t params[MAX_ENTITY_PARAMS];
	int n_params;
	struct tri_list tris;
};

static struct tri_list world;
static struct map_entity* entities = NULL;
static uint32_t n_entities = 0;
static uint32_t cap_entities = 0;
static int tex_w[TEXTURE_COUNT];
static int tex_h[TEXTURE_COUNT];

static void push_tri(struct tri_list* l, struct vert a, struct vert b, struct vert c, uint16_t id) {
	if(l->n == l->cap) {
		if(l->cap) {
			l->cap *= 2;
		} else {
			l->cap = 64;
		}
		l->tris = realloc(l->tris, l->cap * sizeof(struct triangle));
	}
	l->tris[l->n] = (struct triangle) {{a, b, c}, id};
	l->n++;
}

static struct map_entity* new_entity(void) {
	if(n_entities == cap_entities) {
		if(cap_entities) {
			cap_entities *= 2;
		} else {
			cap_entities = 16;
		}
		entities = realloc(entities, cap_entities * sizeof(struct map_entity));
	}
	entities[n_entities] = (struct map_entity) {0};
	return &entities[n_entities++];
}

// world and func_group (trenchbroom layers/groups) are static
static bool is_static(struct map_entity* e) {
	return strcmp(e->classname, "worldspawn") == 0 || strcmp(e->classname, "func_group") == 0;
}

// returns ENTITY_COUNT if classname is unknown
static uint16_t entity_type_of(char* classname) {
	for(uint16_t t = 0; t < ENTITY_COUNT; t++) {
		if(strcmp(classname, entity_descs[t].name) == 0) return t;
	}
	return ENTITY_COUNT;
}

static int clip_poly(double (*in)[3], int n, struct face_plane p, double (*out)[3]) {
	int m = 0;
	for(int i = 0; i < n; i++) {
		double* a = in[i];
		double* b = in[(i + 1) % n];
		double da = p.n.x * a[0] + p.n.y * a[1] + p.n.z * a[2] - p.d;
		double db = p.n.x * b[0] + p.n.y * b[1] + p.n.z * b[2] - p.d;

		int sa, sb;
		if(da > INSIDE_EPS) {
			sa = 1;
		} else if(da < -INSIDE_EPS) {
			sa = -1;
		} else {
			sa = 0;
		}

		if(db > INSIDE_EPS) {
			sb = 1;
		} else if(db < -INSIDE_EPS) {
			sb = -1;
		} else {
			sb = 0;
		}

		if(sa >= 0) {
			memcpy(out[m++], a, sizeof(double[3]));
		}
		if(sa * sb < 0) {
			double t = da / (da - db);
			for(int k = 0; k < 3; k++) out[m][k] = a[k] + (b[k] - a[k]) * t;
			m++;
		}
	}
	return m;
}

static int build_face(struct face_plane* planes, int n, int i, vec3_t* out) {
	vec3_t nrm = planes[i].n;
	vec3_t ref;
	if(fabsf(nrm.z) > 0.9f) {
		ref = vec3(1.0f, 0.0f, 0.0f);
	} else {
		ref = vec3(0.0f, 0.0f, 1.0f);
	}
	vec3_t u = vec3_normalize(vec3_cross(ref, nrm));
	vec3_t v = vec3_cross(nrm, u);
	vec3_t o = vec3_mulf(nrm, planes[i].d);
	double buf[2][MAX_VERTS_PER_FACE][3];
	for(int q = 0; q < 4; q++) {
		double su;
		if(q == 1 || q == 2) {
			su = BIG;
		} else {
			su = -BIG;
		}
		double sv;
		if(q >= 2) {
			sv = BIG;
		} else {
			sv = -BIG;
		}
		buf[0][q][0] = o.x + su * u.x + sv * v.x;
		buf[0][q][1] = o.y + su * u.y + sv * v.y;
		buf[0][q][2] = o.z + su * u.z + sv * v.z;
	}
	int nv = 4, cur = 0;
	for(int j = 0; j < n && nv >= 3; j++) {
		if(j == i) continue;
		nv = clip_poly(buf[cur], nv, planes[j], buf[cur ^ 1]);
		cur ^= 1;
	}
	for(int q = 0; q < nv; q++) {
		out[q] = vec3((float)buf[cur][q][0], (float)buf[cur][q][1], (float)buf[cur][q][2]);
	}
	return nv;
}

// quake standard axes: normal, s, t per face orientation
static const vec3_t base_axis[18] = {
	{0, 0, 1},  {1, 0, 0}, {0, -1, 0},
	{0, 0, -1}, {1, 0, 0}, {0, -1, 0},
	{1, 0, 0},  {0, 1, 0}, {0, 0, -1},
	{-1, 0, 0}, {0, 1, 0}, {0, 0, -1},
	{0, 1, 0},  {1, 0, 0}, {0, 0, -1},
	{0, -1, 0}, {1, 0, 0}, {0, 0, -1},
};

static int axis_index(vec3_t a) {
	if(a.x != 0.0f) return 0;
	if(a.y != 0.0f) return 1;
	return 2;
}

// texture axes for quake standard format, as qbsp does it
static void standard_axes(vec3_t n, float rot, float sx, float sy, vec3_t* u_out, vec3_t* v_out) {
	int best = 0;
	float best_dot = 0.0f;
	for(int i = 0; i < 6; i++) {
		float d = vec3_dot(n, base_axis[i * 3]);
		if(d > best_dot) {
			best_dot = d;
			best = i;
		}
	}
	vec3_t vecs[2] = {base_axis[best * 3 + 1], base_axis[best * 3 + 2]};

	float s, c;
	if(rot == 0.0f) { s = 0.0f; c = 1.0f; }
	else if(rot == 90.0f) { s = 1.0f; c = 0.0f; }
	else if(rot == 180.0f) { s = 0.0f; c = -1.0f; }
	else if(rot == 270.0f) { s = -1.0f; c = 0.0f; }
	else {
		s = sinf(rot * VECMATH_PI / 180.0f);
		c = cosf(rot * VECMATH_PI / 180.0f);
	}

	int sv = axis_index(vecs[0]);
	int tv = axis_index(vecs[1]);
	for(int i = 0; i < 2; i++) {
		float ns = c * vec3_get(vecs[i], sv) - s * vec3_get(vecs[i], tv);
		float nt = s * vec3_get(vecs[i], sv) + c * vec3_get(vecs[i], tv);
		vec3_set(&vecs[i], sv, ns);
		vec3_set(&vecs[i], tv, nt);
	}

	if(sx == 0.0f) sx = 1.0f;
	if(sy == 0.0f) sy = 1.0f;
	*u_out = vec3_mulf(vecs[0], 1.0f / sx);
	*v_out = vec3_mulf(vecs[1], 1.0f / sy);
}

static struct vert make_vert(vec3_t p, float u, float v, uint16_t id) {
	return (struct vert) {p.x, p.y, p.z, u, v, (float)id};
}

static void write_face(struct tri_list* l, vec3_t* verts, int nv, struct face_plane p) {
	float tw = (float)tex_w[p.texid];
	float th = (float)tex_h[p.texid];
	vec3_t tri[3] = { verts[0] };
	for(int q = 1; q < nv - 1; q++) {
		tri[1] = verts[q];
		tri[2] = verts[q + 1];
		struct vert out[3];
		for(int k = 0; k < 3; k++) {
			float u = (vec3_dot(tri[k], p.u_axis) + p.u_off) / tw;
			float v = (vec3_dot(tri[k], p.v_axis) + p.v_off) / th;
			out[k] = make_vert(tri[k], u, v, p.texid);
		}
		push_tri(l, out[0], out[1], out[2], p.texid);
	}
}

#define PLANE_AREA_RATIO 2.0f
#define PLANE_ANTIPARALLEL_DOT -0.99f

// cull skips non-worldspawn brushes, which can move and expose hidden faces
static void emit_brush(struct tri_list* l, struct face_plane* planes, int n, bool cull) {
	vec3_t face_verts[MAX_PLANES_PER_BRUSH][MAX_VERTS_PER_FACE];
	int face_nv[MAX_PLANES_PER_BRUSH];
	float face_area[MAX_PLANES_PER_BRUSH] = {0};

	for(int i = 0; i < n; i++) {
		vec3_t* verts = face_verts[i];
		face_nv[i] = build_face(planes, n, i, verts);
		for(int q = 1; q < face_nv[i] - 1; q++) {
			vec3_t e1 = vec3_sub(verts[q], verts[0]);
			vec3_t e2 = vec3_sub(verts[q + 1], verts[0]);
			face_area[i] += 0.5f * vec3_length(vec3_cross(e1, e2));
		}
	}

	int main_a = -1, main_b = -1;
	float best_pair_area = 0.0f;
	for(int i = 0; i < n; i++) {
		if(face_nv[i] < 3) continue;
		for(int j = i + 1; j < n; j++) {
			if(face_nv[j] < 3) continue;
			if(vec3_dot(planes[i].n, planes[j].n) > PLANE_ANTIPARALLEL_DOT) continue;
			float pair_area = face_area[i] + face_area[j];
			if(pair_area > best_pair_area) {
				best_pair_area = pair_area;
				main_a = i;
				main_b = j;
			}
		}
	}

	int keep_only = -1;
	if(cull && main_a >= 0) {
		float max_other = 0.0f;
		for(int i = 0; i < n; i++) {
			if(i == main_a || i == main_b) continue;
			if(face_area[i] > max_other) max_other = face_area[i];
		}
		float min_main;
		if(face_area[main_a] < face_area[main_b]) {
			min_main = face_area[main_a];
		} else {
			min_main = face_area[main_b];
		}
		if(min_main > PLANE_AREA_RATIO * max_other) {
			if(planes[main_a].d > planes[main_b].d) {
				keep_only = main_a;
			} else {
				keep_only = main_b;
			}
		}
	}

	for(int i = 0; i < n; i++) {
		if(face_nv[i] < 3) continue;
		if(keep_only >= 0 && i != keep_only) continue;
		write_face(l, face_verts[i], face_nv[i], planes[i]);
	}
}

static float (*tri_lo)[3];
static float (*tri_hi)[3];
static struct octree_node* nodes = NULL;
static uint32_t n_nodes = 0;
static uint32_t cap_nodes = 0;
static uint32_t* sorted_indices;
static uint32_t n_sorted = 0;

static uint32_t new_node(float* lo, float* hi) {
	if(n_nodes == cap_nodes) {
		if(cap_nodes) {
			cap_nodes *= 2;
		} else {
			cap_nodes = 64;
		}
		nodes = realloc(nodes, cap_nodes * sizeof(struct octree_node));
	}
	nodes[n_nodes] = (struct octree_node) {0};
	memcpy(nodes[n_nodes].min, lo, sizeof(float[3]));
	memcpy(nodes[n_nodes].max, hi, sizeof(float[3]));
	return n_nodes++;
}

static int tri_octant(uint32_t t, float* mid) {
	int oct = 0;
	for(int k = 0; k < 3; k++) {
		if(tri_lo[t][k] >= mid[k]) oct |= 1 << k;
		else if(tri_hi[t][k] > mid[k]) return -1;
	}
	return oct;
}

static void build_node(uint32_t node, uint32_t* tris, uint32_t n, int depth, uint32_t* indices) {
	float lo[3], hi[3], mid[3];
	memcpy(lo, nodes[node].min, sizeof lo);
	memcpy(hi, nodes[node].max, sizeof hi);
	for(int k = 0; k < 3; k++) mid[k] = (lo[k] + hi[k]) * 0.5f;

	bool split = n > OCTREE_LEAF_TRIS && depth < OCTREE_MAX_DEPTH;
	int* oct = malloc(n * sizeof(int));
	uint32_t count[8] = {0};
	nodes[node].first_tri = n_sorted;
	for(uint32_t i = 0; i < n; i++) {
		if(split) {
			oct[i] = tri_octant(tris[i], mid);
		} else {
			oct[i] = -1;
		}
		if(oct[i] >= 0) {
			count[oct[i]]++;
			continue;
		}
		memcpy(&sorted_indices[n_sorted * 3], &indices[tris[i] * 3], sizeof(uint32_t[3]));
		n_sorted++;
	}
	nodes[node].tri_count = n_sorted - nodes[node].first_tri;

	for(int c = 0; c < 8; c++) {
		if(!count[c]) continue;
		uint32_t* sub = malloc(count[c] * sizeof(uint32_t));
		uint32_t m = 0;
		for(uint32_t i = 0; i < n; i++) {
			if(oct[i] == c) sub[m++] = tris[i];
		}
		float clo[3], chi[3];
		for(int k = 0; k < 3; k++) {
			if(c >> k & 1) {
				clo[k] = mid[k];
				chi[k] = hi[k];
			} else {
				clo[k] = lo[k];
				chi[k] = mid[k];
			}
		}
		uint32_t child = new_node(clo, chi);
		nodes[node].children[c] = child;
		build_node(child, sub, m, depth + 1, indices);
		free(sub);
	}
	free(oct);
}

static void build_octree(struct vert* verts, uint32_t* indices, uint32_t n_tris) {
	tri_lo = malloc(n_tris * sizeof(float[3]));
	tri_hi = malloc(n_tris * sizeof(float[3]));
	sorted_indices = malloc(n_tris * 3 * sizeof(uint32_t));
	float lo[3] = {0}, hi[3] = {0};
	for(uint32_t t = 0; t < n_tris; t++) {
		for(int v = 0; v < 3; v++) {
			struct vert* p = &verts[indices[t * 3 + v]];
			float pos[3] = {p->x, p->y, p->z};
			for(int k = 0; k < 3; k++) {
				if(v == 0 || pos[k] < tri_lo[t][k]) tri_lo[t][k] = pos[k];
				if(v == 0 || pos[k] > tri_hi[t][k]) tri_hi[t][k] = pos[k];
				if((t == 0 && v == 0) || pos[k] < lo[k]) lo[k] = pos[k];
				if((t == 0 && v == 0) || pos[k] > hi[k]) hi[k] = pos[k];
			}
		}
	}
	float size = 0.0f;
	for(int k = 0; k < 3; k++) {
		if(hi[k] - lo[k] > size) size = hi[k] - lo[k];
	}
	for(int k = 0; k < 3; k++) hi[k] = lo[k] + size;

	uint32_t* all = malloc(n_tris * sizeof(uint32_t));
	for(uint32_t t = 0; t < n_tris; t++) all[t] = t;
	build_node(new_node(lo, hi), all, n_tris, 0, indices);
	free(all);
	free(tri_lo);
	free(tri_hi);
}

// merges matching verts; verts and indices sized for l->n * 3
static uint32_t dedup(struct tri_list* l, struct vert* verts, uint32_t* indices) {
	uint32_t n_verts = 0;
	for(uint32_t t = 0; t < l->n; t++) {
		for(int vi = 0; vi < 3; vi++) {
			struct vert rv = l->tris[t].v[vi];
			uint32_t found = n_verts;
			for(uint32_t j = 0; j < n_verts; j++) {
				if(fabsf(verts[j].x - rv.x) < DEDUP_EPS &&
				   fabsf(verts[j].y - rv.y) < DEDUP_EPS &&
				   fabsf(verts[j].z - rv.z) < DEDUP_EPS &&
				   fabsf(verts[j].u - rv.u) < UV_FLOAT_EPS &&
				   fabsf(verts[j].v - rv.v) < UV_FLOAT_EPS &&
				   verts[j].texid == rv.texid) {
					found = j;
					break;
				}
			}
			if(found == n_verts) verts[n_verts++] = rv;
			indices[t * 3 + vi] = found;
		}
	}
	return n_verts;
}

int main(int argc, char** argv) {
	if(argc != 3) {
		printf(".map to .lvl converter takes two args");
		return 1;
	}
	char* map_path = argv[1];
	char* lvl_path = argv[2];
	FILE* fmap = fopen(map_path, "r");
	if(!fmap) {
		printf("failed to open input file\n");
		return 1;
	}

	int dummy;
	for(int i = 0; i < TEXTURE_COUNT; i++) {
		if(!stbi_info(texture_paths[i], &tex_w[i], &tex_h[i], &dummy)) {
			printf("warn: can't read dims for %s, defaulting to 64\n", texture_paths[i]);
			tex_w[i] = tex_h[i] = 64;
		}
	}

	struct face_plane brush[MAX_PLANES_PER_BRUSH];
	int n_brush = 0;
	int depth = 0;
	char line[256];
	struct map_entity* ent = NULL;

	while(fgets(line, sizeof line, fmap) != NULL) {
		if(line[0] == '/' && line[1] == '/') continue;
		int p = 0;
		while(line[p] == ' ' || line[p] == '\t') p++;
		if(line[p] == '{') {
			depth++;
			if(depth == 1) ent = new_entity();
			if(depth == 2) n_brush = 0;
			continue;
		}
		if(line[p] == '}') {
			if(depth == 2) {
				if(is_static(ent)) {
					emit_brush(&world, brush, n_brush, true);
				} else {
					emit_brush(&ent->tris, brush, n_brush, false);
				}
				n_brush = 0;
			}
			if(depth == 1) {
				if(is_static(ent)) {
					n_entities--;
				} else {
					ent->type = entity_type_of(ent->classname);
					if(ent->type == ENTITY_COUNT) {
						printf("unknown entity classname \"%s\"\n", ent->classname);
						return 1;
					}
					if(ent->n_params != entity_descs[ent->type].params) {
						printf("warn: %s has %d params, desc says %d\n",
						       ent->classname, ent->n_params, entity_descs[ent->type].params);
					}
				}
			}
			depth--;
			continue;
		}
		if(depth == 1) {
			char key[64], value[128];
			if(sscanf(line + p, "\"%63[^\"]\" \"%127[^\"]\"", key, value) != 2) continue;
			if(strcmp(key, "classname") == 0) {
				strcpy(ent->classname, value);
				continue;
			}
			if(key[0] == '_') continue; // trenchbroom internal
			// values may hold several ints, ie origin "x y z"
			char* s = value;
			char* end;
			for(long v = strtol(s, &end, 10); end != s; v = strtol(s, &end, 10)) {
				if(ent->n_params < MAX_ENTITY_PARAMS) ent->params[ent->n_params++] = (int32_t)v;
				s = end;
			}
			continue;
		}
		if(depth != 2 || line[p] != '(') continue;

		float coords[9];
		char texname[64];
		int consumed = 0;
		if(sscanf(line + p, "( %f %f %f ) ( %f %f %f ) ( %f %f %f ) %63s %n",
		          &coords[0], &coords[1], &coords[2], &coords[3], &coords[4], &coords[5],
		          &coords[6], &coords[7], &coords[8], texname, &consumed) != 10) {
			continue;
		}
		char* tex_args = line + p + consumed;

		// valve 220: [ ux uy uz uoff ] [ vx vy vz voff ] rot sx sy
		// standard:  uoff voff rot sx sy
		float ua[4], va[4], rot, sx, sy;
		bool valve = sscanf(tex_args, "[ %f %f %f %f ] [ %f %f %f %f ] %f %f %f",
		                    &ua[0], &ua[1], &ua[2], &ua[3], &va[0], &va[1], &va[2], &va[3],
		                    &rot, &sx, &sy) == 11;
		if(!valve && sscanf(tex_args, "%f %f %f %f %f", &ua[3], &va[3], &rot, &sx, &sy) != 5) {
			continue;
		}

		uint16_t face_texid = 0;
		for(int t = 0; t < TEXTURE_COUNT; t++) {
			if(strcmp(texname, texture_names[t]) == 0) {
				face_texid = (uint16_t)t;
				break;
			}
		}

		vec3_t a = vec3(coords[0], coords[1], coords[2]);
		vec3_t b = vec3(coords[3], coords[4], coords[5]);
		vec3_t c = vec3(coords[6], coords[7], coords[8]);

		vec3_t nrm = vec3_normalize(vec3_cross(vec3_sub(b, a), vec3_sub(c, a)));
		if(n_brush < MAX_PLANES_PER_BRUSH) {
			brush[n_brush].n = nrm;
			brush[n_brush].d = vec3_dot(nrm, a);
			if(valve) {
				if(sx == 0.0f) sx = 1.0f;
				if(sy == 0.0f) sy = 1.0f;
				brush[n_brush].u_axis = vec3_mulf(vec3(ua[0], ua[1], ua[2]), 1.0f / sx);
				brush[n_brush].v_axis = vec3_mulf(vec3(va[0], va[1], va[2]), 1.0f / sy);
			} else {
				standard_axes(nrm, rot, sx, sy, &brush[n_brush].u_axis, &brush[n_brush].v_axis);
			}
			brush[n_brush].u_off = ua[3];
			brush[n_brush].v_off = va[3];
			brush[n_brush].texid = face_texid;
			n_brush++;
		}
	}
	fclose(fmap);

	uint32_t n_tris = world.n;
	struct vert* verts = malloc(n_tris * 3 * sizeof(struct vert));
	uint32_t* indices = malloc(n_tris * 3 * sizeof(uint32_t));
	uint32_t n_verts = dedup(&world, verts, indices);

	build_octree(verts, indices, n_tris);

	FILE* flvl = fopen(lvl_path, "wb");
	if(!flvl) {
		printf("failed to open output file\n");
		return 1;
	}

	fwrite(&n_verts,    sizeof(uint32_t), 1,               flvl);
	fwrite(&n_tris,     sizeof(uint32_t), 1,               flvl);
	fwrite(&n_nodes,    sizeof(uint32_t), 1,               flvl);
	fwrite(sorted_indices, sizeof(uint32_t), n_tris * 3, flvl);
	fwrite(verts,   sizeof(struct vert), n_verts,       flvl);
	fwrite(nodes,   sizeof(struct octree_node), n_nodes, flvl);

	fwrite(&n_entities, sizeof(uint32_t), 1, flvl);
	for(uint32_t i = 0; i < n_entities; i++) {
		struct map_entity* e = &entities[i];
		struct vert* ev = malloc(e->tris.n * 3 * sizeof(struct vert));
		uint32_t* ei = malloc(e->tris.n * 3 * sizeof(uint32_t));
		uint32_t env = dedup(&e->tris, ev, ei);
		fwrite(&e->type,   sizeof(uint16_t), 1, flvl);
		fwrite(e->params,  sizeof(int32_t), entity_descs[e->type].params, flvl); // unset params are 0
		fwrite(&env,       sizeof(uint32_t), 1, flvl);
		fwrite(&e->tris.n, sizeof(uint32_t), 1, flvl);
		fwrite(ei, sizeof(uint32_t), e->tris.n * 3, flvl);
		fwrite(ev, sizeof(struct vert), env, flvl);
		free(ev);
		free(ei);
		free(e->tris.tris);
	}

	fclose(flvl);

	printf("wrote %u verts, %u tris, %u nodes, %u entities\n", n_verts, n_tris, n_nodes, n_entities);

	free(world.tris);
	free(entities);
	free(verts);
	free(indices);
	free(sorted_indices);
	free(nodes);
	return 0;
}
