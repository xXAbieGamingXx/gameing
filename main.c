#if defined(__APPLE__)
	#define SOKOL_METAL
#else
	#define SOKOL_VULKAN
	#define VK_USE_PLATFORM_XLIB_KHR
#endif
#define _POSIX_C_SOURCE 199309L

#define SOKOL_GFX_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_TIME_IMPL

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
#include "triangle.glsl.h"
#include "textures.h"
#include "images.h"
#include "player.h"
#include "parse_map.h"
#include "render.h"
#include "ui.h"
#include "entities.h"
#include "constants.h"

// TODO: config the pixel format, stuff like msaa
// for gui use imgui and include the frame stuff inside the sg pass, draw after (probably)

static struct{
	sg_pipeline pip;
	sg_bindings bind;
	sg_bindings ent_bind; // entity geometry, vertex buffer rewritten each frame
	sg_pass_action pass_action;
	struct mouse_state mouse;
	bool loaded_images;
	struct triangle_data* triangles;
	struct entity* entities;
} state;

#define SENSITIVITY .25f

double delta_time = 0;
double acc = 0;
uint32_t vertex_num;

void cleanup();

void init() {
	state.loaded_images = false;

	sg_setup(&(sg_desc) {
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	stm_setup();

	render_init();
	ui_init();

	sapp_lock_mouse(true);
	sapp_show_mouse(false);

	fetch_images();

	state.pass_action = (sg_pass_action) {
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {.0f, .0f, .0f, 1.0f}
		},
	};

	for(int i = 0; i < TEXTURE_COUNT; i++) {
		state.bind.views[i] = sg_alloc_view();
	}

	state.bind.samplers[SMP_smp] = sg_make_sampler(&(sg_sampler_desc) {
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
	});

	char* file = "maps/untitled.lvl";
	state.triangles = get_triangles(file);

	state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
		.data = (sg_range) { .ptr = state.triangles->vertices, .size = state.triangles->vertex_count * sizeof(struct vertex) },
	});

	state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc) {
		.usage.index_buffer = true,
		.data = (sg_range) { .ptr = state.triangles->indices, .size = state.triangles->tri_count * 3 * sizeof(uint32_t) },
	});

	vertex_num = state.triangles->tri_count * 3;

	if(state.triangles->ent_vertex_count > 0) {
		state.ent_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
			.size = state.triangles->ent_vertex_count * sizeof(struct vertex),
			.usage = { .vertex_buffer = true, .write_transient = true },
		});

		state.ent_bind.index_buffer = sg_make_buffer(&(sg_buffer_desc) {
			.usage.index_buffer = true,
			.data = (sg_range) { .ptr = state.triangles->ent_indices, .size = state.triangles->ent_tri_count * 3 * sizeof(uint32_t) },
		});

		for(int i = 0; i < TEXTURE_COUNT; i++) {
			state.ent_bind.views[i] = state.bind.views[i];
		}
		state.ent_bind.samplers[SMP_smp] = state.bind.samplers[SMP_smp];
	}

	sg_shader tri_shd = sg_make_shader(triangle_shader_desc(sg_query_backend()));

	state.pip = sg_make_pipeline(&(sg_pipeline_desc) {
		.shader = tri_shd,
		.layout = {
			.attrs = {
				[ATTR_triangle_pos].format = SG_VERTEXFORMAT_FLOAT3,
				[ATTR_triangle_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
				[ATTR_triangle_texid].format = SG_VERTEXFORMAT_FLOAT,
			},
		},
		.index_type = SG_INDEXTYPE_UINT32,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
	});

	state.entities = malloc(sizeof(struct entity)*state.triangles->entity_count);
	for(int i = 0; i < state.triangles->entity_count; i++) {
		state.entities[i].type = state.triangles->entities[i].type;
		state.entities[i].params = state.triangles->entities[i].params;
		state.entities[i].vertices = state.triangles->entities[i].vertices;
		state.entities[i].vertex_count = state.triangles->entities[i].vertex_count;
		state.entities[i].indices = state.triangles->entities[i].indices;
		state.entities[i].tri_count = state.triangles->entities[i].tri_count;

		state.entities[i].cb = entity_fptrs[state.entities[i].type].cb;
		state.entities[i].entity_values = NULL;

		if(entity_fptrs[state.entities[i].type].init != NULL) {
			entity_fptrs[state.entities[i].type].init(&state.entities[i]);
		}
	}

	free(state.triangles->entities);
}

void frame() {
	acc += sapp_frame_duration();

	while(acc >= TICK_LENGTH) {
		player_frame(state.triangles->nodes, state.triangles);
		for(int i = 0; i < state.triangles->entity_count; i++) {
			if(state.entities[i].cb != NULL) {
				state.entities[i].cb(&state.entities[i]);
			}
		}
		acc -= TICK_LENGTH;
	}

	reset_mouse(&state.mouse);

	if(key_down(SAPP_KEYCODE_ESCAPE)) {
		cleanup();
	}

	images_dowork();

	if(!state.loaded_images && done_fetching) {
		for(int i = 0; i < TEXTURE_COUNT; i++) {
			sg_init_view(state.bind.views[i], &(sg_view_desc) {
				.texture = {.image = images[i]},
			});
		}
		state.loaded_images = true;
	}

	sg_begin_pass(&(sg_pass) {
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});

	draw_vertices(state.pip, &state.bind, vertex_num);

	size_t size = state.triangles->ent_vertex_count * sizeof(struct vertex);
	if(state.triangles->ent_vertex_count > 0) {
		sg_write_buffer_transient(&(sg_write_buffer_desc) {
			.dst.buffer = state.ent_bind.vertex_buffers[0],
			.src.data = { .ptr = state.triangles->ent_vertices, .size = size },
			.size = size
		});
		draw_vertices(state.pip, &state.ent_bind, state.triangles->ent_tri_count * 3);
	}

	ui_rect((struct ui_box) {sapp_widthf()/2 - 2, sapp_heightf()/2 - 2, 4, 4}, 0xFFFFFFFF); // crosshair
	ui_draw();

	sg_end_pass();
	sg_commit();
}

void cleanup() {
	free(state.triangles->vertices);
	free(state.triangles->indices);
	free(state.triangles->nodes);
	for(uint32_t i = 0; i < state.triangles->entity_count; i++) {
		free(state.entities[i].params);
		free(state.entities[i].entity_values);
	}
	free(state.triangles->ent_vertices);
	free(state.triangles->ent_indices);
	free(state.entities);
	free(state.triangles);
	sg_shutdown();
	image_shutdown();
}


void event(const sapp_event* ev) {
	handle_input(ev, &state.mouse);
	player.rz -= state.mouse.dx*SENSITIVITY; // z is the one that does that
	player.ry += state.mouse.dy*SENSITIVITY;
	if(player.ry > 90.0f) {
		player.ry = 89.9f; // stops camera flipping
	} else if (player.ry < -90.0f) {
		player.ry = -89.9f;
	}
}

sapp_desc sokol_main(int argc, char* argv[]) {
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
		.fullscreen = true,
	};
}
