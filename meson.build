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
  dependency('vulkan'),
  vkfl_sub.get_variable('vkfl_cpp_dep')
]

version = meson.project_version().split('.')

oberon_args = [
  '-DMESON_PROJECT_NAME="@0@"'.format(meson.project_name()),
  '-DMESON_PROJECT_VERSION="@0@"'.format(meson.project_version()),
  '-DMESON_PROJECT_VERSION_MAJOR=@0@'.format(version[0]),
  '-DMESON_PROJECT_VERSION_MINOR=@0@'.format(version[1]),
  '-DMESON_PROJECT_VERSION_PATCH=@0@'.format(version[2]),
  # Useful for examples that want to locate shaders and other data files.
  '-DMESON_BUILD_DIRECTORY="@0@"'.format(meson.current_build_dir())
]
oberon_link_args = [ ]
oberon_incs = [
  include_directories('include')
]
oberon_srcs = [
  files('src/oberon/debug.cpp', 'src/oberon/errors.cpp', 'src/oberon/system.cpp', 'src/oberon/graphics_device.cpp'),
  files('src/oberon/internal/vulkan.cpp', 'src/oberon/internal/graphics_context.cpp', 'src/oberon/internal/system.cpp',
        'src/oberon/internal/graphics_device.cpp')
]

if cxx.get_id() == 'clang' or cxx.get_id() == 'clang-cl'
  oberon_args += [ '-DUSING_CLANG' ]
endif

if host_machine.system() == 'linux'
  oberon_linux_args = [
    '-DMESON_SYSTEM="linux"',
    '-DMESON_SYSTEM_LINUX=1',
    '-DMESON_SYSTEM_LINUX_WSI_TYPE="@0@"'.format(get_option('linux_wsi_type'))
  ]
  oberon_linux_deps = [
    dependency('glm')
  ]
  oberon_linux_srcs = [
    files('src/oberon/internal/linux/wsi_graphics_device.cpp', 'src/oberon/internal/linux/wsi_system.cpp')
  ]
  if get_option('linux_wsi_type') == 'x11'
    oberon_linux_args += [
      '-DMESON_SYSTEM_LINUX_WSI_TYPE_X11=1'
    ]
    oberon_linux_deps += [
      dependency('xcb'),
      dependency('xcb-xkb'),
      dependency('xcb-xinput'),
      dependency('xkbcommon'),
      dependency('xkbcommon-x11')
    ]
    oberon_linux_srcs += [
      files('src/oberon/internal/linux/xcb/wsi_context.cpp')
    ]
  endif
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


#subdir('shaders')

oberon_lib = library('oberon', oberon_srcs, include_directories: oberon_incs, dependencies: oberon_deps)
oberon_dep = declare_dependency(link_with: oberon_lib, include_directories: oberon_incs)

#subdir('examples')
