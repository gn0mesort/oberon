#ifndef OBERON_RENDER_TARGET_HPP
#define OBERON_RENDER_TARGET_HPP

#include <concepts>

#include "macros.hpp"

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  template <typename Type>
  concept render_target = requires {
    requires std::destructible<Type>;
  };

}
}

#endif
