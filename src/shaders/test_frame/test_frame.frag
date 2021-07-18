#version 450 core

layout (location = 0) in vec4 i_color;

layout (location = 0) out vec4 final_color;

void main() {
  final_color = i_color;
}
