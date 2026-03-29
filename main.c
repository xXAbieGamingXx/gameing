#define SOKOL_GFX_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_FETCH_IMPL
#define SOKOL_GLUE_IMPL
#define VECMATH_GENERICS
#define SOKOL_GLCORE
#define STB_IMAGE_IMPLEMENTATION
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_fetch.h"
#include "sokol_glue.h"
#include "input.h"
#include "util/vecmath.h"
#include "rectangle.glsl.h"
#include "util/fileutil.h"
#include "util/stb/stb_image.h"
#include "plane.h"
#include "player.h"

static struct{
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
	uint8_t file_buffer[500000];
	struct mouse_state* mouse;
} state;

struct Vertex {
	float x, y, z;
	short u, v;
};

#define WALKING_SPEED 10 // 10 units per sec
#define FOV 60 // also defined in plane.c xd
#define SENSITIVITY .25

double delta_time = 0;

static void fetch_cb(const sfetch_response_t* response);

void init(){
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	sfetch_setup(&(sfetch_desc_t){
		.max_requests = 1,
		.num_channels = 1,
		.num_lanes = 1,
		.logger.func = slog_func,
	});

	plane_init();

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

	char path_buf[512];
	sfetch_send(&(sfetch_request_t){
		.path = fileutil_get_path("image.png", path_buf, sizeof(path_buf)),
		.callback = fetch_cb,
		.buffer = SFETCH_RANGE(state.file_buffer),
	});
}

void frame(){
	sfetch_dowork();

	delta_time = sapp_frame_duration();

	float direction = key_down(SAPP_KEYCODE_W) - key_down(SAPP_KEYCODE_S);

	vec3_t x_movement = player_direction();
	x_movement.z = 0;
	float factor = pow(x_movement.x, 2) + pow(x_movement.y, 2);
	float multiplier = direction * WALKING_SPEED * delta_time / factor;

	x_movement.x *= multiplier;
	x_movement.y *= multiplier;
	player_add_vector(x_movement);

	direction = key_down(SAPP_KEYCODE_A) - key_down(SAPP_KEYCODE_D);
	player.rz += 90;
	vec3_t y_movement = player_direction();
	player.rz -= 90;
	y_movement.z = 0;
	// factor will be the same
	multiplier = direction * WALKING_SPEED * delta_time / factor;

	y_movement.x *= multiplier;
	y_movement.y *= multiplier;
	player_add_vector(y_movement);
	
	printf("%f, %f, %f\n", player.x, player.y, player.z);

	sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});

	draw_plane(&(struct plane_desc){
		.x = 5,
		.pip = state.pip,
		.bind = state.bind,
	});

	// draw_plane(&(struct plane_desc){
	// 	.rz = 90,
	// 	.pip = state.pip,
	// 	.bind = state.bind,
	// });

	sg_end_pass();
	sg_commit();
}

void cleanup(){
	sfetch_shutdown();
	sg_shutdown();
}

static void fetch_cb(const sfetch_response_t* response){
	if(response->fetched){
		printf("sucessful fetch\n");
		int width, height, num_channels;
		const int desired_channels = 4;
		stbi_uc* pixels = stbi_load_from_memory(
			response->data.ptr,
			(int)response->data.size,
			&width, &height,
			&num_channels, desired_channels);
		if(pixels){
			sg_image image = sg_make_image(&(sg_image_desc){
				.width = width,
				.height = height,
				.pixel_format = SG_PIXELFORMAT_RGBA8,
				.data.mip_levels[0] = {
					.ptr = pixels,
					.size = (size_t)(width * height * 4), // byte for r g b a
				},
				.label = "image",
			});
			stbi_image_free(pixels);
			sg_init_view(state.bind.views[VIEW_tex], &(sg_view_desc){
				.texture = {.image = image,},
				.label = "texture-test",
			});
		}
	
	}else if(response->finished){
		printf("unfetched\n");
		if(response->error_code == 0){
			printf("no error\n");
		}else if(response->error_code == 1){
			printf("file not found\n");
		}else if(response->error_code == 2){
			printf("no buffer\n");
		}else if(response->error_code == 3){
			printf("buffer too small\n");
		}else if(response->error_code == 4){
			printf("unexpected eof\n");
		}else if(response->error_code == 5){
			printf("invalid http\n");
		}else if(response->error_code == 6){
			printf("cancelled\n");
		}else if(response->error_code == 7){
			printf("js other\n");
		}
		
		if(response->failed){
			state.pass_action = (sg_pass_action){
				.colors[0] = {
					.load_action = SG_LOADACTION_CLEAR,
					.clear_value = {1.0f, .0f, .0f, 1.0f}
				},
			};
		}
	}
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
