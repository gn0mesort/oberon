#ifndef OBERON_RENDER_TARGET_HPP
#define OBERON_RENDER_TARGET_HPP

#include <concepts>

namespace oberon {
inline namespace render_target_v01 {

  template <typename Type>
  concept render_target = requires {
    requires std::destructible<Type>;
  };

}
}

#endif
