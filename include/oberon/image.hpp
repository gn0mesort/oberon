#ifndef OBERON_IMAGE_HPP
#define OBERON_IMAGE_HPP

#include <concepts>

#include "macros.hpp"
#include "render_target.hpp"

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  template <typename Type>
  concept image = requires {
    requires render_target<Type>;
  };

}
}

#endif
