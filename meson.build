project(
  'gaming',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
dl_dep = cc.find_library('dl', required : false)
thread_dep = dependency('threads')
gl_dep = dependency('vulkan')
x11_deps = [dependency('x11'), dependency('xi'), dependency('xcursor')]
app_deps = [m_dep, dl_dep, thread_dep, gl_dep] + x11_deps

run_command('bash', ['./gen_textures.sh'], check: true)

shaders = ['triangle.glsl', 'ui.glsl', 'model.glsl']
foreach shader : shaders
run_command('bash', ['./shdc.sh', shader], check: true)
endforeach

files = ['input.c', 'player.c', 'render.c', 'images.c', 'parse_map.c', 'ui.c', 'main.c', 'entities/platform.c', 'aabb.c', 'model.c', 'entities.c', 'entities/player_spawn.c']

exe = executable(
  'gaming',
  files,
  install : true,
  dependencies : app_deps
)

# test_files = ['util/vecmath.h', 'player.c', 'player.h', 'test/test.c', 'test/munit.c', 'test/munit.h']
#
# test = executable(
#   'unittest',
#   test_files,
#   c_args : ['-DTEST'],
#   dependencies : [m_dep]
# )

map_tool_files = ['tools/trenchbroom_map.c', 'entities.c']

map_tool_files = executable(
  'map_tool',
  map_tool_files,
  dependencies : [m_dep],
  c_args: ['-DMAP_TOOL']
)

glb_tool_files = ['tools/glb_tool.c']

glb_tool_files = executable(
  'glb_tool',
  glb_tool_files,
  dependencies : [m_dep],
)

# test('test cases', test)
