#define VECMATH_GENERICS
#include <stdint.h>
#include <stdio.h>

#include "sokol_gfx.h"

#include "util/vecmath.h"
#include "util/stb/stb_image.h"
#include "entities.h"
#include "model.glsl.h"

// call sg_draw_ex with the base vertex

vec3_t* init_models(struct entity* entities, uint16_t instances, sg_bindings* bind, sg_pipeline* pip, sg_view* views) {
	uint32_t vertex_size = 0;
	uint32_t index_size = 0;
	uint32_t texcoord0_size = 0;
	uint32_t offset_size = (instances) * 3 * sizeof(float);
	for(int i = 0; i < ENTITY_COUNT; i++) {
		if(model_instances[i] != 0) {
			// header: vertex, index, texcoord0, pixel sizes in bytes
			uint32_t sizes[4];
			FILE* file = fopen(model_paths[i], "rb");
			fread(sizes, sizeof(uint32_t), 4, file);

			model_buffer_indices[i].vertex_start = vertex_size / (3*sizeof(float));
			model_buffer_indices[i].index_start = index_size / sizeof(uint16_t);
			model_buffer_indices[i].vertex_count = sizes[0] / (3*sizeof(float));
			model_buffer_indices[i].index_count = sizes[1] / sizeof(uint16_t);
			vertex_size += sizes[0];
			index_size += sizes[1];
			texcoord0_size += sizes[2];

			fseek(file, 4*sizeof(uint32_t) + sizes[0] + sizes[1] + sizes[2], SEEK_SET);
			uint8_t* bytes = malloc(sizes[3]);
			fread(bytes, sizes[3], 1, file);

			int w = 0, h = 0, channels;
			stbi_uc* pixels = stbi_load_from_memory(bytes, sizes[3], &w, &h, &channels, 4);
			if(!pixels) printf("stbi: %s: %s\n", model_paths[i], stbi_failure_reason());

			sg_image img = sg_make_image(&(sg_image_desc) {
				.width = w,
				.height = h,
				.pixel_format = SG_PIXELFORMAT_RGBA8,
				.data = { .ptr = pixels, .size = w * h * 4 }
			});

			stbi_image_free(pixels);

			views[i] = sg_make_view(&(sg_view_desc) {
				.texture = { .image = img }
			});

			free(bytes);
			fclose(file);
		}
	}

	uint8_t* vertex_buffer = malloc(vertex_size);
	uint8_t* index_buffer = malloc(index_size);
	uint8_t* texcoord0_buffer = malloc(texcoord0_size);
	vec3_t* inst_pos_buffer = malloc(offset_size);
	uint8_t* buffers[3] = {vertex_buffer, index_buffer, texcoord0_buffer};
	uint32_t offsets[3] = {0};

	for(int i = 0; i < ENTITY_COUNT; i++) {
		if(model_instances[i] > 0) {
			uint32_t sizes[4];
			FILE* file = fopen(model_paths[i], "rb");
			fread(sizes, sizeof(uint32_t), 4, file);
			for(int j = 0; j < 3; j++) {
				fread(buffers[j] + offsets[j], sizes[j], 1, file);
				offsets[j] += sizes[j];
			}
			fclose(file);
		}
	}

	bind->vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
		.usage.vertex_buffer = true,
		.data = { .ptr = vertex_buffer, .size = vertex_size }
	});

	bind->vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc) {
		.usage.vertex_buffer = true,
		.usage.write_transient = true,
		.size = offset_size
	});

	bind->vertex_buffers[2] = sg_make_buffer(&(sg_buffer_desc) {
		.usage.vertex_buffer = true,
		.data = { .ptr = texcoord0_buffer, .size = texcoord0_size }
	});

	bind->index_buffer = sg_make_buffer(&(sg_buffer_desc) {
		.usage.index_buffer = true,
		.data = { .ptr = index_buffer, .size = index_size }
	});

	free(vertex_buffer);
	free(index_buffer);
	free(texcoord0_buffer);

	sg_shader model_shd = sg_make_shader(model_shader_desc(sg_query_backend()));

	*pip = sg_make_pipeline(&(sg_pipeline_desc) {
		.shader = model_shd,
		.layout = {
			.attrs = {
				[ATTR_model_pos] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
				[ATTR_model_inst_pos] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 1 },
				[ATTR_model_texcoord0] = { .format = SG_VERTEXFORMAT_FLOAT2, .buffer_index = 2 }
			},
			.buffers = {
				[0] = { .stride = sizeof(float) * 3, .step_func = SG_VERTEXSTEP_PER_VERTEX },
				[1] = { .stride = sizeof(float) * 3, .step_func = SG_VERTEXSTEP_PER_INSTANCE },
				[2] = { .stride = sizeof(float) * 2, .step_func = SG_VERTEXSTEP_PER_VERTEX },
			}
		},
		.index_type = SG_INDEXTYPE_UINT16,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL
		}
	});

	return inst_pos_buffer;
}

mat44_t get_model_matrix(float rx, float rz) {
	mat44_t rotx = mat44_rotation_x(vm_radians(rx));
	mat44_t rotz = mat44_rotation_z(vm_radians(rz));
	return vm_mul(rotx, rotz);
}

void render_models(sg_pipeline pip, sg_bindings* bind, struct entity* entities, uint16_t entity_count, sg_view* views, vec3_t* inst_pos, mat44_t view_proj) {
	sg_write_buffer_transient(&(sg_write_buffer_desc) {
		.dst.buffer = bind->vertex_buffers[1],
		.src.data = { .ptr = inst_pos, .size = total_instances * sizeof(vec3_t) },
		.size = total_instances * sizeof(vec3_t)
	});

	sg_apply_pipeline(pip);
	sg_apply_bindings(bind);

	int16_t entity_id = -1;

	for(int i = 0; i < entity_count; i++) {
		if(entities[i].instance_id != -1) {
			if(entities[i].type != entity_id) {
				bind->views[VIEW_tex] = views[entities[i].type];
				sg_apply_bindings(bind);
				entity_id = entities[i].type;
			}
			mat44_t mvp = vm_mul(get_model_matrix(entities[i].rx, entities[i].rz), view_proj);
			mvp_params_t params = (mvp_params_t) { .mvp = mvp };
			sg_apply_uniforms(UB_mvp_params, &SG_RANGE(params));
			struct model_buffer_index* buffer_indices = &model_buffer_indices[entities[i].type];
			sg_draw_ex(buffer_indices->index_start, buffer_indices->index_count, 1, buffer_indices->vertex_start, entities[i].instance_id);
		}
	}
}
