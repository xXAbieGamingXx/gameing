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

shaders = ['triangle.glsl', 'ui.glsl', 'model.glsl']
foreach shader : shaders
run_command('bash', ['./shdc.sh', shader], check: true)
endforeach

files = ['input.c', 'player.c', 'render.c', 'images.c', 'parse_map.c', 'ui.c', 'main.c', 'entities/platform.c', 'aabb.c', 'model.c', 'entities.c', 'entities/player_spawn.c', 'collision.c']

exe = executable(
  'gaming',
  files,
  install : true,
  dependencies : app_deps
)

map_tool_files = ['tools/trenchbroom_map.c', 'entities.c', 'collision.c']

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

add_languages('cpp', native : false)

rect_pack_files = ['assets/rect_pack/main.cpp', 'assets/rect_pack/rect_pack.cpp', 'assets/rect_pack/MaxRectsBinPack.cpp', 'assets/rect_pack/stb_rect_pack.cpp']

rect_pack_exe = executable(
  'rect_pack',
  rect_pack_files,
  cpp_args : ['-std=c++17'],
)
