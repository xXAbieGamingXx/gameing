#define SOKOL_GFX_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_GLUE_IMPL
#define VECMATH_GENERICS
#define SOKOL_GLCORE
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "input.h"
#include "util/vecmath.h"
#include "rectangle.glsl.h"
#include "util/fileutil.h"
#include "plane.h"
#include "images.h"
#include "player.h"

static struct{
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
	struct mouse_state* mouse;
} state;

struct Vertex {
	float x, y, z;
	short u, v;
};

#define SENSITIVITY .25

double delta_time = 0;

static void fetch_cb(const sfetch_response_t* response);

void cleanup();

void init(){
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	plane_init();

	sapp_lock_mouse(true);

	fetch_images();

	state.pass_action = (sg_pass_action){
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {.0f, .0f, .0f, 1.0f}
		},
	};

	state.bind.views[VIEW_tex] = sg_alloc_view();

	state.bind.samplers[SMP_smp] = sg_make_sampler(&(sg_sampler_desc){
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
		.label = "texture-test",
	});

	const struct Vertex vertices[] = { // x, y, z, u (horizontal), v (vertical)
		{1.0f , 1.0f , .1f, 32767, 0    },
		{-1.0f, 1.0f , .1f, 0    , 0    },
		{1.0f , -1.0f, .1f, 32767, 32767},
		{-1.0f, -1.0f, .1f, 0    , 32767},
	};

	state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.data = SG_RANGE(vertices),
		.label = "rectangle_vertices",
	});

	const uint16_t indices[] = {
		0, 1, 2,    1, 2, 3,
	};

	state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
		.usage.index_buffer = true,
		.data = SG_RANGE(indices),
	});

	sg_shader rect_shd = sg_make_shader(rectangle_shader_desc(sg_query_backend()));

	state.pip = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = rect_shd,
		.layout = {
			.attrs = {
				[ATTR_rectangle_pos].format = SG_VERTEXFORMAT_FLOAT3,
				[ATTR_rectangle_texcoord0].format = SG_VERTEXFORMAT_SHORT2N,
			},
		},
		.index_type = SG_INDEXTYPE_UINT16,
	});

	sg_init_view(state.bind.views[VIEW_tex], &(sg_view_desc){
		.texture = {.image = images[NERD_EMOJI_ID]},
	});
}

void frame(){
	if(key_down(SAPP_KEYCODE_ESCAPE)){
		cleanup();
	}

	sfetch_dowork();

	delta_time = sapp_frame_duration();

	move_player(delta_time);
	
	sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});

	// i think the longer side is one then scales from aspect ratio?

	draw_plane(&(struct plane_desc){
		.x = 5,
		.pip = state.pip,
		.bind = state.bind,
	});

	draw_plane(&(struct plane_desc){
		.rz = 90,
		.pip = state.pip,
		.bind = state.bind,
	});

	sg_end_pass();
	sg_commit();
}

void cleanup(){
	sg_shutdown();
	image_shutdown();
}


struct mouse_state temp_mouse;

void event(const sapp_event* ev){
	handle_input(ev);
	temp_mouse = get_mouse_state();
	state.mouse = &temp_mouse;

	player.rz -= state.mouse->dx*SENSITIVITY; // z is the one that does that
	player.ry += state.mouse->dy*SENSITIVITY;
}

sapp_desc sokol_main(int argc, char* argv[]){
	(void)argc; (void)argv; 
	return (sapp_desc) {
		.width = 1280,
		.height = 720,
		.init_cb = init,
		.frame_cb = frame,
		.cleanup_cb = cleanup,
		.event_cb = event,
		.logger.func = slog_func,
		.window_title = "gmaing",
	};
}
