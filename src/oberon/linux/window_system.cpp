#include "oberon/linux/detail/window_system_impl.hpp"

#include <cstdlib>

#include "oberon/debug.hpp"

namespace oberon::linux::detail {

  std::pair<ptr<xcb_connection_t>, int> x11_create_connection(const cstring display) {
    auto screen_pref = int{ 0 };
    auto connection = xcb_connect(display, &screen_pref);
    OBERON_INVARIANT(!xcb_connection_has_error(connection), x_connection_failed_error{ });
    return { connection, screen_pref };
  }

  ptr<xcb_screen_t> x11_select_screen(const ptr<xcb_connection_t> connection, int screen_pref) {
    auto setup = xcb_get_setup(connection);
    auto screen = ptr<xcb_screen_t>{ };
    for (auto roots = xcb_setup_roots_iterator(setup); roots.rem; xcb_screen_next(&roots))
    {
      if (screen_pref-- == 0)
      {
        screen = roots.data;
      }
    }
    OBERON_INVARIANT(screen != nullptr, x_no_screen_error{ });
    return screen;
  }

  void x11_init_ewmh(const ptr<xcb_connection_t> connection, xcb_ewmh_connection_t &ewmh) {
    OBERON_PRECONDITION(connection != nullptr);
    auto atom_cookies = xcb_ewmh_init_atoms(connection, &ewmh);
    OBERON_INVARIANT(xcb_ewmh_init_atoms_replies(&ewmh, atom_cookies, nullptr) == 1,
                     x_ewmh_init_failed_error{ });
  }

  xcb_atom_t x11_atom(const ptr<xcb_connection_t> connection, const std::string_view name) {
    const auto cookie = xcb_intern_atom(connection, 0, std::size(name), std::data(name));
    const auto reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    const auto res = reply ? reply->atom : XCB_NONE;
    std::free(reply);
    return res;
  }

  void x11_destroy_connection(const ptr<xcb_connection_t> connection) noexcept {
    if (connection)
    {
      xcb_flush(connection);
      xcb_disconnect(connection);
    }
  }
}

namespace oberon::linux {

  window_system::window_system(const ptr<detail::window_system_impl> impl) {
    OBERON_PRECONDITION(impl != nullptr);
    OBERON_PRECONDITION(impl->connection != nullptr);
    OBERON_PRECONDITION(impl->ewmh.connection == impl->connection);
    OBERON_PRECONDITION(impl->screen != nullptr);
    m_impl = impl;
    OBERON_POSTCONDITION(m_impl != nullptr);
  }

}
