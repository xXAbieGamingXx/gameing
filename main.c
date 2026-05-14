#define SOKOL_GFX_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_TIME_IMPL
#define VECMATH_GENERICS
#define SOKOL_GLCORE
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#include "input.h"
#include "util/vecmath.h"
#include "rectangle.glsl.h"
#include "images.h"
#include "player.h"
#include "parse_map.h"
#include "render.h"

static struct{
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
	struct mouse_state mouse;
	bool loaded_images;
} state;

#define SENSITIVITY .25f

double delta_time = 0;
uint32_t vertex_num;

void cleanup();

void init(){
	state.loaded_images = false;
	
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	stm_setup();

	render_init();

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
	
	char* file = "maps/unnamed-compile.lvl";
	struct triangle_data* triangles = get_triangles(file);
	state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.data = (sg_range){ .ptr = triangles->vertices, .size = triangles->vertex_count * sizeof(struct vertex) },
		.label = "rectangle_vertices",
	});

	state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
		.usage.index_buffer = true,
		.data = (sg_range){ .ptr = triangles->indices, .size = triangles->tri_count * 3 * sizeof(uint32_t) },
		.label = "rectangle_indices",
	});

	vertex_num = triangles->tri_count * 3;

	free(triangles->vertices);
	free(triangles->indices);
	free(triangles->texids);
	free(triangles);

	sg_shader rect_shd = sg_make_shader(rectangle_shader_desc(sg_query_backend()));

	state.pip = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = rect_shd,
		.layout = {
			.attrs = {
				[ATTR_rectangle_pos].format = SG_VERTEXFORMAT_FLOAT3,
				[ATTR_rectangle_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
			},
		},
		.index_type = SG_INDEXTYPE_UINT32,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
	});

}

void frame(){
	reset_mouse(&state.mouse);
	if(key_down(SAPP_KEYCODE_ESCAPE)){
		cleanup();
	}

	images_dowork();

	if(!state.loaded_images && done_fetching){
		sg_init_view(state.bind.views[VIEW_tex], &(sg_view_desc){
			.texture = {.image = images[NERD_EMOJI_ID]},
		});
		state.loaded_images = true;
	}

	delta_time = sapp_frame_duration();

	move_player(delta_time);
	
	sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});

	draw_vertices(state.pip, state.bind, vertex_num);

	sg_end_pass();
	sg_commit();
}

void cleanup(){
	sg_shutdown();
	image_shutdown();
}


void event(const sapp_event* ev){
	handle_input(ev, &state.mouse);

	player.rz -= state.mouse.dx*SENSITIVITY; // z is the one that does that
	player.ry += state.mouse.dy*SENSITIVITY;
	if(player.ry > 90.0f){
		player.ry = 89.9f; // stops camera flipping
	}else if(player.ry < -90.0f){
		player.ry = -89.9f;
	}
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
