#ifndef OBERON_RENDER_WINDOW_HPP
#define OBERON_RENDER_WINDOW_HPP

#include <unordered_set>

#include "memory.hpp"
#include "bounds.hpp"

namespace oberon::detail {

  class render_window_impl;
  struct render_window_impl_dtor final {
    void operator()(const ptr<render_window_impl> p) const noexcept;
  };

}

namespace oberon {

  class context;

  class render_window final {
  private:
    std::unique_ptr<detail::render_window_impl, detail::render_window_impl_dtor> m_impl{ };
  public:
    enum class presentation_mode {
      vulkan_fifo,
      vulkan_fifo_relaxed,
      vulkan_immediate,
      vulkan_mailbox
    };
    render_window(context& ctx, const bounding_rect& bounds);
    render_window(render_window&& other) = default;

    ~render_window() noexcept;

    render_window& operator=(render_window&& rhs) = default;

    const std::unordered_set<presentation_mode> supported_presentation_modes() const;

    void show();
    void hide();
  };

}

#endif
