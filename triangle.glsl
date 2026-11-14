@ctype mat4 mat44_t

@vs vs
layout(binding = 0) uniform mvp_params{
	mat4 mvp;
};

in vec4 pos;
in vec2 texcoord0;
in float atlasid;
in vec4 rect;
in vec2 lmcoord;
out vec2 uv;
out float atlas;
out vec4 tile_rect;
out vec2 lmuv;

void main(){
	gl_Position = mvp * pos;
	uv = texcoord0;
	atlas = atlasid;
	tile_rect = rect;
	lmuv = lmcoord;
}

@end

@fs fs
layout(binding=0) uniform texture2DArray tex;
layout(binding=0) uniform sampler smp;
layout(binding=1) uniform texture2D lightmap;
layout(binding=1) uniform sampler lm_smp;

in vec2 uv;
in float atlas;
in vec4 tile_rect;
in vec2 lmuv;
out vec4 frag_color;

void main(){
	vec4 albedo = texture(sampler2DArray(tex, smp), vec3(mix(tile_rect.xy, tile_rect.zw, fract(uv)), atlas));
	vec3 light = texture(sampler2D(lightmap, lm_smp), lmuv).rgb * 2.0; // stored at half for overbright
	frag_color = vec4(albedo.rgb * light, albedo.a);
}

@end

@program triangle vs fs
