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
# glslangValidator doesn't appear to support --option=value syntax.
glsl = [ find_program('glslangValidator'), '--target-env', 'vulkan1.3', '--client', 'vulkan100', '--quiet', '-o',
         '@OUTPUT@', '@INPUT@' ]

vkfl_sub = subproject('vkfl', default_options: [ 'build_c=disabled', 'build_examples=disabled',
                                                 'build_tests=disabled'])

oberon_deps = [
]
oberon_args = [
  '-DMESON_PROJECT_VERSION="1.0.0"',
  '-DMESON_PROJECT_VERSION_MAJOR=1',
  '-DMESON_PROJECT_VERSION_MINOR=0',
  '-DMESON_PROJECT_VERSION_PATCH=0',
  # Useful for examples that want to locate shaders and other data files.
  '-DMESON_BUILD_DIRECTORY="@0@"'.format(meson.current_build_dir())
]
oberon_link_args = [ ]
oberon_incs = [
  include_directories('include')
]
oberon_srcs = [
  files('src/oberon/application.cpp', 'src/oberon/debug.cpp', 'src/oberon/errors.cpp', 'src/oberon/keys.cpp',
        'src/oberon/mouse.cpp', 'src/oberon/graphics.cpp')
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
    dependency('xcb-xinput'),
    dependency('xkbcommon'),
    dependency('xkbcommon-x11'),
    dependency('x11-xcb'),
    dependency('vulkan'),
    vkfl_sub.get_variable('vkfl_cpp_dep'),
    dependency('glm')
  ]
  oberon_linux_srcs = [
    files('src/oberon/linux/system.cpp', 'src/oberon/linux/input.cpp', 'src/oberon/linux/window.cpp',
          'src/oberon/linux/platform.cpp', 'src/oberon/linux/x11-errors.cpp', 'src/oberon/linux/vk.cpp',
          'src/oberon/linux/graphics.cpp', 'src/oberon/linux/vk_device.cpp', 'src/oberon/linux/nvidia_vk_device.cpp',
          'src/oberon/linux/amd_vk_device.cpp', 'src/oberon/linux/intel_vk_device.cpp')
  ]
  oberon_args += oberon_linux_args
  oberon_deps += oberon_linux_deps
  oberon_srcs += oberon_linux_srcs
endif


foreach arg : oberon_args
  add_project_arguments(arg, language: [ 'cpp' ])
endforeach

foreach link_arg : oberon_link_args
  add_project_link_arguments(link_arg, language: [ 'cpp' ])
endforeach

if host_machine.system() == 'linux'
  subdir('tools/linux')
endif


subdir('shaders')

oberon_lib = library('oberon', oberon_srcs, include_directories: oberon_incs, dependencies: oberon_deps)
oberon_dep = declare_dependency(link_with: oberon_lib, include_directories: oberon_incs)

subdir('examples')
