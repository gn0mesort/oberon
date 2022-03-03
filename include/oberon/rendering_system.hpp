#ifndef OBERON_RENDERING_SYSTEM_HPP
#define OBERON_RENDERING_SYSTEM_HPP

#include <concepts>

#include "render_target.hpp"

namespace oberon {
inline namespace rendering_system_v01 {

  template <typename Type>
  concept rendering_system = requires {
    requires std::destructible<Type>;
    requires render_target<typename Type::target_type>;
  };

}
}

#endif
