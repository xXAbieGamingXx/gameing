#pragma once

typedef struct sg_image sg_image;

extern sg_image atlas_array;
extern bool done_fetching;

void image_shutdown();
void fetch_images();
void images_dowork();
