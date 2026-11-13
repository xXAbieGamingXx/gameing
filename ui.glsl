@vs ui_vs
layout(binding = 0) uniform ui_params {
	vec2 screen;
};

in vec2 pos;
in vec2 texcoord0;
in vec4 color0;
out vec2 uv;
out vec4 color;

void main() {
	vec2 ndc = pos / screen * 2.0 - 1.0;
	gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
	uv = texcoord0;
	color = color0;
}
@end

@fs ui_fs
layout(binding = 0) uniform texture2D atlas;
layout(binding = 0) uniform sampler atlas_smp;

in vec2 uv;
in vec4 color;
out vec4 frag_color;

void main() {
	if(uv.x < 0.0) {
		frag_color = color;
	} else {
		frag_color = color * texture(sampler2D(atlas, atlas_smp), uv);
	}
}
@end

@program ui ui_vs ui_fs
