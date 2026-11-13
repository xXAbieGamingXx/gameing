#define STB_IMAGE_IMPLEMENTATION
#define SOKOL_FETCH_IMPL
#include <stdlib.h>
#include <stdio.h>
#include "sokol_fetch.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "util/stb/stb_image.h"
#include "textures.h"
#include "images.h"

static uint8_t file_buffer[3 * 1024 * 1024 + 100]; // supports 1024x1024 with no alpha with <= 100 bytes of non pixel info

bool done_fetching = false;

struct image_dimensions* image_dimensions;
sg_image* images;

static int current_image;

static void fetch_cb(const sfetch_response_t* response);

void images_dowork(){
	sfetch_dowork();
}

void fetch_images(){
	sfetch_setup(&(sfetch_desc_t){
		.max_requests = TEXTURE_COUNT,
		.num_channels = 1,
		.num_lanes = 1,
		.logger.func = slog_func,
	});

	images = malloc(sizeof(sg_image) * TEXTURE_COUNT);
	image_dimensions = malloc(sizeof(struct image_dimensions) * TEXTURE_COUNT);

	for(int i = 0; i < TEXTURE_COUNT; i++){
		sfetch_send(&(sfetch_request_t){
			.path = texture_paths[i],
			.callback = fetch_cb,
			.buffer = SFETCH_RANGE(file_buffer),
		});
	}
}

void image_shutdown(){
	free(images);
	free(image_dimensions);
	sfetch_shutdown();
}

static void fetch_cb(const sfetch_response_t* response){
	if(response->fetched){
		int width, height, num_channels;
		const int desired_channels = 4;
		stbi_uc* pixels = stbi_load_from_memory(
			response->data.ptr,
			(int)response->data.size,
			&width, &height,
			&num_channels, desired_channels
		);
		image_dimensions[current_image].width = width;
		image_dimensions[current_image].height = height;
		if(pixels){
			images[current_image] = sg_make_image(&(sg_image_desc){
				.width = width,
				.height = height,
				.pixel_format = SG_PIXELFORMAT_RGBA8,
				.data = {
					.ptr = pixels,
					.size = (size_t)(width * height * 4),
				},
			});
			stbi_image_free(pixels);
		}
	}else if(response->finished){
		printf("unfetched %s\n", response->path);
		if(response->failed) printf("BIG FAIL ALERT\n");
	}

	if(response->finished){
		current_image++;
		if(current_image == TEXTURE_COUNT) done_fetching = true;
	}
}
