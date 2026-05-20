#pragma once

struct image_dimensions {
	int width, height;
};

typedef struct sg_image sg_image;

extern struct image_dimensions* image_dimensions;
extern sg_image* images;
extern bool done_fetching;

void image_shutdown();
void fetch_images();
void images_dowork();
