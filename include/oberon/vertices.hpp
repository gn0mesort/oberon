#ifndef OBERON_VERTICES_HPP
#define OBERON_VERTICES_HPP

#include <glm/glm.hpp>

namespace oberon {

  using namespace glm;

  enum class vertex_type {
    position_only = 0,
    position_color = 1,
    position_normal_texture_coordinates = 2
  };

  struct vertex_p final {
    vec4 position{ };
  };

  struct vertex_pc final {
    vec4 position{ };
    vec4 color{ };
  };

  struct vertex_pnt final {
    vec4 position{ };
    vec4 normal{ };
    vec2 texture_coordinates{ };
  };

}

#endif
