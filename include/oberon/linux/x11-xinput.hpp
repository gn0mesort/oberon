/**
 * @file x11-xinput.hpp
 * @brief X11 XInput 2 related types and definitions.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_X11_XINPUT_HPP
#define OBERON_LINUX_X11_XINPUT_HPP

#include <xcb/xinput.h>

#include "../types.hpp"

namespace oberon::linux {

  /**
   * @brief A structure representing a list of XInput 2 event mask entries.
   * @details This is not defined by libxcb_xinput.
   */
  struct x_input_event_mask_list {
    /**
     * @brief The head of the event mask list.
     */
    xcb_input_event_mask_t head{ };

    /**
     * @brief The event mask.
     */
    xcb_input_xi_event_mask_t mask{ };
  };

  /**
   * @brief A mask representing the integer component of an XInput 2 FP1616 value.
   */
  constexpr const u32 OBERON_LINUX_X_XI_FP1616_INT_MASK{ 0xffff0000 };

  /**
   * @brief A mask repreenting the fractional component of an XInput 2 FP1616 value.
   */
  constexpr const u32 OBERON_LINUX_X_XI_FP1616_FRAC_MASK{ 0x0000ffff };

  /**
   * @brief The maximum number of XInput 2 events.
   */
  constexpr const usize OBERON_LINUX_X_XI_EVENT_MAX{ 33 };

}

#endif
