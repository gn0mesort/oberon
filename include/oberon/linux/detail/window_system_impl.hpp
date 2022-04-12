#ifndef OBERON_LINUX_DETAIL_WINDOW_SYSTEM_IMPL_HPP
#define OBERON_LINUX_DETAIL_WINDOW_SYSTEM_IMPL_HPP

#include <tuple>

#include "../window_system.hpp"

#include "x11.hpp"

namespace oberon::linux::detail {

  struct window_system_impl final {
    ptr<xcb_connection_t> connection{ };
    xcb_ewmh_connection_t ewmh{ };
    ptr<xcb_screen_t> screen{ };
  };

  std::pair<ptr<xcb_connection_t>, int> x11_create_connection(const cstring display);
  ptr<xcb_screen_t> x11_select_screen(const ptr<xcb_connection_t> connection, int screen_pref);
  void x11_init_ewmh(const ptr<xcb_connection_t> connection, xcb_ewmh_connection_t& ewmh);
  xcb_atom_t x11_atom(const ptr<xcb_connection_t> connection, const std::string_view name);

  void x11_destroy_connection(ptr<xcb_connection_t> connection) noexcept;
}

#endif
