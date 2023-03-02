##
# @file meson.build
# @brief Project level Meson build script.
# @author Alexander Rothman <gnomesort@megate.ch>
# @date 2023
# @copyright AGPL-3.0+
#
project(
  'oberon',
  [ 'cpp' ],
  version: '1.0.0',
  license: 'AGPL-3.0+',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'b_ndebug=if-release',
    'b_pie=true'
  ],
  subproject_dir: 'lib'
)

cxx = meson.get_compiler('cpp')

oberon_deps = [ ]
oberon_link_args = [ ]
oberon_args = [ ]
oberon_incs = [
  include_directories('include')
]
oberon_srcs = [
  files('src/main.cpp'),
  files('src/oberon/application.cpp', 'src/oberon/debug.cpp', 'src/oberon/errors.cpp', 'src/oberon/keys.cpp')
]



if cxx.get_id() == 'clang' or cxx.get_id() == 'clang-cl'
  oberon_args += [ '-DUSING_CLANG' ]
endif

if host_machine.system() == 'linux'
  oberon_linux_args = [
    '-DMESON_SYSTEM="linux"',
    '-DMESON_SYSTEM_LINUX=1'
  ]
  oberon_linux_deps = [
    dependency('x11'),
    dependency('xcb'),
    dependency('xcb-xkb'),
    dependency('xkbcommon'),
    dependency('xkbcommon-x11'),
    dependency('x11-xcb')
  ]
  oberon_linux_srcs = [
    files('src/oberon/linux/system.cpp', 'src/oberon/linux/input.cpp', 'src/oberon/linux/window.cpp')
  ]
  oberon_args += oberon_linux_args
  oberon_deps += oberon_linux_deps
  oberon_srcs += oberon_linux_srcs
endif

foreach link_arg : oberon_link_args
  add_project_link_arguments(link_arg, language: [ 'cpp' ])
endforeach

foreach arg : oberon_args
  add_project_arguments(arg, language: [ 'cpp' ])
endforeach

if host_machine.system() == 'linux'
  subdir('tools/linux')
endif

oberon = executable('oberon', oberon_srcs, include_directories: oberon_incs, dependencies: oberon_deps)
