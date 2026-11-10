#pragma once
#include <stdbool.h>
#include <stdint.h>

// screen space, pixels, origin top-left
struct ui_box {
	float x, y, w, h;
};

// colors are 0xAABBGGRR
void ui_init();
bool ui_load_atlas(const char* path);
void ui_rect(struct ui_box dst, uint32_t color);
void ui_sprite(struct ui_box dst, struct ui_box src, uint32_t color); // src in atlas pixels
void ui_draw();
