#include "oberon/events.hpp"

#include <cstdlib>

#include "oberon/debug.hpp"
#include "oberon/application.hpp"

#include "oberon/detail/x11.hpp"
#include "oberon/detail/io_subsystem.hpp"

namespace oberon::detail {

  void default_window_message_handler(const u32, const ptr<void>) { }

}

namespace oberon {

  bool event_dispatcher::process_event(const ptr<void> ev) {
    if (!ev)
    {
      return false;
    }
    const auto generic = reinterpret_cast<ptr<xcb_generic_event_t>>(ev);
    switch (generic->response_type & 0x7f)
    {
    case XCB_CLIENT_MESSAGE:
      {
        const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(ev);
        m_window_message_handler(client_message->window, client_message);
      }
      break;
    default:
      break;
    }
    std::free(ev);
    return true;
  }

  event_dispatcher::event_dispatcher(context& ctx) {
    m_io = &ctx.io();
    OBERON_POSTCONDITION(m_io);
  }

  void event_dispatcher::set_window_message_handler(const window_message_handler& handler) {
    m_window_message_handler = handler;
  }

  bool event_dispatcher::poll_for_event() {
    auto ev = xcb_poll_for_event(m_io->x_connection());
    return process_event(ev);
  }

  void event_dispatcher::wait_for_event() {
    auto ev = xcb_wait_for_event(m_io->x_connection());
    process_event(ev);
  }

}
