#ifndef OBERON_WINDOWING_X11_SYSTEM_HPP
#define OBERON_WINDOWING_X11_SYSTEM_HPP

#include "../macros.hpp"

#include "../interfaces/windowing_system.hpp"

#include "x11_window.hpp"

namespace oberon {
OBERON_INLINE_V_0_0 namespace v0_0 {
namespace windowing {

  class x11_system final : public interfaces::windowing_system {
  public:
    using window_type = interfaces::window;
    x11_system();

    ~x11_system() noexcept = default;

    x11_window& create_window(const bounds_2d& bnds) const override;
    void destroy_window(interfaces::window& bnds) const override;

  };

}
}
}

#endif
