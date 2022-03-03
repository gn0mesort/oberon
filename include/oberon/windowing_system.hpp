#ifndef OBERON_WINDOWING_SYSTEM_HPP
#define OBERON_WINDOWING_SYSTEM_HPP

#include <concepts>

#include "macros.hpp"
#include "imaging_system.hpp"
#include "window.hpp"

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  template <typename Type>
  concept windowing_system = requires {
    requires imaging_system<Type>;
    requires window<typename Type::window_type>;
  };

}
}

#endif
