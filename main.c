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
#include <string.h>

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "sokol_time.h"

#include "input.h"
#include "util/vecmath.h"
#include "triangle.glsl.h"
#include "assets.h"
#include "images.h"
#include "player.h"
#include "parse_map.h"
#include "render.h"
#include "ui.h"
#include "entities.h"
#include "aabb.h"
#include "constants.h"
#include "model.h"
#include "atlas.h"

static struct {
	sg_pipeline pip;
	sg_bindings bind;
	sg_bindings ent_bind; // entity geometry, vertex buffer rewritten each frame
	sg_bindings model_bind;
	sg_pipeline model_pip;
	sg_view model_views[ENTITY_COUNT];
	sg_pass_action pass_action;
	struct mouse_state mouse;
	bool loaded_images;
	struct triangle_data* triangles;
	struct entity* entities;
	struct vertex* ent_render; // ent_vertices offset to interpolated positions
	vec3_t player_prev; // player position before last tick
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

	state.bind.views[0] = sg_alloc_view();

	state.bind.samplers[SMP_smp] = sg_make_sampler(&(sg_sampler_desc) {
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
	});

	state.model_bind.samplers[SMP_smp] = sg_make_sampler(&(sg_sampler_desc) {
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
	});

	state.model_bind.views[0] = sg_alloc_view();

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
			.buffers[0].stride = sizeof(struct vertex),
			.attrs = {
				[ATTR_triangle_pos].format = SG_VERTEXFORMAT_FLOAT3,
				[ATTR_triangle_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
				[ATTR_triangle_atlasid].format = SG_VERTEXFORMAT_FLOAT,
				[ATTR_triangle_rect].format = SG_VERTEXFORMAT_FLOAT4,
			},
		},
		.index_type = SG_INDEXTYPE_UINT32,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
	});

	state.entities = malloc(sizeof(struct entity)*state.triangles->entity_count);
	uint16_t instances = 0;
	for(int i = 0; i < state.triangles->entity_count; i++) {
		state.entities[i].type = state.triangles->entities[i].type;
		state.entities[i].params = state.triangles->entities[i].params;
		state.entities[i].vertices = state.triangles->entities[i].vertices;
		state.entities[i].vertex_count = state.triangles->entities[i].vertex_count;
		state.entities[i].indices = state.triangles->entities[i].indices;
		state.entities[i].tri_count = state.triangles->entities[i].tri_count;

		state.entities[i].cb = entity_fptrs[state.entities[i].type].cb;
		state.entities[i].collide = entity_fptrs[state.entities[i].type].collide;
		state.entities[i].entity_values = NULL;
		state.entities[i].position = malloc(sizeof(vec3_t));
		*state.entities[i].position = state.triangles->entities[i].origin;
		state.entities[i].rx = 0.0f;
		state.entities[i].rz = 0.0f;
		state.entities[i].visible = true; // init runs after instance count

		if(model_paths[state.entities[i].type] != NULL && state.entities[i].visible) {
			model_instances[state.entities[i].type]++;
			state.entities[i].instance_id = instances;
			instances++;
		} else {
			state.entities[i].instance_id = -1;
		}
	}

	total_instances = instances;

	state.triangles->instance_positions = init_models(state.entities, instances, &state.model_bind, &state.model_pip, state.model_views);
	vec3_t* pos_ptr = state.triangles->instance_positions;

	for(uint32_t i = 0; i < state.triangles->entity_count; i++) {
		if(state.entities[i].instance_id != -1) {
			*pos_ptr = *state.entities[i].position;
			free(state.entities[i].position);
			state.entities[i].position = pos_ptr;
			pos_ptr++;
		}

		if(entity_fptrs[state.entities[i].type].init != NULL) {
			entity_fptrs[state.entities[i].type].init(&state.entities[i]);
		}

		state.entities[i].prev_position = *state.entities[i].position;
	}
	state.player_prev = vec3(player.x, player.y, player.z);
	state.ent_render = malloc(state.triangles->ent_vertex_count * sizeof(struct vertex));

	state.triangles->aabb_node_count = aabb_build(state.entities, state.triangles->entity_count, &state.triangles->aabb_nodes);

	free(state.triangles->entities);
}

void frame() {
	acc += sapp_frame_duration();

	while(acc >= TICK_LENGTH) {
		state.player_prev = vec3(player.x, player.y, player.z);
		for(uint32_t i = 0; i < state.triangles->entity_count; i++) {
			state.entities[i].prev_position = *state.entities[i].position;
		}
		player_frame(state.triangles->nodes, state.triangles);
		for(int i = 0; i < state.triangles->entity_count; i++) {
			if(state.entities[i].cb != NULL) {
				state.entities[i].cb(&state.entities[i]);
			}
		}
		aabb_refit(state.triangles->aabb_nodes, state.triangles->aabb_node_count);
		acc -= TICK_LENGTH;
	}

	reset_mouse(&state.mouse);

	if(key_down(SAPP_KEYCODE_ESCAPE)) {
		cleanup();
	}

	images_dowork();

	if(!state.loaded_images && done_fetching) {
		sg_init_view(state.bind.views[0], &(sg_view_desc) {
			.texture.image = atlas_array,
		});
		state.loaded_images = true;
	}

	sg_begin_pass(&(sg_pass) {
		.action = state.pass_action,
		.swapchain = sglue_swapchain(),
	});

	float alpha = acc / TICK_LENGTH;
	vec3_t eye = vec3_lerp(state.player_prev, vec3(player.x, player.y, player.z), alpha);

	draw_vertices(state.pip, &state.bind, vertex_num, eye);

	size_t size = state.triangles->ent_vertex_count * sizeof(struct vertex);
	if(state.triangles->ent_vertex_count > 0) {
		memcpy(state.ent_render, state.triangles->ent_vertices, size);
		for(uint32_t i = 0; i < state.triangles->entity_count; i++) {
			if(state.entities[i].visible && state.entities[i].instance_id == -1) {
				struct entity* entity = &state.entities[i];
				vec3_t lerped = vec3_lerp(entity->prev_position, *entity->position, alpha);
				vec3_t offset = vec3_sub(lerped, *entity->position);
				struct vertex* vertices = state.ent_render + (entity->vertices - state.triangles->ent_vertices);
				for(uint32_t j = 0; j < entity->vertex_count; j++) {
					vertices[j].x += offset.x;
					vertices[j].y += offset.y;
					vertices[j].z += offset.z;
				}
			}
		}

		sg_write_buffer_transient(&(sg_write_buffer_desc) {
			.dst.buffer = state.ent_bind.vertex_buffers[0],
			.src.data = { .ptr = state.ent_render, .size = size },
			.size = size
		});
		draw_vertices(state.pip, &state.ent_bind, state.triangles->ent_tri_count * 3, eye);
	}

	mat44_t view_proj = get_view_proj(eye);

	render_models(state.model_pip, &state.model_bind, state.entities, state.triangles->entity_count, state.model_views, state.triangles->instance_positions, view_proj);

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
	free(state.ent_render);
	free(state.triangles->ent_indices);
	free(state.entities);
	free(state.triangles->aabb_nodes);
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
