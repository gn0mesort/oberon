#ifndef OBERON_IMAGING_SYSTEM_HPP
#define OBERON_IMAGING_SYSTEM_HPP

#include <concepts>

#include "macros.hpp"
#include "rendering_system.hpp"
#include "image.hpp"

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  template <typename Type>
  concept imaging_system = requires {
    requires rendering_system<Type>;
    requires image<typename Type::image_type>;
  };

}
}

#endif
