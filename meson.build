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

oberon_config = configuration_data()
if true
  version = meson.project_version().split('.')
  cpu = host_machine.cpu_family() == 'x86_64' ? 'amd64' : 'unknown'
  os = host_machine.system() == 'linux' ? 'linux' : 'unknown'
  wsi = get_option('linux_window_system')
  build_string = 'oberon v@0@ @1@-@2@-@3@'.format(meson.project_version(), cpu, os, wsi)
  oberon_config.set('CONFIGURATION_VERSION_MAJOR', version[0])
  oberon_config.set('CONFIGURATION_VERSION_MINOR', version[1])
  oberon_config.set('CONFIGURATION_VERSION_PATCH', version[2])
  oberon_config.set_quoted('CONFIGURATION_VERSION', meson.project_version())
  oberon_config.set('CONFIGURATION_CPU_FAMILY', 'configuration::cpu_family::@0@'.format(cpu))
  oberon_config.set_quoted('CONFIGURATION_CPU_FAMILY_NAME', cpu)
  oberon_config.set_quoted('CONFIGURATION_BUILD_STRING', build_string)
  if cpu == 'amd64'
    oberon_config.set('CONFIGURATION_CPU_FAMILY_AMD64', 1)
  else
    oberon_config.set('CONFIGURATION_CPU_FAMILY_UNKNOWN', 1)
  endif
  oberon_config.set('CONFIGURATION_OPERATING_SYSTEM', 'configuration::operating_system::@0@'.format(os))
  oberon_config.set_quoted('CONFIGURATION_OPERATING_SYSTEM_NAME', os)
  if os == 'linux'
    oberon_config.set('CONFIGURATION_OPERATING_SYSTEM_LINUX', 1)
  else
    oberon_config.set('CONFIGURATION_OPERATING_SYSTEM_UNKNOWN', 1)
  endif
  oberon_config.set('CONFIGURATION_WINDOW_SYSTEM', 'configuration::window_system::@0@'.format(wsi))
  oberon_config.set_quoted('CONFIGURATION_WINDOW_SYSTEM_NAME', wsi)
  if wsi == 'x11'
    oberon_config.set('CONFIGURATION_WINDOW_SYSTEM_X11', 1)
  else
    oberon_config.set('CONFIGURATION_WINDOW_SYSTEM_UNKNOWN', 1)
  endif
endif


oberon_args = [
]
oberon_link_args = [ ]
oberon_incs = [
  include_directories('include')
]
oberon_srcs = [
  configure_file(input: files('generated/configuration.hpp.in'), output: 'configuration.hpp',
                 configuration: oberon_config),
  files('src/oberon/debug.cpp', 'src/oberon/errors.cpp', 'src/oberon/application.cpp', 'src/oberon/system.cpp',
        'src/oberon/graphics_device.cpp', 'src/oberon/render_window.cpp', 'src/oberon/keys.cpp',
        'src/oberon/mouse.cpp'),
  files('src/oberon/internal/base/system_impl.cpp', 'src/oberon/internal/base/graphics_context.cpp',
        'src/oberon/internal/base/vulkan.cpp', 'src/oberon/internal/base/physical_graphics_device.cpp',
        'src/oberon/internal/base/graphics_device_impl.cpp', 'src/oberon/internal/base/render_window_impl.cpp')
]

if cxx.get_id() == 'clang' or cxx.get_id() == 'clang-cl'
  oberon_args += [ '-DUSING_CLANG' ]
endif

if host_machine.system() == 'linux'
  oberon_linux_args = [
  ]
  oberon_linux_deps = [
    dependency('uuid'),
    dependency('threads'),
  ]
  oberon_linux_srcs = [
  ]
  if get_option('linux_window_system') == 'x11'
    oberon_linux_deps += [
      dependency('xcb'),
      dependency('xcb-xkb'),
      dependency('xcb-xinput'),
      dependency('xkbcommon'),
      dependency('xkbcommon-x11'),
      dependency('nng')
    ]
    oberon_linux_srcs += [
      files('src/oberon/internal/linux/x11/system_impl.cpp', 'src/oberon/internal/linux/x11/wsi_context.cpp',
            'src/oberon/internal/linux/x11/graphics_device_impl.cpp', 'src/oberon/internal/linux/x11/wsi_worker.cpp',
            'src/oberon/internal/linux/x11/render_window_impl.cpp')
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

subdir('examples')
