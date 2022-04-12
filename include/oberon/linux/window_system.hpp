#ifndef OBERON_LINUX_WINDOW_SYSTEM_HPP
#define OBERON_LINUX_WINDOW_SYSTEM_HPP

#include "../memory.hpp"
#include "../errors.hpp"


namespace oberon::linux::detail {

  OBERON_NON_OWNING_PIMPL_FWD(window_system);

}

namespace oberon::linux {

  class window_system final {
  private:
    friend class application;
    friend class render_window;

    OBERON_NON_OWNING_PIMPL_PTR(detail, window_system);

    window_system(const ptr<detail::window_system_impl> impl);

    ~window_system() noexcept = default;
  public:
    window_system(const window_system& other) = delete;
    window_system(window_system&& other) = delete;

    window_system& operator=(const window_system& rhs) = delete;
    window_system& operator=(window_system&& rhs) = delete;
  };

  OBERON_EXCEPTION_TYPE(x_connection_failed, "Failed to connect to X server.", 1);
  OBERON_EXCEPTION_TYPE(x_no_screen, "Failed to find desired X screen.", 1);
  OBERON_EXCEPTION_TYPE(x_ewmh_init_failed, "Failed to initialize EWMH atoms.", 1);
}

#endif
