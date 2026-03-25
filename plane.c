#define VECMATH_GENERICS
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

static mat44_t proj; 
static float w;
static float h;

void plane_init(){
	w = sapp_widthf();
	h = sapp_heightf();
	proj = mat44_perspective_fov_rh(vm_radians((float)FOV), w/h, .01f, 100.0f);
}

void draw_plane(struct plane_desc* desc){ // will draw a plane oriented on the x axis with up as up
	desc->rx += 90;
	desc->rz += 270;
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
	mat44_t view = mat44_look_at_rh(vec(player.x, player.y, player.z), player_direction(), vec3(.0f, .0f, 1.0f));
	mat44_t view_proj = vm_mul(view, proj);
	mat44_t translation = mat44_translation(desc->x-player.x, desc->y-player.y, desc->z-player.z); // always will be defined
	mat44_t rotation;
	mat44_t model;
	mat44_t rx = mat44_rotation_x(vm_radians(desc->rx));
	mat44_t rz = mat44_rotation_z(vm_radians(desc->rz));
	if(desc->ry == 0.0f){
		rotation = vm_mul(rx, rz);
	}else{
		ry = mat44_rotation_y(vm_radians(desc->ry));
		mat44_t rxy = vm_mul(rx, ry);
		rotation = vm_mul(rxy, rz);
	}
	mat44_t scaling;
	bool scaling_defined = true;
	if(desc->sx == 1 && desc.sy == 1 && desc.sz == 1){
		model = vm_mul(rotation, translation);
	}else{
		scaling = mat44_scaling(sx, sy, sz);
		mat44_t scale_rotate = vm_mul(scaling, rotation);
	}
}
