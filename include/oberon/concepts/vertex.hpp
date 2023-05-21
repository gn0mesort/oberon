/**
 * @file vertex.hpp
 * @brief A concept that defines vertex objects.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_CONCEPTS_VERTEX_HPP
#define OBERON_CONCEPTS_VERTEX_HPP

#include "../types.hpp"

namespace oberon {

  enum class vertex_type;

}

namespace oberon::concepts {

  /**
   * @brief A concept defining an object that implements the basic functionality of a renderable vertex.
   * @tparam Type The type that must satisfy this concept.
   */
  template <typename Type>
  concept vertex = requires (Type t) {
    requires std::destructible<Type>;
    { Type::type() } -> std::same_as<vertex_type>;
  };

}

#endif
