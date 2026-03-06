#define SOKOL_IMPL
#define SOKOL_GLCORE
#define SOKOL_WWIN32_FORCE_WINMAIN
#include <stdio.h>
#include <stdbool.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "input.h"
#include "rectangle.glsl.h"

static struct{
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
} state;

#define WALKING_SPEED 10 // 10 units per sec
#define RECT_LOCATION 70 // +70 from 0,0
#define HEIGHT_CONSTANT 100

double delta_time = 0;

typedef struct Player {
	float x, y, z;
} Player;

Player player = {0};

void init(){
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.usage = {
			.immutable = false,
			.stream_update = true,
		},
		.size = sizeof(float)*18,
		.label = "rectangle_vertices",
	});

	sg_shader rect_shd = sg_make_shader(rectangle_shader_desc(sg_query_backend()));

	state.pip = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = rect_shd,
		.layout = {
			.attrs = {
				[ATTR_rectangle_pos].format = SG_VERTEXFORMAT_FLOAT3,
			},
		},
	});

	state.pass_action = (sg_pass_action){
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {.0f, .0f, .0f, 1.0f}
		},
	};
}

void frame(){
	delta_time = sapp_frame_duration();
	float direction = key_down(SAPP_KEYCODE_W) - key_down(SAPP_KEYCODE_S);
	player.x += direction * WALKING_SPEED * delta_time;
	float distance = RECT_LOCATION - player.x;
	float height = (HEIGHT_CONSTANT/distance)/720; // /720 to scale from 0 to 1
	float vertices[] = {
		height, height, .5f,
		-height, height, .5f,
		height, -height, .5f,
		-height, -height, .5f,
		-height, height, .5f,
		height, -height, .5f,
	};
	sg_update_buffer(state.bind.vertex_buffers[0], &SG_RANGE(vertices));
	sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});
	if(player.x < RECT_LOCATION){
		sg_apply_pipeline(state.pip);
		sg_apply_bindings(&state.bind);
		sg_draw(0, 6, 1);
	}
	sg_end_pass();
	sg_commit();
}

void cleanup(){
	sg_shutdown();
}

void event(const sapp_event* ev){
	handle_input(ev);
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
