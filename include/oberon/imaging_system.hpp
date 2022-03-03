#ifndef OBERON_IMAGING_SYSTEM_HPP
#define OBERON_IMAGING_SYSTEM_HPP

#include "rendering_system.hpp"
#include "image.hpp"

namespace oberon {
inline namespace imaging_system_v01 {

  template <typename Type>
  concept imaging_system = requires {
    requires rendering_system<Type>;
    requires image<typename Type::image_type>;
  };

}
}

#endif
