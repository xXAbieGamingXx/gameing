@ctype mat4 mat44_t

@vs vs
layout(binding = 0) uniform mvp_params{
	mat4 mvp;
};

in vec4 pos;
in vec2 texcoord0;
in float texid;
out vec2 uv;
flat out int v_texid;

void main(){
	gl_Position = mvp * pos;
	uv = texcoord0;
	v_texid = int(texid);
}

@end

@fs fs
layout(binding=0) uniform texture2D tex0;
layout(binding=1) uniform texture2D tex1;

layout(binding=0) uniform sampler smp;

in vec2 uv;
flat in int v_texid;
out vec4 frag_color;

void main(){
	switch(v_texid){
		case 0: frag_color = texture(sampler2D(tex0, smp), uv); break;
		case 1: frag_color = texture(sampler2D(tex1, smp), uv); break;

		default: frag_color = vec4(1.0, 0.0, 1.0, 1.0); break;
	}
}

@end

@program triangle vs fs
