#ifndef OBERON_EVENTS_HPP
#define OBERON_EVENTS_HPP

#include "types.hpp"
#include "rects.hpp"

namespace oberon {

  constexpr usize OBERON_PLATFORM_EVENT_SIZE{ 36 };

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

  struct platform_event_data final {
    char pad[OBERON_PLATFORM_EVENT_SIZE];
  };

  struct geometry_reconfigure_event_data final {
    rect_2d geometry{ };
  };

  struct key_press_event_data final {
    u32 key{ };
    bool echoing{ };
  };

  struct key_release_event_data final {
    u32 key{ };
  };

  struct button_press_event_data final {
    u32 button{ };
  };

  using button_release_event_data = button_press_event_data;

  struct motion_event_data final {
    offset_2d screen_offset{ };
    offset_2d window_offset{ };
  };

  struct event {
    event_type type{ event_type::none };
    union {
      platform_event_data platform;
      geometry_reconfigure_event_data geometry_reconfigure;
      key_press_event_data key_press;
      key_release_event_data key_release;
      button_press_event_data button_press;
      button_release_event_data button_release;
      motion_event_data motion;
    } data;

    inline operator bool() const {
      return type != event_type::none;
    }
  };

}

#endif
