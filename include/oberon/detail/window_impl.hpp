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

  enum window_messages {
    WINDOW_MESSAGE_NONE = 0,
    WINDOW_MESSAGE_HIDE
  };

  enum window_configure_bits {
    WINDOW_CONFIGURE_NONE_BIT = 0,
    WINDOW_CONFIGURE_RESIZE_BIT = 0x01,
    WINDOW_CONFIGURE_REPOSITION_BIT = 0x02
  };

  struct window_impl : public object_impl {
    bool is_hidden{ true };

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
  iresult handle_x11_configure(window_impl& window, const ptr<xcb_configure_notify_event_t> ev) noexcept;
  iresult handle_x11_message(window_impl& window, const ptr<xcb_client_message_event_t> ev) noexcept;
  iresult hide_x11_window(const context_impl& ctx, window_impl& window) noexcept;
  iresult destroy_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept;
  iresult destroy_x11_window(const context_impl& ctx, window_impl& window) noexcept;
}
}

#endif
