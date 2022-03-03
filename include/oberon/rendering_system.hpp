#ifndef OBERON_RENDERING_SYSTEM_HPP
#define OBERON_RENDERING_SYSTEM_HPP

#include <concepts>

#include "macros.hpp"
#include "render_target.hpp"

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  template <typename Type>
  concept rendering_system = requires {
    requires std::destructible<Type>;
    requires render_target<typename Type::target_type>;
  };

}
}

#endif
