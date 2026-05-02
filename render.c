#define VECMATH_GENERICS
#include <stdio.h>
#include <math.h>
#include "player.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "util/vecmath.h"
#include "rectangle.glsl.h"

#define FOV 60

static mat44_t proj; 
static float w;
static float h;

void render_init(){
	w = sapp_widthf();
	h = sapp_heightf();
	proj = mat44_perspective_fov_rh(vm_radians((float)FOV), w/h, .01f, 100.0f);
}

void draw_vertices(sg_pipeline pip, sg_bindings bind, uint32_t vertex_num){
	vec3_t direction = player_direction(); // dont want to look straight at (0, 0, 0)
	direction.x += player.x;
	direction.y += player.y;
	direction.z += player.z;
	mat44_t view = mat44_look_at_rh(vec3(player.x, player.y, player.z), direction, vec3(.0f, .0f, 1.0f));
	mat44_t view_proj = vm_mul(view, proj);

	const mvp_params_t mvp_params = (mvp_params_t){.mvp = view_proj};

	sg_apply_pipeline(pip);
	sg_apply_bindings(&bind);
	sg_apply_uniforms(UB_mvp_params, &SG_RANGE(mvp_params));
	sg_draw(0, vertex_num, 1);
}
