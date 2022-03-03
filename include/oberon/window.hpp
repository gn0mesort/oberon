#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <concepts>

#include "macros.hpp"
#include "image.hpp"

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  template <typename Type>
  concept window = requires {
    requires image<Type>;
  };

}
}

#endif
