#include <stdio.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "util/stb/stb_image.h"
#include "ui.glsl.h"
#include "ui.h"

#define UI_MAX_VERTS 6144

struct ui_vertex {
	float x, y;
	float u, v;
	uint32_t color;
};

static struct ui_vertex verts[UI_MAX_VERTS];
static int vert_count;
static sg_pipeline pip;
static sg_bindings bind;
static sg_image atlas;
static float atlas_w, atlas_h;

void ui_init() {
	bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
		.size = sizeof(verts),
		.usage = { .vertex_buffer = true, .write_transient = true },
	});

	static const uint32_t white = 0xFFFFFFFF; // placeholder until atlas loads
	atlas = sg_make_image(&(sg_image_desc) {
		.width = 1,
		.height = 1,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data = SG_RANGE(white),
	});
	atlas_w = atlas_h = 1.0f;

	bind.views[VIEW_atlas] = sg_make_view(&(sg_view_desc) {
		.texture = {.image = atlas},
	});

	bind.samplers[SMP_atlas_smp] = sg_make_sampler(&(sg_sampler_desc) {
		.min_filter = SG_FILTER_NEAREST,
		.mag_filter = SG_FILTER_NEAREST,
		.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
		.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
	});

	pip = sg_make_pipeline(&(sg_pipeline_desc) {
		.shader = sg_make_shader(ui_shader_desc(sg_query_backend())),
		.layout.attrs = {
			[ATTR_ui_pos].format = SG_VERTEXFORMAT_FLOAT2,
			[ATTR_ui_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
			[ATTR_ui_color0].format = SG_VERTEXFORMAT_UBYTE4N,
		},
		.colors[0].blend = {
			.enabled = true,
			.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
			.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		},
	});
}

bool ui_load_atlas(const char* path) {
	int w, h, channels;
	stbi_uc* pixels = stbi_load(path, &w, &h, &channels, 4);
	if(!pixels) {
		printf("ui: failed to load atlas %s\n", path);
		return false;
	}

	sg_image img = sg_make_image(&(sg_image_desc) {
		.width = w,
		.height = h,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data = { .ptr = pixels, .size = (size_t)(w * h * 4) },
	});
	stbi_image_free(pixels);

	sg_destroy_view(bind.views[VIEW_atlas]);
	sg_destroy_image(atlas);

	atlas = img;
	atlas_w = (float)w;
	atlas_h = (float)h;
	bind.views[VIEW_atlas] = sg_make_view(&(sg_view_desc) {
		.texture = {.image = atlas},
	});
	return true;
}

static void push_quad(struct ui_box d, float u0, float v0, float u1, float v1, uint32_t color) {
	if(vert_count + 6 > UI_MAX_VERTS) return;
	float x1 = d.x + d.w;
	float y1 = d.y + d.h;
	struct ui_vertex* v = &verts[vert_count];
	v[0] = (struct ui_vertex) {d.x, d.y, u0, v0, color};
	v[1] = (struct ui_vertex) {x1,  d.y, u1, v0, color};
	v[2] = (struct ui_vertex) {x1,  y1,  u1, v1, color};
	v[3] = (struct ui_vertex) {d.x, d.y, u0, v0, color};
	v[4] = (struct ui_vertex) {x1,  y1,  u1, v1, color};
	v[5] = (struct ui_vertex) {d.x, y1,  u0, v1, color};
	vert_count += 6;
}

void ui_rect(struct ui_box dst, uint32_t color) {
	push_quad(dst, -1.0f, -1.0f, -1.0f, -1.0f, color);
}

void ui_sprite(struct ui_box dst, struct ui_box src, uint32_t color) {
	push_quad(dst, src.x / atlas_w, src.y / atlas_h, (src.x + src.w) / atlas_w, (src.y + src.h) / atlas_h, color);
}

void ui_draw() {
	if(vert_count == 0) return;

	size_t size = vert_count * sizeof(struct ui_vertex);
	sg_write_buffer_transient(&(sg_write_buffer_desc) {
		.src.data = { .ptr = verts, .size = size },
		.dst.buffer = bind.vertex_buffers[0],
		.size = size,
	});

	ui_params_t params = { .screen = { sapp_widthf(), sapp_heightf() } };

	sg_apply_pipeline(pip);
	sg_apply_bindings(&bind);
	sg_apply_uniforms(UB_ui_params, &SG_RANGE(params));
	sg_draw(0, vert_count, 1);

	vert_count = 0;
}
