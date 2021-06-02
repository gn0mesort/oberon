project(
  'oberon',
  [ 'c', 'cpp' ],
  version: '0.1.0',
  license: 'AGPL-3.0',
  default_options: [
    'c_std=c18',
    'cpp_std=c++20',
    'warning_level=3',
    'b_ndebug=if-release',
    'b_pie=true'
  ]
)

oberon_deps = [
  dependency('xcb'),
  dependency('vulkan')
]

oberon_incs = [
  include_directories('include')
]

oberon_srcs = [
  files(
    'src/oberon/errors.cpp',
    'src/oberon/object.cpp',
    'src/oberon/context.cpp',
    'src/oberon/debug_context.cpp'
  ),
  files(
    'src/oberon/detail/graphics.cpp'
  )
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
