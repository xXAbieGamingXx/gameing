@ctype mat4 mat44_t

@vs vs
layout(binding = 0) uniform mvp_params{
	mat4 mvp;
};

in vec4 pos;
in vec2 texcoord0;
in float atlasid;
in vec4 rect;
out vec2 uv;
out float atlas;

void main(){
	gl_Position = mvp * pos;
	uv = mix(rect.xy, rect.zw, texcoord0);
	atlas = atlasid;
}

@end

@fs fs
layout(binding=0) uniform texture2DArray tex;
layout(binding=0) uniform sampler smp;

in vec2 uv;
in float atlas;
out vec4 frag_color;

void main(){
	frag_color = texture(sampler2DArray(tex, smp), vec3(uv, atlas));
}

@end

@program triangle vs fs
