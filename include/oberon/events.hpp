#ifndef OBERON_EVENTS_HPP
#define OBERON_EVENTS_HPP

#include <array>

#include "types.hpp"
#include "memory.hpp"
#include "bounds.hpp"

namespace oberon {

  class window;

namespace events {

  struct empty_data final { };

  struct window_configure_data final {
    bounding_rect bounds{ };
    bool was_resized{ false };
    bool was_repositioned{ false };
  };

  struct window_hide_data final {
  };

}

  enum class event_type {
    empty,
    window_hide,
    window_configure
  };

  struct event final {
    ptr<window> window_ptr{ };
    event_type type{ };
    union {
      events::empty_data empty;
      events::window_hide_data window_hide;
      events::window_configure_data window_configure;
    } data;
  };

}

#endif
