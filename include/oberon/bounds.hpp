#ifndef OBERON_BOUNDS_HPP
#define OBERON_BOUNDS_HPP

#include "types.hpp"

namespace oberon {

  struct extent_2d final {
    usize width{ };
    usize height{ };
  };

  struct extent_3d final {
    usize width{ };
    usize height{ };
    usize depth{ };
  };

  struct offset_2d final {
    usize x{ };
    usize y{ };
  };

  struct offset_3d final {
    usize x{ };
    usize y{ };
    usize z{ };
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
