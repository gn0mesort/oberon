#ifndef OBERON_CONCEPTS_HAS_INTERNAL_IMPLEMENTATION_HPP
#define OBERON_CONCEPTS_HAS_INTERNAL_IMPLEMENTATION_HPP

#include "../types.hpp"

namespace oberon {

  template <typename Type>
  concept has_internal_implementation = requires (Type t) {
    requires std::destructible<Type>;
    typename Type::implementation_type;
    { t.implementation() } -> std::same_as<typename Type::implementation_type&>;
  };

}

#endif
