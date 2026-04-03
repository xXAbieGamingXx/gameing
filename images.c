#define STB_IMAGE_IMPLEMENTATION
#define SOKOL_FETCH_IMPL
#include <stdlib.h>
#include <stdio.h>
#include "sokol_fetch.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "util/stb/stb_image.h"
#include "util/fileutil.h"

// ids are in the h file

char filepaths[][30] = {
	"image.png",
};

uint16_t file_count;
uint8_t file_buffer[500000];

bool done_fetching = false;

struct image_dimensions {
	int width, height;
};

struct image_dimensions* image_dimensions;

sg_image* images;

int current_image;

static void fetch_cb(const sfetch_response_t* response);

void images_dowork(){
	sfetch_dowork();
}

void fetch_images(){
	sfetch_setup(&(sfetch_desc_t){
		.max_requests = 1,
		.num_channels = 1,
		.num_lanes = 1,
		.logger.func = slog_func,
	});

	file_count = sizeof(filepaths) / sizeof(filepaths[0]);
	images = malloc(sizeof(sg_image) * file_count);
	image_dimensions = malloc(sizeof(struct image_dimensions) * file_count);

	for(int i = 0; i < file_count; i++){
		char path_buf[512];
		sfetch_send(&(sfetch_request_t){
			.path = fileutil_get_path(filepaths[i], path_buf, sizeof(path_buf)),
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
				.data.mip_levels[0] = {
					.ptr = pixels,
					.size = (size_t)(width * height * 4), // byte for r g b a
				},
			});
			stbi_image_free(pixels);
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
			printf("BIG FAIL ALERT\n");
		}
	}
	current_image++;
	if(current_image == file_count){
		done_fetching = true;
		printf("done with all fetches\n");
	}
}
