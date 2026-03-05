project(
  'gaming',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['warning_level=2', 'c_std=c11'],
)

# sokol_shdc = find_program('C:\\Users\\Macch\\OneDrive\\Desktop\\s\\gaming\\tools\\sokol-shdc')

# triangle = custom_target(
#   'shaders',
#   input : 'triangle.glsl',
#   output : 'triangle.glsl.h',
#   command : [
#     sokol_shdc,
#     '--input', '@INPUT@',
#     '--output', '@OUTPUT@',
#     '--slang', 'glsl430:hlsl5:metal_macos'
#   ]
# )

# triangle_dep = declare_dependency(sources: triangle)

# root = include_directories('.')

exe = executable(
  'gaming',
  ['main.c', 'sokol_app.h', 'sokol_gfx.h', 'sokol_glue.h', 'sokol_log.h'],
  # dependencies : triangle_dep,
  install : true,
)

test('basic', exe)
