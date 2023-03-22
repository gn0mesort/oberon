##
# @file meson.build
# @brief Shader Meson build script.
# @author Alexander Rothman <gnomesort@megate.ch>
# @date 2023
# @copyright AGPL-3.0+
#
test_image_vert = custom_target('test_image_vert', command: glsl, input: files('test_image.vert'),
                                output: 'test_image.vert.spv', build_by_default: true)
test_image_frag = custom_target('test_image_frag', command: glsl, input: files('test_image.frag'),
                                output: 'test_image.frag.spv', build_by_default: true)
unlit_pc_vert = custom_target('unlit_pc_vert', command: glsl, input: files('unlit_pc.vert'),
                              output: 'unlit_pc.vert.spv', build_by_default: true)
unlit_pc_frag = custom_target('unlit_pc_frag', command: glsl, input: files('unlit_pc.frag'),
                              output: 'unlit_pc.frag.spv', build_by_default: true)
