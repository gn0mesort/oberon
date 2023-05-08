#ifndef OBERON_RECTS_HPP
#define OBERON_RECTS_HPP

#include "types.hpp"

namespace oberon {

  struct offset_2d final {
    i16 x{ };
    i16 y{ };
  };

  struct offset_3d final {
    i16 x{ };
    i16 y{ };
    i16 z{ };
  };

  struct extent_2d final {
    u16 width{ };
    u16 height{ };
  };

  struct extent_3d final {
    u16 width{ };
    u16 height{ };
    u16 depth{ };
  };

  struct rect_2d final {
    offset_2d offset{ };
    extent_2d extent{ };
  };

  struct rect_3d final {
    offset_3d offset{ };
    extent_3d extent{ };
  };

}

#endif
