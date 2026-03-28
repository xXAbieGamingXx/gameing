#define VECMATH_GENERICS
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "player.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "util/vecmath.h"
#include "rectangle.glsl.h"

#define FOV 60

struct plane_desc {
	float x, y, z;
	float rx, ry, rz;
	float sx, sy, sz;
	sg_pipeline pip;
	sg_bindings bind;
};

struct Vertex {
	float x, y, z;
	float u, v;
};

static mat44_t proj; 
static float w;
static float h;

static void draw_plane_helper(struct plane_desc* desc);

void plane_init(){
	w = sapp_widthf();
	h = sapp_heightf();
	proj = mat44_perspective_fov_rh(vm_radians((float)FOV), w/h, .01f, 100.0f);
}

void draw_plane(struct plane_desc* desc){ // will draw a plane oriented on the x axis with up as up
	desc->rx += 90;
	if(desc->sx == 0){
		desc->sx = 1;
	}

	if(desc->sy == 0){
		desc->sy = 1;
	}

	if(desc->sz == 0){
		desc->sz = 1;
	}

	draw_plane_helper(desc);
}

static void draw_plane_helper(struct plane_desc* desc){
	// math.h trig is in radians
	vec3_t direction = player_direction(); // dont want to look straight at (0, 0, 0)
	direction.x += player.x;
	direction.y += player.y;
	direction.z += player.z;
	mat44_t view = mat44_look_at_rh(vec3(player.x, player.y, player.z), direction, vec3(.0f, .0f, 1.0f));
	mat44_t view_proj = vm_mul(view, proj);
	mat44_t translation = mat44_translation(desc->x-player.x, desc->y-player.y, desc->z-player.z); // always will be defined
	mat44_t rotation;
	mat44_t model;
	mat44_t rx = mat44_rotation_x(vm_radians(desc->rx));
	mat44_t rz = mat44_rotation_z(vm_radians(desc->rz));
	mat44_t ry = mat44_rotation_y(vm_radians(desc->ry));
	mat44_t rxy = vm_mul(ry, rx);
	rotation = vm_mul(rz, rxy);
	mat44_t scaling;
	if(desc->sx == 1 && desc->sy == 1 && desc->sz == 1){
		model = vm_mul(translation, rotation);
	}else{
		scaling = mat44_scaling(desc->sx, desc->sy, desc->sz);
		mat44_t scale_rotate = vm_mul(scaling, rotation);
		model = vm_mul(translation, scale_rotate);
	}
	mat44_t mvp = vm_mul(model, view_proj);

	const mvp_params_t mvp_params = (mvp_params_t){.mvp = mvp};

	sg_apply_pipeline(desc->pip); // might ned to pass in a pointer
	sg_apply_bindings(&desc->bind);
	sg_apply_uniforms(UB_mvp_params, &SG_RANGE(mvp_params));
	sg_draw(0, 6, 1);
}
