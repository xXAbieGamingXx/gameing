#include "util/vecmath.h"

typedef struct sg_pipeline sg_pipeline;
typedef struct sg_bindings sg_bindings;

void render_init();

mat44_t get_view_proj(vec3_t eye);

void draw_vertices(sg_pipeline pip, sg_bindings* bind, int vertex_num, vec3_t eye);
