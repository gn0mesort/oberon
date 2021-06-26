#ifndef OBERON_EVENTS_HPP
#define OBERON_EVENTS_HPP

#include <array>

#include "types.hpp"
#include "bounds.hpp"

namespace oberon {
namespace events {

  struct empty_data final { };

  struct window_expose_data final { };

  struct window_message_data final {
    std::array<u8, 20> content{ };
  };

  struct window_resize_data final {
    extent_2d size{ };
  };
}

  enum class event_type {
    empty,
    window_expose,
    window_message,
    window_resize
  };

  struct event final {
    imax window_id{ };
    event_type type{ };
    union {
      events::empty_data empty;
      events::window_expose_data window_expose;
      events::window_message_data window_message;
      events::window_resize_data window_resize;
    } data;
  };

}

#endif
