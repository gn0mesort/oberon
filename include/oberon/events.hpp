#ifndef OBERON_EVENTS_HPP
#define OBERON_EVENTS_HPP

#include <variant>

#include "basics.hpp"

namespace oberon {

  class context;

  struct empty_event final { };

  struct window_message_event final {
    u32 window_id{ };
    ptr<void> data{ };
  };

  using event_variant = std::variant<empty_event, window_message_event>;

  event_variant poll_for_event(context& ctx);
  event_variant wait_for_event(context& ctx);


}

#endif
