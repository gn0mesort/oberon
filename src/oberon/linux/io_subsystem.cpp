#include "oberon/linux/io_subsystem.hpp"

#include <cstdlib>

#include "oberon/debug.hpp"

namespace oberon::linux {

  void io_subsystem::open_x_connection(const cstring display) {
    OBERON_PRECONDITION(m_ewmh.connection == nullptr);
    OBERON_PRECONDITION(m_screen == nullptr);
    auto screen_pref = int{ 0 };
    auto connection = xcb_connect(display, &screen_pref);
    OBERON_INVARIANT(!xcb_connection_has_error(connection), x_connection_failed_error{ });
    auto cookies = xcb_ewmh_init_atoms(connection, &m_ewmh);
    {
      auto setup = xcb_get_setup(connection);
      for (auto roots = xcb_setup_roots_iterator(setup); roots.rem; xcb_screen_next(&roots))
      {
        if (!(screen_pref--))
        {
          m_screen = roots.data;
        }
      }
      OBERON_INVARIANT(m_screen != nullptr, x_no_screen_error{ });
    }
    OBERON_INVARIANT(xcb_ewmh_init_atoms_replies(&m_ewmh, cookies, nullptr) == 1, x_ewmh_init_failed_error{ });
    OBERON_POSTCONDITION(m_screen != nullptr);
    OBERON_POSTCONDITION(m_ewmh.connection != nullptr);
  }

  void io_subsystem::close_x_connection() noexcept {
    OBERON_PRECONDITION(m_ewmh.connection != nullptr);
    xcb_flush(m_ewmh.connection);
    xcb_disconnect(m_ewmh.connection);
    m_ewmh.connection = nullptr;
    m_screen = nullptr;
  }

  io_subsystem::io_subsystem() : io_subsystem(nullptr) { }

  io_subsystem::io_subsystem(const cstring display) {
    open_x_connection(display);
  }

  io_subsystem::~io_subsystem() noexcept {
    close_x_connection();
  }

  xcb_atom_t io_subsystem::x_atom(const std::string_view name) {
    const auto cookie = xcb_intern_atom(m_ewmh.connection, 0, std::size(name), std::data(name));
    const auto reply = xcb_intern_atom_reply(m_ewmh.connection, cookie, nullptr);
    const auto res = reply ? reply->atom : XCB_NONE;
    std::free(reply);
    return res;
  }

  ptr<xcb_connection_t> io_subsystem::x_connection() {
    return m_ewmh.connection;
  }

  ptr<xcb_screen_t> io_subsystem::x_screen() {
    return m_screen;
  }

}
