#ifndef OBERON_IMAGE_HPP
#define OBERON_IMAGE_HPP

#include "render_target.hpp"

namespace oberon {
inline namespace image_v01 {

  template <typename Type>
  concept image = requires {
    requires render_target<Type>;
  };

}
}

#endif
