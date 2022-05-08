#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <string_view>

#include "basics.hpp"

OBERON_OPAQUE_BASE_FWD(window);

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

    void show();
    void hide();
  };

}

#endif
