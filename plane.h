#pragma once

struct plane_desc {
	float x, y, z;
	float rx, ry, rz;
	float sx, sy, sz;
	sg_pipeline pip;
	sg_bindings bind;
};

void plane_init();
void draw_plane(struct plane_desc* desc);
