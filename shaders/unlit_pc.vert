#version 450 core

layout (location = 0) in vec4 i_position;
layout (location = 1) in vec4 i_color;

layout (location = 0) out vec4 o_color;

void main() {
  gl_Position = i_position;
  o_color = i_color;
}
