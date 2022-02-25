#ifndef OBERON_BOUNDS_HPP
#define OBERON_BOUNDS_HPP

#include "macros.hpp"

#include "types.hpp"

namespace oberon {
OBERON_INLINE_V_0_0 namespace v0_0 {

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
    isize x{ };
    isize y{ };
  };

  struct offset_3d final {
    isize x{ };
    isize y{ };
    isize z{ };
  };

  struct bounds_2d final {
    offset_2d position{ };
    extent_2d size{ };
  };

  struct bounds_3d final {
    offset_3d position{ };
    extent_3d size{ };
  };

}
}

#endif
