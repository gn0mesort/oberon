#ifndef OBERON_SHADER_UNIFORMS_GLSL
#define OBERON_SHADER_UNIFORMS_GLSL

struct Mesh {
  mat4 model;
};

struct Camera {
  mat4 view;
  mat4 projection;
};

layout (push_constant, std430) uniform pc {
  layout (offset = 0) Mesh mesh;
  layout (offset = 64) Camera camera;
};

//layout (set = 0, binding = 0) uniform Camera {
//  mat4 view;
//  mat4 projection;
//} camera;
//
//layout (set = 1, binding = 0) uniform Mesh {
//  mat4 model;
//} mesh;
#endif
