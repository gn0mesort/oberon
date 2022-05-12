#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <string_view>

#include "basics.hpp"
#include "bounds.hpp"


namespace oberon::detail {

  OBERON_OPAQUE_BASE_FWD(window);

}

namespace oberon {

  class context;


  class window final {
  public:
    // These values map to the internal window_present_mode_bits bitmask.
    enum class presentation_mode {
      // Core modes
      immediate = 0x1,
      mailbox = 0x2,
      fifo = 0x4,
      fifo_relaxed = 0x8,

      // Aliases
      vsync = fifo
    };

    struct config final {
      std::string_view title{ };
      bounding_rect bounds{ };
      presentation_mode preferred_present_mode{ presentation_mode::fifo };
      u32 preferred_buffer_count{ 3 };
    };
  private:
    OBERON_OPAQUE_BASE_PTR(detail::window);
  public:
    window(context& ctx, const config& conf);
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
