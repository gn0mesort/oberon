#include "oberon/detail/io_subsystem.hpp"

#include <cstdlib>

#include <sys/types.h>
#include <sys/utsname.h>

#include <unistd.h>

#include "oberon/debug.hpp"

namespace oberon::detail {

// Generates array of X11 atom names at compile time
#define OBERON_X_ATOM_NAME(name, str) (str),
  consteval std::array<cstring, static_cast<usize>(X_ATOM_MAX)> x_atom_names() {
    return { OBERON_X_ATOMS };
  }
#undef OBERON_X_ATOM_NAME

  // Pre: connection
  xcb_intern_atom_cookie_t io_subsystem::x_intern_atom(const std::string_view name) {
    OBERON_PRECONDITION(m_connection);
    return xcb_intern_atom(m_connection, false, std::size(name), std::data(name));
  }

  // Pre: connection
  xcb_atom_t io_subsystem::x_intern_atom_reply(const xcb_intern_atom_cookie_t request) {
    OBERON_PRECONDITION(m_connection);
    auto rep = ptr<xcb_intern_atom_reply_t>{ };
    OBERON_X_SUCCEEDS(rep, xcb_intern_atom_reply(m_connection, request, err));
    auto result = rep->atom;
    std::free(rep);
    return result;
  }

  // Pre: No connection, no screen, atoms undefined
  // Post: Connection, screen, atoms defined
  void io_subsystem::open_x_connection(const cstring display) {
    OBERON_PRECONDITION(!m_connection);
    OBERON_PRECONDITION(!m_screen);
    auto screen_pref = int{ 0 };
    m_connection = xcb_connect(display, &screen_pref);
    OBERON_INVARIANT(!xcb_connection_has_error(m_connection));
    xcb_set_close_down_mode(m_connection, XCB_CLOSE_DOWN_DESTROY_ALL);
    // Fire off X intern atom requests
    auto intern_atom_reqs = std::array<xcb_intern_atom_cookie_t, static_cast<usize>(X_ATOM_MAX)>{ };
    {
      constexpr auto atom_names = x_atom_names();
      auto cur = std::begin(intern_atom_reqs);
      for (const auto name : atom_names)
      {
        *(cur++) = x_intern_atom(name);
      }
    }
    {
      auto setup = xcb_get_setup(m_connection);
      for (auto roots = xcb_setup_roots_iterator(setup); roots.rem; xcb_screen_next(&roots))
      {
        if (!(screen_pref--))
        {
          m_screen = roots.data;
        }
      }
    }
    // Retrieve X atoms
    {
      auto cur = std::begin(m_atoms);
      for (const auto req : intern_atom_reqs)
      {
        *(cur++) = x_intern_atom_reply(req);
      }
    }
    OBERON_POSTCONDITION(m_connection);
    OBERON_POSTCONDITION(m_screen);
  }

  // Pre: connection
  // Post: No connection, no screen, atoms undefined
  void io_subsystem::close_x_connection() noexcept {
    // xcb_connect() *always* returns a valid connection even if it's immediately in a bad state.
    // Therefore it's safe to assume the precondition can be expected to be true.
    OBERON_PRECONDITION(m_connection);
    xcb_disconnect(m_connection);
    m_connection = nullptr;
    m_screen = nullptr;
    OBERON_POSTCONDITION(!m_screen);
    OBERON_POSTCONDITION(!m_connection);
  }

  io_subsystem::io_subsystem(const cstring display) {
    open_x_connection(display);
  }

  io_subsystem::io_subsystem() : io_subsystem{ nullptr } { }

  io_subsystem::io_subsystem(const std::string_view display) : io_subsystem{ std::data(display) } { }

  io_subsystem::~io_subsystem() noexcept {
    close_x_connection();
  }

  ptr<xcb_connection_t> io_subsystem::x_connection() {
    return m_connection;
  }

  ptr<xcb_screen_t> io_subsystem::x_screen() {
    return m_screen;
  }

  xcb_atom_t io_subsystem::x_atom(const enum x_atom atom) {
    return m_atoms[static_cast<usize>(atom)];
  }

  void io_subsystem::x_send_event(const xcb_window_t destination, const xcb_generic_event_t& event,
                                  const u32 event_mask, const bool propagate) {
    static_assert(sizeof(xcb_generic_event_t) >= 32, "libxcb needs 32-byte event buffers.");
    xcb_send_event(m_connection, propagate, destination, event_mask, reinterpret_cast<readonly_ptr<char>>(&event));
  }

  std::string io_subsystem::hostname() const {
    auto uname_res = utsname{ };
    OBERON_INVARIANT(uname(&uname_res) != -1);
    return uname_res.nodename;
  }

  u32 io_subsystem::process_id() const {
    return getpid();
  }

}
