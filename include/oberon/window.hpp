#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include "image.hpp"

namespace oberon {
inline namespace window_v01 {

  template <typename Type>
  concept window = requires {
    requires image<Type>;
  };

}
}

#endif
