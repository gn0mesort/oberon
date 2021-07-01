#include "oberon/detail/window_impl.hpp"

#include <cstring>

#include "oberon/debug.hpp"
#include "oberon/events.hpp"
#include "oberon/errors.hpp"

#include "oberon/detail/context_impl.hpp"

namespace oberon {
namespace detail {

  iresult create_x11_window(const context_impl& ctx, window_impl& window, const bounding_rect& bounds) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    OBERON_PRECONDITION(ctx.x11_screen);
    window.x11_window = xcb_generate_id(ctx.x11_connection); // Can this fail??
    {
      auto depth = XCB_COPY_FROM_PARENT;
      auto parent = ctx.x11_screen->root;
      auto border = u16{ 0 };
      auto visual = XCB_COPY_FROM_PARENT;
      auto window_class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
      auto value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
      auto event_mask = XCB_EVENT_MASK_EXPOSURE |
                        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                        XCB_EVENT_MASK_KEY_PRESS |
                        XCB_EVENT_MASK_KEY_RELEASE |
                        XCB_EVENT_MASK_POINTER_MOTION |
                        XCB_EVENT_MASK_BUTTON_PRESS |
                        XCB_EVENT_MASK_BUTTON_RELEASE |
                        XCB_EVENT_MASK_ENTER_WINDOW |
                        XCB_EVENT_MASK_FOCUS_CHANGE |
                        XCB_EVENT_MASK_PROPERTY_CHANGE;
      auto value_array = std::array<u32, 3>{ ctx.x11_screen->black_pixel, false, static_cast<u32>(event_mask) };
      xcb_create_window(
        ctx.x11_connection,
        depth,
        window.x11_window,
        parent,
        bounds.position.x, bounds.position.y, bounds.size.width, bounds.size.height,
        border,
        window_class,
        visual,
        value_mask, std::data(value_array)
      );
    }
    {
      auto protocols_cookie = xcb_intern_atom(ctx.x11_connection, false, 12, "WM_PROTOCOLS");
      auto delete_cookie = xcb_intern_atom(ctx.x11_connection, false, 16, "WM_DELETE_WINDOW");
      auto* protocols_reply = xcb_intern_atom_reply(ctx.x11_connection, protocols_cookie, nullptr);
      auto* delete_reply = xcb_intern_atom_reply(ctx.x11_connection, delete_cookie, nullptr);
      window.x11_wm_protocols_atom = protocols_reply->atom;
      window.x11_delete_atom = delete_reply->atom;
      std::free(protocols_reply);
      std::free(delete_reply);
      xcb_change_property(
        ctx.x11_connection,
        XCB_PROP_MODE_REPLACE,
        window.x11_window,
        window.x11_wm_protocols_atom,
        XCB_ATOM_ATOM,
        32,
        1,
        &window.x11_delete_atom
      );
      xcb_change_property(
        ctx.x11_connection,
        XCB_PROP_MODE_REPLACE,
        window.x11_window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        std::size(ctx.application_name),
        std::data(ctx.application_name)
      );
    }
    window.bounds = bounds;
    OBERON_POSTCONDITION(window.x11_window);
    return 0;
  }

