#ifndef OBERON_SHADER_UNIFORMS_GLSL
#define OBERON_SHADER_UNIFORMS_GLSL
layout (set = 0, binding = 0) uniform UniformBlock {
  mat4 model;
  mat4 view;
  mat4 projection;
} ub;
#endif
