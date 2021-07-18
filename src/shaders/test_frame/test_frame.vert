#version 450 core

const vec3 positions[3] = vec3[](
  vec3(-0.5, 0.5, 0.0),
  vec3(0.5, 0.5, 0.0),
  vec3(0.0, -0.5, 0.0)
);

const vec3 colors[3] = vec3[](
  vec3(1.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0)
);

layout (location = 0) out vec4 o_color;

void main() {
  gl_Position = vec4(positions[gl_VertexIndex], 1.0);
  o_color = vec4(colors[gl_VertexIndex], 1.0);
}
