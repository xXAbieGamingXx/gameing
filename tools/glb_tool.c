#include <stdio.h>
#include <stdint.h>

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "util/cgltf.h"
#include "util/stb/stb_image.h"

struct glb_data {
	uint32_t vertices_size;
	uint32_t indices_size;
	uint32_t texcoord0_size;
	uint32_t pixel_size;
	uint8_t* vertices;
	uint8_t* indices;
	uint8_t* texcoord0;
	uint8_t* pixels;
};

// TODO: add joints/weights for rigging

int main(int argc, char** argv) {
	if(argc != 3) {
		printf("expected 3 args");
		return 1;
	}
	struct glb_data glb = {0};
	cgltf_options options = {0};
	cgltf_data* data = NULL;
	char* filepath = argv[1];
	if(cgltf_parse_file(&options, filepath, &data) != cgltf_result_success) {
		printf("cgltf failed to parse %s\n", filepath);
		return 1;
	}
	cgltf_load_buffers(&options, data, filepath);


	const cgltf_primitive* primitive = &data->meshes[0].primitives[0];

	if(data->meshes_count != 1) {
		printf("cgltf gave multiple meshes, expected one\n");
		return 1;
	}

	if(data->meshes[0].primitives_count != 1) {
		printf("cgltf gave multiple primitives, expected one\n");
		return 1;
	}

	if(primitive->indices->component_type != cgltf_component_type_r_16u) {
		printf("unexpected index type: expected uint16");
		return 1;
	}

	for(cgltf_size i = 0; i < primitive->attributes_count; i++) {
		if(primitive->attributes[i].type == cgltf_attribute_type_position) {
			glb.vertices = primitive->attributes[i].data->buffer_view->buffer->data + primitive->attributes[i].data->buffer_view->offset + primitive->attributes[i].data->offset;
			glb.vertices_size = primitive->attributes[i].data->count * primitive->attributes[i].data->stride;
		} else if(primitive->attributes[i].type == cgltf_attribute_type_texcoord) {
			glb.texcoord0 = primitive->attributes[i].data->buffer_view->buffer->data + primitive->attributes[i].data->buffer_view->offset + primitive->attributes[i].data->offset;
			glb.texcoord0_size = primitive->attributes[i].data->count * primitive->attributes[i].data->stride;
		}
	}

	glb.indices = primitive->indices->buffer_view->buffer->data + primitive->indices->buffer_view->offset + primitive->indices->offset;
	glb.indices_size = primitive->indices->count * primitive->indices->stride;

	cgltf_image* img = primitive->material->pbr_metallic_roughness.base_color_texture.texture->image;
	uint8_t* img_bytes = img->buffer_view->buffer->data + img->buffer_view->offset;

	glb.pixels = img_bytes;
	glb.pixel_size = img->buffer_view->size;

	FILE* file = fopen(argv[2], "wb");

	fwrite(&glb.vertices_size, sizeof(uint32_t), 1, file);
	fwrite(&glb.indices_size, sizeof(uint32_t), 1, file);
	fwrite(&glb.texcoord0_size, sizeof(uint32_t), 1, file);
	fwrite(&glb.pixel_size, sizeof(uint32_t), 1, file);
	fwrite(glb.vertices, glb.vertices_size, 1, file);
	fwrite(glb.indices , glb.indices_size, 1, file);
	fwrite(glb.texcoord0, glb.texcoord0_size, 1, file);
	fwrite(glb.pixels, glb.pixel_size, 1, file);

	fclose(file);

	cgltf_free(data);

	return 0;
}
