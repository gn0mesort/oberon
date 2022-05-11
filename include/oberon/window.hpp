#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <string_view>

#include "basics.hpp"

OBERON_OPAQUE_BASE_FWD(window);

namespace oberon::window_flag_bits {

  OBERON_DEFINE_BIT(none, 0);
  OBERON_DEFINE_BIT(shown, 1);

}

namespace oberon::window_signal_bits {

  OBERON_DEFINE_BIT(none, 0);
  OBERON_DEFINE_BIT(destroy, 1);

}

namespace oberon {

  struct bounding_rect;
  class context;

  class window final {
  private:
    OBERON_OPAQUE_BASE_PTR(window);
  public:
    window(context& ctx, const std::string_view title, const bounding_rect& bounds);
    window(const window& other) = delete;
    window(window&& other) = default;

    ~window() noexcept = default;

    window& operator=(const window& rhs) = delete;
    window& operator=(window&& rhs) = default;

    bitmask get_signals() const;
    void clear_signals(const bitmask signals);
    bitmask get_flags() const;

    void show();
    void hide();
    void accept_message(const ptr<void> message);
  };

}

#endif
