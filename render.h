typedef struct sg_pipeline sg_pipeline;
typedef struct sg_bindings sg_bindings;

void render_init();

void draw_vertices(sg_pipeline pip, sg_bindings bind, int vertex_num);
