#include "oberon/events.hpp"

#include <cstdlib>

#include "oberon/application.hpp"

#include "oberon/detail/x11.hpp"
#include "oberon/detail/io_subsystem.hpp"

namespace oberon {

  event_variant process_event(const ptr<xcb_generic_event_t> ev) {
    if (!ev)
    {
      return empty_event{ };
    }
    switch (ev->response_type & 0x7f)
    {
    case XCB_CLIENT_MESSAGE:
      {
        const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(ev);
        return window_message_event{ .window_id = client_message->window,
                                     .data = reinterpret_cast<ptr<void>>(client_message) };
      }
      break;
    default:
      std::free(ev);
      return empty_event{ };
    }
  }

  event_variant poll_for_event(context& ctx) {
    auto conn = ctx.io().x_connection();
    auto ev = xcb_poll_for_event(conn);
    return process_event(ev);
  }

  event_variant wait_for_event(context& ctx) {
    auto conn = ctx.io().x_connection();
    auto ev = xcb_wait_for_event(conn);
    return process_event(ev);
  }

}
