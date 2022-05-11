#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <string_view>

#include "basics.hpp"

namespace oberon::detail {

  OBERON_OPAQUE_BASE_FWD(window);

}

namespace oberon {

  struct bounding_rect;
  class context;

  class window final {
  private:
    OBERON_OPAQUE_BASE_PTR(detail::window);
  public:
    window(context& ctx, const std::string_view title, const bounding_rect& bounds);
    window(const window& other) = delete;
    window(window&& other) = default;

    ~window() noexcept = default;

    window& operator=(const window& rhs) = delete;
    window& operator=(window&& rhs) = default;

    bool is_destroy_signaled() const;
    void clear_destroy_signal();
    bool is_shown() const;

    void show();
    void hide();
    void accept_message(const ptr<void> message);
  };

}

#endif
