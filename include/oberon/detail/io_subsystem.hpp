#ifndef OBERON_DETAIL_IO_SUBSYSTEM_HPP
#define OBERON_DETAIL_IO_SUBSYSTEM_HPP

#include <string_view>
#include <string>

#include "../basics.hpp"

#include "x11.hpp"

namespace oberon::detail {

  class io_subsystem final {
  private:
    ptr<xcb_connection_t> m_connection{ };
    ptr<xcb_screen_t> m_screen{ };
    std::array<xcb_atom_t, static_cast<usize>(X_ATOM_MAX)> m_atoms{ };

    xcb_intern_atom_cookie_t x_intern_atom(const std::string_view name);
    xcb_intern_atom_cookie_t x_intern_existing_atom(const std::string_view name);
    xcb_atom_t x_intern_atom_reply(const xcb_intern_atom_cookie_t request);

    void open_x_connection(const cstring display);
    void close_x_connection() noexcept;

    io_subsystem(const cstring display);
  public:
    static constexpr usize x_event_size{ 32 };

    io_subsystem();
    io_subsystem(const std::string_view display);

    ~io_subsystem() noexcept;

    ptr<xcb_connection_t> x_connection();
    ptr<xcb_screen_t> x_screen();
    xcb_atom_t x_atom(const enum x_atom atom);
    void x_send_event(const xcb_window_t destination, const xcb_generic_event_t& event, const u32 event_mask,
                      const bool propagate);

    std::string hostname() const;
    u32 process_id() const;
  };

  OBERON_STATIC_EXCEPTION_TYPE(get_hostname_failed, "Failed to get system hostname.", 1);

}

#endif
