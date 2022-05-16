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

#cmake = import('cmake')

project_root = meson.current_source_dir()

vkfl_proj = subproject('vkfl', version: '>=1.3.0',
                       default_options: [ 'api=1.3', 'build_c=disabled',
                                          'extensions=VK_EXT_debug_utils,VK_EXT_validation_features,' +
                                          'VK_KHR_xcb_surface,VK_KHR_surface,VK_KHR_swapchain',
                                          'specification=@0@'.format(project_root / 'share/vulkan/registry/vk.xml') ])
vkfl_dep = vkfl_proj.get_variable('vkfl_cpp_dep')

# Configure VMA
#vma_opts = cmake.subproject_options()
#vma_opts.append_compile_args('cpp', '-w')
#vma_opts.append_compile_args('cpp', '-DVMA_VULKAN_VERSION=1002000')
#vma_opts.append_compile_args('cpp', '-DVMA_STATS_STRING_ENABLED=0')
#vma_opts.add_cmake_defines({ 'VMA_STATIC_VULKAN_FUNCTIONS': 'OFF' })
#vma_opts.add_cmake_defines({ 'BUILD_SHARED_LIBS': 'ON' })
#vma_proj = cmake.subproject('VulkanMemoryAllocator', options: vma_opts)
#vma_dep = vma_proj.dependency('VulkanMemoryAllocator')

glslc = generator(find_program('glslangValidator'),
                  output: '@PLAINNAME@.spv',
                  arguments: [ '-t', '-Os', '-g0', '--quiet', '--target-env', 'vulkan1.2', '-o', '@OUTPUT@',
                               '@EXTRA_ARGS@', '@INPUT@' ])

spv2cpp = find_program('tools/spv2cpp.py')

oberon_deps = [
  vkfl_dep,
  #vma_dep,
  dependency('xcb'),
  dependency('vulkan', version: '>=1.3.0')
]

oberon_incs = [
  include_directories('include')
]

#shader_srcs = [ ]
#subdir('src/shaders')

oberon_srcs = [ ]

oberon_srcs += [
  files('src/oberon/debug.cpp')
]

if host_machine.system() == 'linux'
  oberon_srcs += [
    files('src/oberon/application.cpp', 'src/oberon/window.cpp', 'src/oberon/events.cpp')
  ]
  oberon_srcs += [
    files('src/oberon/detail/io_subsystem.cpp', 'src/oberon/detail/graphics_subsystem.cpp',
          'src/oberon/detail/vulkan.cpp', 'src/oberon/detail/window_impl.cpp')
  ]
endif

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
