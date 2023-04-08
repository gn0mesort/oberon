#ifndef OBERON_INTERNAL_IMPLEMENTATION_HPP
#define OBERON_INTERNAL_IMPLEMENTATION_HPP

#include <concepts>

#include "../types.hpp"

#define OBERON_INTERNAL_IMPLEMENTATION_IDS \
  OBERON_INTERNAL_IMPLEMENTATION_ID(vulkan, 1) \
  OBERON_INTERNAL_IMPLEMENTATION_ID(xcb_vulkan, 2)

namespace oberon::internal {

#define OBERON_INTERNAL_IMPLEMENTATION_ID(name, id) name = (id),
  enum class implementation_id {
    invalid = 0,
    OBERON_INTERNAL_IMPLEMENTATION_IDS
  };
#undef OBERON_INTERNAL_IMPLEMENTATION_ID

  template <typename Type>
  concept implementation = requires (Type t) {
    std::destructible<Type>;
    { t.id() } -> std::same_as<implementation_id>;
    { t.traits() } -> std::same_as<bitmask>;
  };

}

namespace oberon::internal::implementation_trait_bits {

  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(has_wsi, 1);

}

#endif
