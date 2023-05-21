/**
 * @file vertices.hpp
 * @brief Vertex data structures.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_VERTICES_HPP
#define OBERON_VERTICES_HPP

#include "glm.hpp"

#include "concepts/vertex.hpp"

namespace oberon {

  /**
   * @enum vertex_type
   * @brief An enumeration of possible vertex types.
   */
  enum class vertex_type {
    position_color,
    position_normal_texture_coordinates
  };

  /**
   * @brief The default vertex type for the library.
   */
  constexpr const vertex_type default_vertex_type{ vertex_type::position_color };

  /**
   * @class vertex_pc
   * @brief A vertex type with position and color data.
   */
  struct vertex_pc final {
    static constexpr vertex_type type() { return vertex_type::position_color; }
    glm::vec4 position{ };
    glm::vec4 color{ };
  };

  OBERON_ENFORCE_CONCEPT(concepts::vertex, vertex_pc);

  /**
   * @class vertex_pnt
   * @brief A vertex type with position, normal, and texture coordinate data.
   */
  struct vertex_pnt final {
    static constexpr vertex_type type() { return vertex_type::position_normal_texture_coordinates; }
    glm::vec3 position{ };
    glm::vec3 normal{ };
    glm::vec2 texture_coordinates{ };
  };

  OBERON_ENFORCE_CONCEPT(concepts::vertex, vertex_pnt);

}

#endif
