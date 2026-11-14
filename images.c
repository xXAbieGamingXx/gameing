#define STB_IMAGE_IMPLEMENTATION
#define SOKOL_FETCH_IMPL
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sokol_fetch.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "util/stb/stb_image.h"
#include "images.h"
#include "atlas.h"

#define ATLAS_DIMS 2048
#define FILE_BUFFER_SIZE (4 * ATLAS_DIMS * ATLAS_DIMS + 1000)

static uint8_t* file_buffer;

bool done_fetching = false;

sg_image atlas_array;

uint8_t* atlas_pixels;
uint8_t images_fetched = 0;

static void fetch_cb(const sfetch_response_t* response);

void images_dowork(){
	sfetch_dowork();
}

void fetch_images(){
	file_buffer = malloc(FILE_BUFFER_SIZE);
	sfetch_setup(&(sfetch_desc_t){
		.max_requests = ATLAS_COUNT,
		.num_channels = 1,
		.num_lanes = 1,
		.logger.func = slog_func,
	});

	atlas_pixels = malloc(ATLAS_DIMS * ATLAS_DIMS * 4 * ATLAS_COUNT);

	for(int i = 0; i < ATLAS_COUNT; i++){
		sfetch_send(&(sfetch_request_t){
			.path = atlas_sheets[i].path,
			.callback = fetch_cb,
			.buffer = {file_buffer, FILE_BUFFER_SIZE},
			.user_data = SFETCH_RANGE(i)
		});
	}
}

void image_shutdown(){
	sfetch_shutdown();
}

static void fetch_cb(const sfetch_response_t* response){
	int slice = *(int*)response->user_data;
	if(response->fetched){
		int width, height, num_channels;
		stbi_uc* pixels = stbi_load_from_memory(
			response->data.ptr,
			(int)response->data.size,
			&width, &height,
			&num_channels, 4
		);
		if(pixels){
			memcpy(atlas_pixels + slice * ATLAS_DIMS * ATLAS_DIMS * 4, pixels, ATLAS_DIMS * ATLAS_DIMS * 4);
			stbi_image_free(pixels);
			images_fetched++;
		} else {
			printf("decode failed %s: %s\n", response->path, stbi_failure_reason());
		}
	} else if(response->finished) {
		printf("unfetched %s (error %d)\n", response->path, response->error_code);
		if(response->failed) {
			printf("BIG FAIL ALERT\n");
		}
	}

	
	if(!response->finished || images_fetched < ATLAS_COUNT) {
		return;
	}

	atlas_array = sg_make_image(&(sg_image_desc){
		.width = ATLAS_DIMS,
		.height = ATLAS_DIMS,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.num_slices = ATLAS_COUNT,
		.type = SG_IMAGETYPE_ARRAY,
		.data = {
			.ptr = atlas_pixels,
			.size = ATLAS_DIMS * ATLAS_DIMS * 4 * ATLAS_COUNT,
		},
	});
	free(atlas_pixels);
	done_fetching = true;
}
