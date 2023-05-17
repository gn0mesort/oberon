#ifndef OBERON_CONCEPTS_VERTEX_HPP
#define OBERON_CONCEPTS_VERTEX_HPP

#include "../types.hpp"

namespace oberon {

  enum class vertex_type;

}

namespace oberon::concepts {

  template <typename Type>
  concept vertex = requires (Type t) {
    requires std::destructible<Type>;
    { Type::type() } -> std::same_as<vertex_type>;
  };

}

#endif