  iresult create_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.instance);
    OBERON_PRECONDITION(ctx.vkft.vkCreateXcbSurfaceKHR);
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    OBERON_PRECONDITION(window.x11_window);
    auto vkCreateXcbSurfaceKHR = ctx.vkft.vkCreateXcbSurfaceKHR;
    auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
    OBERON_INIT_VK_STRUCT(surface_info, XCB_SURFACE_CREATE_INFO_KHR);
    surface_info.connection = ctx.x11_connection;
    surface_info.window = window.x11_window;
    auto result = vkCreateXcbSurfaceKHR(ctx.instance, &surface_info, nullptr, &window.surface);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(window.surface);
    return 0;
  }

  iresult display_x11_window(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    xcb_map_window(ctx.x11_connection, window.x11_window);
    xcb_flush(ctx.x11_connection);
    return 0;
  }

  iresult handle_x11_expose(window_impl& window, const events::window_expose_data& expose) noexcept {
    return 0;
  }

  iresult handle_x11_message(window_impl& window, const events::window_message_data& message) noexcept {
    OBERON_PRECONDITION(window.x11_window);
    OBERON_PRECONDITION(window.x11_delete_atom != XCB_ATOM_NONE);
    if (window.x11_delete_atom == reinterpret_cast<readonly_ptr<xcb_atom_t>>(std::data(message.content))[0])
    {
      window.was_close_requested = true;
    }
    return 0;
  }

  iresult handle_x11_configure(window_impl& window, const events::window_configure_data& configure) noexcept {
    OBERON_PRECONDITION(window.x11_window);
    if (configure.bounds.size.width != window.bounds.size.width ||
        configure.bounds.size.height != window.bounds.size.height)
    {
      //TODO resized: recreate swapchain etc.
    }
    if (configure.bounds.position.x != window.bounds.position.x ||
        configure.bounds.position.y != window.bounds.position.y)
    {
      //TODO repositioned: probably just ignore. Positions don't seem super meaningful.
      // The reported positions are influenced by the window manager theme for instance. Themes with chunkier window
      // decorations have larger offsets from 0 on either axis.
    }
    window.bounds = configure.bounds;
    return 0;
  }

  iresult hide_x11_window(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    xcb_unmap_window(ctx.x11_connection, window.x11_window);
    xcb_flush(ctx.x11_connection);
    return 0;
  }

  iresult destroy_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept {
    if (!window.surface)
    {
      return 0;
    }
    OBERON_ASSERT(ctx.instance);
    OBERON_ASSERT(ctx.vkft.vkDestroySurfaceKHR);
    auto vkDestroySurfaceKHR = ctx.vkft.vkDestroySurfaceKHR;
    vkDestroySurfaceKHR(ctx.instance, window.surface, nullptr);
    window.surface = nullptr;
    OBERON_POSTCONDITION(!window.surface);
    return 0;
  }

  iresult destroy_x11_window(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    xcb_destroy_window(ctx.x11_connection, window.x11_window);
    window.x11_window = 0;
    OBERON_POSTCONDITION(!window.x11_window);
    return 0;
  }

}

  void window::v_dispose() noexcept {
    auto& q = reference_cast<detail::window_impl>(implementation());
    auto& parent_q = reference_cast<detail::context_impl>(parent().implementation());
    detail::hide_x11_window(parent_q, q);
    detail::destroy_vulkan_surface(parent_q, q);
    detail::destroy_x11_window(parent_q, q);
  }

  window::window(const context& ctx, const ptr<detail::window_impl> impl) : object{ impl, &ctx } { }

  window::window(const context& ctx) : object{ new detail::window_impl{ }, &ctx } {
  }

  window::window(const context& ctx, const bounding_rect& bounds) : object{ new detail::window_impl{ }, &ctx } {
    auto& q = reference_cast<detail::window_impl>(implementation());
    auto& parent_q = reference_cast<detail::context_impl>(parent().implementation());
    detail::create_x11_window(parent_q, q, bounds);
    if (OBERON_IS_IERROR(detail::create_vulkan_surface(parent_q, q)))
    {
      throw fatal_error{ "Failed to create Vulkan window surface." };
    }
    detail::display_x11_window(parent_q, q);
  }

  window::~window() noexcept {
    dispose();
  }

  imax window::id() const {
    auto& q = reference_cast<detail::window_impl>(implementation());
    return q.x11_window;
  }

  bool window::should_close() const {
    auto& q = reference_cast<detail::window_impl>(implementation());
    return q.was_close_requested;
  }

  const extent_2d& window::size() const {
    auto& q = reference_cast<detail::window_impl>(implementation());
    return q.bounds.size;
  }

  usize window::width() const {
    return size().width;
  }

  usize window::height() const {
    return size().height;
  }

  window& window::notify(const event& ev) {
    if (ev.window_id != id())
    {
      return *this;
    }
    switch (ev.type)
    {
    case event_type::window_expose:
      return this->notify(ev.data.window_expose);
    case event_type::window_message:
      return this->notify(ev.data.window_message);
    case event_type::window_configure:
      return this->notify(ev.data.window_configure);
    default:
      return *this;
    }
  }

  window& window::notify(const events::window_expose_data& expose) {
    auto& q = reference_cast<detail::window_impl>(implementation());
    detail::handle_x11_expose(q, expose);
    return *this;
  }

  window& window::notify(const events::window_message_data& message) {
    auto& q = reference_cast<detail::window_impl>(implementation());
    detail::handle_x11_message(q, message);
    return *this;
  }

  window& window::notify(const events::window_configure_data& configure) {
    auto& q = reference_cast<detail::window_impl>(implementation());
    detail::handle_x11_configure(q, configure);
    return *this;
  }
/*
  window_message window::translate_message(const events::window_message_data& message) const {
    auto q = q_ptr<detail::window_impl>();
    auto result = window_message{ };
    translate_x11_message(*q, message.content, result);
    return result;
  }
*/
}
