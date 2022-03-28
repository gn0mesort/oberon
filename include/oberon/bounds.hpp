#ifndef OBERON_BOUNDS_HPP
#define OBERON_BOUNDS_HPP

#include "types.hpp"

namespace oberon {

  struct extent_2d final {
    u16 width{ };
    u16 height{ };
  };

  struct extent_3d final {
    u16 width{ };
    u16 height{ };
    u16 depth{ };
  };

  struct offset_2d final {
    i16 x{ };
    i16 y{ };
  };

  struct offset_3d final {
    i16 x{ };
    i16 y{ };
    i16 z{ };
  };

  struct bounding_rect final {
    offset_2d position{ };
    extent_2d size{ };
  };

  struct bounding_box final {
    offset_3d position{ };
    extent_3d size{ };
  };

}

#endif
