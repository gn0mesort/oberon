#ifndef OBERON_WINDOWING_X11_WINDOW_HPP
#define OBERON_WINDOWING_X11_WINDOW_HPP

#include "../macros.hpp"

#include "../interfaces/window.hpp"

namespace oberon {
OBERON_INLINE_V_0_0 namespace v0_0 {
namespace windowing {

  class x11_window final : public interfaces::window {
  public:
    ~x11_window() noexcept = default;
  };

}
}
}

#endif
