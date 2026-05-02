project(
  'gaming',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['warning_level=2', 'asdjflasjdfbsfdycjx=348iosugbjyh'],
)

shaders = ['rectangle.glsl']
foreach shader : shaders
run_command('bash', ['./shdc.sh', shader], check: true)
endforeach

files = ['sokol_app.h', 'sokol_gfx.h', 'sokol_glue.h', 'sokol_log.h', 'input.c', 'input.h', 'rectangle.glsl.h', 'util/stb/stb_image.h', 
   'sokol_fetch.h', 'util/fileutil.h', 'util/fileutil.c', 'util/vecmath.h', 'player.c', 'player.h', 'render.h', 'render.c', 'images.c', 'images.h',
   'parse_map.c', 'parse_map.h', 'main.c', 'util/tinycthread.c', 'util/tinycthread.h']

exe = executable(
  'gaming',
  files,
  install : true,
)

test_files = ['util/vecmath.h', 'player.c', 'player.h', 'test/test.c', 'test/munit.c', 'test/munit.h']

test = executable(
  'unittest',
  test_files,
  c_args : ['-DTEST']
)

map_tool_files = ['tools/trenchbroom_map.c']

map_tool_files = executable(
  'map_tool',
  map_tool_files
)

test('test cases', test)
