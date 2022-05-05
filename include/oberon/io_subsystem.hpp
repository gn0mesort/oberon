#ifndef OBERON_IO_SUBSYSTEM_HPP
#define OBERON_IO_SUBSYSTEM_HPP

#include <string_view>

#include "x11.hpp"
#include "memory.hpp"

namespace oberon {

  class io_subsystem final {
  private:
    ptr<xcb_connection_t> m_connection{ };
    ptr<xcb_screen_t> m_screen{ };
    xcb_atom_t m_wm_name{ };
    xcb_atom_t m_wm_protocols{ };
    xcb_atom_t m_wm_delete_window{ };
    xcb_atom_t m_wm_normal_hints{ };

    xcb_intern_atom_cookie_t x_intern_atom(const std::string_view name);
    xcb_intern_atom_cookie_t x_intern_existing_atom(const std::string_view name);
    xcb_atom_t x_intern_atom_reply(const xcb_intern_atom_cookie_t request);

    void open_x_connection(const cstring display);
    void close_x_connection() noexcept;

    io_subsystem(const cstring display);
  public:
    io_subsystem();
    io_subsystem(const std::string_view display);

    ~io_subsystem() noexcept;

    ptr<xcb_connection_t> x_connection();
    ptr<xcb_screen_t> x_screen();
    xcb_atom_t x_wm_name_atom();
    xcb_atom_t x_wm_protocols_atom();
    xcb_atom_t x_wm_delete_window_atom();
    xcb_atom_t x_wm_normal_hints_atom();
  };

}

#endif
