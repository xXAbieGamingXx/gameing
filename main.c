#define SOKOL_GFX_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_FETCH_IMPL
#define SOKOL_GLUE_IMPL
#define VECMATH_GENERICS
#define SOKOL_GLCORE
#include <stdio.h>
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
} state;

struct Vertex{
	float x, y, z;
	uint16_t u, v;
} vertex;

#define WALKING_SPEED 10 // 10 units per sec
#define RECT_LOCATION 5 
#define HEIGHT_CONSTANT 100
#define FOV 60 // also defined in plane.c xd

double delta_time = 0;

static void fetch_cb(const sfetch_response_t* response);
static mvp_params_t calculate_mvp();

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

	state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.usage = {
			.immutable = false,
			.stream_update = true,
		},
		.size = sizeof(float)*30,
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

	const struct Vertex vertices[] = { // x, y, z, u (horizontal), v (vertical)
		{1.0f , 1.0f , .1f, 32767, 0    },
		{-1.0f, 1.0f , .1f, 0    , 0    },
		{1.0f , -1.0f, .1f, 32767, 32767},
		{-1.0f, -1.0f, .1f, 0    , 32767},
	};

	const mvp_params_t mvp_params = calculate_mvp();

	sg_update_buffer(state.bind.vertex_buffers[0], &SG_RANGE(vertices));
	sg_begin_pass(&(sg_pass){
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});

	if(player.x < RECT_LOCATION){
		sg_apply_pipeline(state.pip);
		sg_apply_bindings(&state.bind);
		sg_apply_uniforms(UB_mvp_params, &SG_RANGE(mvp_params));
		sg_draw(0, 6, 1);
	}

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

static mvp_params_t calculate_mvp(){
	float direction = key_down(SAPP_KEYCODE_W) - key_down(SAPP_KEYCODE_S);
	player.x += direction * WALKING_SPEED * delta_time;

	float distance = RECT_LOCATION - player.x;
	const float w = sapp_widthf();
	const float h = sapp_heightf();
	// rh = measuring angles counterclockwise
	mat44_t proj = mat44_perspective_fov_rh(vm_radians((float)FOV), w/h, .01f, 100.0f);
	// camera location in space, camera center (where it is looking), up direction
	mat44_t view = mat44_look_at_rh(vec3(player.x, player.y, player.z), vec3(10000.0f, 0,  0), vec3(.0f, .0f, 1.0f));
	mat44_t view_proj = vm_mul(view, proj);
	mat44_t translate = mat44_translation(distance, .0f, .0f); 
	// it just likes to put things on the ceiling
	mat44_t ry = mat44_rotation_y(vm_radians(0.0f));
	mat44_t rx = mat44_rotation_x(vm_radians(0.0f));
	mat44_t rotation = vm_mul(ry, rx);
	mat44_t model = vm_mul(translate, rotation);
	mat44_t mvp = vm_mul(model, view_proj);
	return (mvp_params_t){.mvp = mvp};
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
