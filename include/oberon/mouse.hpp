/**
 * @file mouse.hpp
 * @brief Mouse related types and values.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_MOUSE_HPP
#define OBERON_MOUSE_HPP

#include <string>

#include "types.hpp"

/**
 * @def OBERON_MOUSE_BUTTONS
 * @brief A list of physical mouse buttons.
 */
#define OBERON_MOUSE_BUTTONS \
  OBERON_MOUSE_BUTTON(button_1) \
  OBERON_MOUSE_BUTTON(button_2) \
  OBERON_MOUSE_BUTTON(button_3) \
  OBERON_MOUSE_BUTTON(button_4) \
  OBERON_MOUSE_BUTTON(button_5) \
  OBERON_MOUSE_BUTTON(button_6) \
  OBERON_MOUSE_BUTTON(button_7) \
  OBERON_MOUSE_BUTTON(button_8) \
  OBERON_MOUSE_BUTTON(button_9) \
  OBERON_MOUSE_BUTTON(button_10) \
  OBERON_MOUSE_BUTTON(button_11) \
  OBERON_MOUSE_BUTTON(button_12) \
  OBERON_MOUSE_BUTTON(button_13) \
  OBERON_MOUSE_BUTTON(button_14) \
  OBERON_MOUSE_BUTTON(button_15) \
  OBERON_MOUSE_BUTTON(button_16) \
  OBERON_MOUSE_BUTTON(button_17) \
  OBERON_MOUSE_BUTTON(button_18) \
  OBERON_MOUSE_BUTTON(button_19) \
  OBERON_MOUSE_BUTTON(button_20) \
  OBERON_MOUSE_BUTTON_ALIAS(left, button_1) \
  OBERON_MOUSE_BUTTON_ALIAS(middle, button_2) \
  OBERON_MOUSE_BUTTON_ALIAS(right, button_3) \
  OBERON_MOUSE_BUTTON_ALIAS(scroll_up, button_4) \
  OBERON_MOUSE_BUTTON_ALIAS(scroll_down, button_5)

namespace oberon {

  /**
   * @brief A structure representing the (x, y) coordinates of the mouse pointer.
   */
  struct mouse_offset final {
    /**
     * @brief The X-axis offset of the mouse pointer.
     */
    i16 x{ };

    /**
     * @brief The Y-axis offset of the mouse pointer.
     */
    i16 y{ };
  };

/// @cond
#define OBERON_MOUSE_BUTTON(name) name,
#define OBERON_MOUSE_BUTTON_ALIAS(alias, name) alias = name,
/// @endcond

  /**
   * An enumeration of mouse buttons.
   */
  enum class mouse_button {
    none,
    OBERON_MOUSE_BUTTONS
  };

/// @cond
#undef OBERON_MOUSE_BUTTON_ALIAS
#undef OBERON_MOUSE_BUTTON
/// @endcond

  /**
   * @brief Get the string representing the name of the given button.
   * @details Aliased button names (e.g., "left") are not returned by this function.
   * @param mb The mouse_button to get the string representation of.
   * @return A string representing the given mouse_button.
   */
  std::string to_string(const mouse_button mb);

}

#endif
