/**
 * @file events.hpp
 * @brief Input system events.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_EVENTS_HPP
#define OBERON_EVENTS_HPP

#include "types.hpp"
#include "rects.hpp"

namespace oberon {

  /**
   * @brief The size in bytes of platform events.
   * @details By their nature, platform events contain data that is only meaningful on specific platforms. This value
   *          should indicate the largest size required by any supported platform.
   */
  constexpr usize OBERON_PLATFORM_EVENT_SIZE{ 36 };

  /**
   * @enum event_type
   * @brief An enumeration which indicates the type of data contained in an `event`
   */
  enum class event_type {
    none,
    platform,
    window_close,
    geometry_reconfigure,
    key_press,
    key_release,
    button_press,
    button_release,
    motion
  };

  /**
   * @class platform_event_data
   * @brief An event data structure containing any platform specific event received.
   */
  struct platform_event_data final {
    char pad[OBERON_PLATFORM_EVENT_SIZE];
  };

  /**
   * @class geometry_reconfigure_event_data
   * @brief An event data structure containing information about the geometry of a window.
   */
  struct geometry_reconfigure_event_data final {
    rect_2d geometry{ };
  };

  /**
   * @class key_press_event_data
   * @brief An event data structure containing information about a key press.
   */
  struct key_press_event_data final {
    u32 key{ };
    bool echoing{ };
  };

  /**
   * @class key_release_event_data
   * @brief An event data structure containing information about a key release.
   */
  struct key_release_event_data final {
    u32 key{ };
  };

  /**
   * @class button_press_event_data
   * @brief An event data structure containing information about a button press.
   */
  struct button_press_event_data final {
    u32 button{ };
  };

  /**
   * @class button_release_event_data
   * @brief An event data structure containing information about a button release.
   */
  using button_release_event_data = button_press_event_data;

  /**
   * @class motion_event_data
   * @brief An event data structure containing information about pointer motion.
   */
  struct motion_event_data final {
    offset_2d screen_offset{ };
    offset_2d window_offset{ };
  };

  /**
   * @class event
   * @brief A structure representing an input event.
   */
  struct event {
    /**
     * @brief The type of the event that was received.
     */
    event_type type{ event_type::none };
    /**
     * @brief The event data.
     * @details Only the union member indicated by `event::type` is considered valid. Accessing another member is
     *          undefined. Some event types have no corresponding data.
     */
    union {
      platform_event_data platform;
      geometry_reconfigure_event_data geometry_reconfigure;
      key_press_event_data key_press;
      key_release_event_data key_release;
      button_press_event_data button_press;
      button_release_event_data button_release;
      motion_event_data motion;
    } data;

    /**
     * @brief Convert an event into a boolean value.
     * @return False if `event::type` is `event_type::none`. True in all other cases.
     */
    inline operator bool() const {
      return type != event_type::none;
    }
  };

}

#endif
