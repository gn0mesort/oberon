#ifndef OBERON_SHADER_UNIFORMS_GLSL
#define OBERON_SHADER_UNIFORMS_GLSL
layout (set = 0, binding = 0) uniform Camera {
  mat4 view;
  mat4 projection;
} camera;

layout (set = 1, binding = 0) uniform Mesh {
  mat4 model;
} mesh;
#endif
