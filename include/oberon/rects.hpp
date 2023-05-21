/**
 * @file rects.hpp
 * @brief Different types of rectangular objects.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_RECTS_HPP
#define OBERON_RECTS_HPP

#include "types.hpp"

namespace oberon {

  /**
   * @class offset_2d
   * @brief A structure representing an offset into a 2D plane.
   */
  struct offset_2d final {
    i16 x{ };
    i16 y{ };
  };

  /**
   * @class offset_3d
   * @brief A structure representing an offset into a 3D volume.
   *
   */
  struct offset_3d final {
    i16 x{ };
    i16 y{ };
    i16 z{ };
  };


  /**
   * @class extent_2d
   * @brief A structure representing the extent of a 2D plane.
   */
  struct extent_2d final {
    u16 width{ };
    u16 height{ };
  };

  /**
   * @class extent_3d
   * @brief A structure representing the extent of a 3D volume.
   *
   */
  struct extent_3d final {
    u16 width{ };
    u16 height{ };
    u16 depth{ };
  };

  /**
   * @class rect_2d
   * @brief A structure representing a rectangle on a 2D plane.
   */
  struct rect_2d final {
    offset_2d offset{ };
    extent_2d extent{ };
  };

  /**
   * @class rect_3d
   * @brief A structure representing a rectangular prism in a 3D volume.
   */
  struct rect_3d final {
    offset_3d offset{ };
    extent_3d extent{ };
  };

}

#endif
