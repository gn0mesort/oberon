#ifndef OBERON_DETAIL_WINDOW_IMPL_HPP
#define OBERON_DETAIL_WINDOW_IMPL_HPP

#include <vector>

#include "../window.hpp"
#include "../bounds.hpp"

#include "object_impl.hpp"
#include "x11.hpp"
#include "vulkan.hpp"

namespace oberon {
namespace detail {
  struct context_impl;

  struct window_impl : public object_impl {
    bool was_close_requested{ false };

    xcb_window_t x11_window{ };
    xcb_atom_t x11_wm_protocols_atom{ };
    xcb_atom_t x11_delete_atom{ };

    bounding_rect bounds{ };

    VkSurfaceKHR surface{ };

    virtual ~window_impl() noexcept = default;
  };

  iresult create_fullscreen_x11_window(const context_impl& ctx, window_impl& window, bounding_rect& bounds) noexcept;
  iresult create_x11_window(const context_impl& ctx, window_impl& window) noexcept;
  iresult create_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept;
  iresult display_x11_window(const context_impl& ctx, window_impl& window) noexcept;
  iresult handle_x11_expose(window_impl& window, const events::window_expose_data& expose) noexcept;
  iresult handle_x11_message(window_impl& window, const events::window_message_data& message) noexcept;
  iresult handle_x11_resize(window_impl& window, const events::window_configure_data& configure) noexcept;
  /*
  iresult translate_x11_message(
    const window_impl& window,
    const std::array<u8, 20>& message,
    window_message& translated
  ) noexcept;
  */
  iresult hide_x11_window(const context_impl& ctx, window_impl& window) noexcept;
  iresult destroy_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept;
  iresult destroy_x11_window(const context_impl& ctx, window_impl& window) noexcept;
}
}

#endif
