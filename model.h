#pragma once

#include <stdint.h>

#include "util/vecmath.h"

struct entity;
struct sg_bindings;
struct sg_pipeline;
struct sg_view;

vec3_t* init_models(struct entity* entities, uint16_t instances, struct sg_bindings* bind, struct sg_pipeline* pip, struct sg_view* views);

void render_models(struct sg_pipeline pip, struct sg_bindings* bind, struct entity* entities, uint16_t entity_count, struct sg_view* views, vec3_t* inst_pos, mat44_t view_proj);
