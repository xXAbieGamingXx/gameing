project(
  'gaming',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['warning_level=2', 'c_std=c11'],
)

shaders = ['rectangle.glsl']
foreach shader : shaders
  run_command('bash', ['./shdc.sh', shader], check: true)
endforeach

exe = executable(
  'gaming',
  ['main.c', 'main.h', 'sokol_app.h', 'sokol_gfx.h', 'sokol_glue.h', 'sokol_log.h', 'input.c', 'input.h', 'rectangle.glsl.h', 'util/stb/stb_image.h',
    'sokol_fetch.h', 'util/fileutil.h', 'util/fileutil.c', 'util/vecmath.h'],
  install : true,
)

test('basic', exe)
