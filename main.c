#define SOKOL_IMPL
#define SOKOL_GLCORE
#define SOKOL_WWIN32_FORCE_WINMAIN
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "triangle.glsl.h"

static struct{
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
} state;


void init(){
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	float vertices[] = {
		// posititions (xyz)	// colors (rgba)
		.0f, .5f, .5f,		//1.0f, .0f, .0f, 1.0f,
		.5f, -.5f, .5f,		//.0f, 1.0f, .0f, 1.0f,
		-.5f, -.5f, .5f,	//.0f, .0f, 1.0f, 1.0f,
	};

	state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.data = SG_RANGE(vertices),
		.label = "triangle_vertices",
	});

	sg_shader tri_shd = sg_make_shader(triangle_shader_desc(sg_query_backend()));

	state.pip = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = tri_shd,
		.layout = {
			.attrs = {
				[ATTR_triangle_pos].format = SG_VERTEXFORMAT_FLOAT3,
			},
		},
		.label = "triangle_pipeline",
	});

	state.pass_action = (sg_pass_action){
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {.0f, .0f, .0f, 1.0f}
		},
	};
}

void frame(){
	sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});
	sg_apply_pipeline(state.pip);
	sg_apply_bindings(&state.bind);
	sg_draw(0, 3, 1); // start at 0, do 3 elements
	sg_end_pass();
	sg_commit();
}

void cleanup(){
	sg_shutdown();
}

void event(const sapp_event* ev){}

sapp_desc sokol_main(int argc, char* argv[]){
	(void)argc; (void)argv; 
	return (sapp_desc) {
		.width = 640,
		.height = 480,
		.init_cb = init,
		.frame_cb = frame,
		.cleanup_cb = cleanup,
		.logger.func = slog_func,
		.window_title = "gmaing",
	};
}
