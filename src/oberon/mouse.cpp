/**
 * @file mouse.cpp
 * @brief Mouse utilty routine implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/mouse.hpp"

namespace oberon {

  std::string to_string(const mouse_button mb) {
#define OBERON_MOUSE_BUTTON(name) case mouse_button::name: return #name;
#define OBERON_MOUSE_BUTTON_ALIAS(alias, name)
    switch (mb)
    {
    OBERON_MOUSE_BUTTONS
    default:
      return "none";
    }
#undef OBERON_MOUSE_BUTTON
#undef OBERON_MOUSE_BUTTON_ALIAS
  }

}
