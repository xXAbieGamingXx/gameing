@ctype mat4 mat44_t

@vs vs
layout(binding = 0) uniform mvp_params {
	mat4 mvp;
};

in vec4 pos;
in vec4 inst_pos;
in vec2 texcoord0;
out vec2 uv;

void main(){
	gl_Position = mvp * vec4(pos.xyz + inst_pos.xyz, 1.0);
	uv = texcoord0;
}

@end

@fs fs
layout(binding=0) uniform sampler smp;
layout(binding=0) uniform texture2D tex;

in vec2 uv;
out vec4 frag_color;

void main(){
	frag_color = texture(sampler2D(tex, smp), uv);
}

@end

@program model vs fs
