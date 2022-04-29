#ifndef OBERON_LINUX_IO_SUBSYSTEM_HPP
#define OBERON_LINUX_IO_SUBSYSTEM_HPP

#include <string_view>

#include "../memory.hpp"
#include "../errors.hpp"
#include "../io_subsystem.hpp"

#include "x11.hpp"

namespace oberon::linux {

  class io_subsystem final : public abstract_io_subsystem {
  private:
    xcb_ewmh_connection_t m_ewmh{ };
    ptr<xcb_screen_t> m_screen{ };

    void open_x_connection(const cstring display);
    void close_x_connection() noexcept;
  public:
    io_subsystem();
    io_subsystem(const cstring display);

    ~io_subsystem() noexcept;

    constexpr subsystem_implementation implementation() noexcept override {
      return subsystem_implementation::linux_xcb;
    }

    xcb_atom_t x_atom(const std::string_view name);
    ptr<xcb_connection_t> x_connection();
    ptr<xcb_screen_t> x_screen();
    xcb_ewmh_connection_t& x_ewmh();
  };

  static_assert(is_io_subsystem_v<io_subsystem>);

  OBERON_EXCEPTION_TYPE(x_connection_failed, "Failed to connect to X11 server.", 1);
  OBERON_EXCEPTION_TYPE(x_no_screen, "Failed to find X11 screen.", 1);
  OBERON_EXCEPTION_TYPE(x_ewmh_init_failed, "Failed to initialized Extended Window Manager Hints.", 1);
}

#endif
