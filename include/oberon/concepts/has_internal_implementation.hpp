/**
 * @file has_internal_implementation.hpp
 * @brief A concept that defines objects which have internal APIs provided by an implementation pointer.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_CONCEPTS_HAS_INTERNAL_IMPLEMENTATION_HPP
#define OBERON_CONCEPTS_HAS_INTERNAL_IMPLEMENTATION_HPP

#include "../types.hpp"

namespace oberon::concepts {

  /**
   * @brief A concept defining an object that provides an internal implementation API.
   * @tparam Type The type that must satisfy this concept.
   */
  template <typename Type>
  concept has_internal_implementation = requires (Type t) {
    requires std::destructible<Type>;
    typename Type::implementation_type;
    { t.implementation() } -> std::same_as<typename Type::implementation_type&>;
  };

}

#endif
