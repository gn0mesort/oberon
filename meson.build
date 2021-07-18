project(
  'oberon',
  [ 'c', 'cpp' ],
  version: '0.1.0',
  license: 'AGPL-3.0',
  default_options: [
    'c_std=c18',
    'c_link_args=-Wl,-rpath=$ORIGIN/../lib',
    'cpp_std=c++20',
    'cpp_link_args=-Wl,-rpath=$ORIGIN/../lib',
    'warning_level=3',
    'b_ndebug=if-release',
    'b_pie=true'
  ]
)

glslc = generator(find_program('glslangValidator'),
                  output: '@PLAINNAME@.spv',
                  arguments: [ '-t', '-Os', '-g0', '--quiet', '--target-env', 'vulkan1.2', '-o', '@OUTPUT@',
                               '@EXTRA_ARGS@', '@INPUT@' ])

spv2cpp = find_program('tools/spv2cpp.py')

oberon_deps = [
  dependency('xcb'),
  dependency('xcb-randr'),
  dependency('vulkan')
]

oberon_incs = [
  include_directories('include')
]

shader_srcs = [ ]
subdir('src/shaders')

oberon_srcs = [
  files(
    'src/oberon/debug.cpp',
    'src/oberon/errors.cpp',
    'src/oberon/object.cpp',
    'src/oberon/context.cpp',
    'src/oberon/debug_context.cpp',
    'src/oberon/window.cpp',
    'src/oberon/renderer_3d.cpp',
    'src/oberon/shader_config.cpp'
  ),
  files(
    'src/oberon/detail/vulkan_function_table.cpp',
    'src/oberon/detail/x11.cpp'
  ),
  shader_srcs
]

oberon_lib = library(
  meson.project_name(),
  oberon_srcs,
  include_directories:  oberon_incs,
  dependencies: oberon_deps,
  version: meson.project_version()
)

oberon_dep = declare_dependency(
  link_with: oberon_lib,
  include_directories: oberon_incs
)

subdir('examples')
