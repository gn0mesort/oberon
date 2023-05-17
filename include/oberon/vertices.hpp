#ifndef OBERON_VERTICES_HPP
#define OBERON_VERTICES_HPP

#include "glm.hpp"

#include "concepts/vertex.hpp"

namespace oberon {

  enum class vertex_type {
    position_color,
    position_normal_texture_coordinates
  };

  constexpr const vertex_type default_vertex_type{ vertex_type::position_color };

  struct vertex_pc final {
    static constexpr vertex_type type() { return vertex_type::position_color; }
    glm::vec4 position{ };
    glm::vec4 color{ };
  };

  OBERON_ENFORCE_CONCEPT(concepts::vertex, vertex_pc);

  struct vertex_pnt final {
    static constexpr vertex_type type() { return vertex_type::position_normal_texture_coordinates; }
    glm::vec3 position{ };
    glm::vec3 normal{ };
    glm::vec2 texture_coordinates{ };
  };

  OBERON_ENFORCE_CONCEPT(concepts::vertex, vertex_pnt);

}

#endif
