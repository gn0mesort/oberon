/**
 * @file test_image.vert
 * @brief Test Image vertex shader.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#version 450 core

const vec2 TRIANGLE[3] = vec2[](vec2(0.5, 0.5), vec2(0.0, -0.5), vec2(-0.5, 0.5));
const vec3 COLORS[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

layout (location = 0) out vec3 o_color;

void main() {
  gl_Position = vec4(TRIANGLE[gl_VertexIndex], 0.0, 1.0);
  o_color = COLORS[gl_VertexIndex];
}
