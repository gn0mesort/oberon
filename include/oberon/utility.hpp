#ifndef OBERON_UTILITY_HPP
#define OBERON_UTILITY_HPP

#include <cmath>

#include <concepts>


namespace oberon {

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
