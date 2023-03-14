/**
 * @file test_image.frag
 * @brief Test Image fragment shader.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#version 450 core

layout (location = 0) in vec3 i_color;

layout (location = 0) out vec4 final_color;

void main() {
  final_color = vec4(i_color, 1.0);
}
