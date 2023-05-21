/**
 * @file utility.hpp
 * @brief Utility functions.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_UTILITY_HPP
#define OBERON_UTILITY_HPP

#include <cmath>

#include <concepts>


namespace oberon {

  /**
   * @brief Convert a linear color value to sRGB.
   * @tparam Float A floating-point type.
   * @param color A color value in linear space.
   * @return The color value converted into sRGB.
   */
  template <std::floating_point Float>
  constexpr Float to_srgb_color(Float&& color) {
    if (color <= 0.0031308)
    {
      return 12.92 * color;
    }
    else
    {
      return (1.055 * std::pow(color, 1.0 / 2.4)) - 0.055;
    }
  }

  /**
   * @brief Convert an sRGB color to linear space.
   * @tparam Float A floating-point type.
   * @param color An sRGB color value.
   * @return The color value converted into linear space.
   */
  template <std::floating_point Float>
  constexpr Float to_linear_color(Float&& color) {
    if (color <= 0.04045)
    {
      return color / 12.92;
    }
    else
    {
      return std::pow((color + 0.055) / 1.055, 2.4);
    }
  }

}

#endif
