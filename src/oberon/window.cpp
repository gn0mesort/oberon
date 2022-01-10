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
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    OBERON_PRECONDITION(window.x11_window);
    OBERON_DECLARE_PFN(ctx.ld, CreateXcbSurfaceKHR);
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
    window.is_hidden = false;
    return 0;
  }



  iresult handle_x11_configure(window_impl& window, const ptr<xcb_configure_notify_event_t> ev) noexcept {
    OBERON_PRECONDITION(window.x11_window);
    auto result = iresult{ 0 };
    if (ev->width != window.bounds.size.width || ev->height != window.bounds.size.height)
    {
      result |= WINDOW_CONFIGURE_RESIZE_BIT;
      window.bounds.size = { ev->width, ev->height };
    }
    else if (ev->x != window.bounds.position.x || ev->y != window.bounds.position.y)
    {
      result |= WINDOW_CONFIGURE_REPOSITION_BIT;
      window.bounds.position = { ev->x, ev->y };
    }
    return result;
  }

  iresult handle_x11_message(window_impl& window, const ptr<xcb_client_message_event_t> ev) noexcept {
    OBERON_PRECONDITION(window.x11_window);
    OBERON_PRECONDITION(window.x11_delete_atom != XCB_ATOM_NONE);
    if (ev->data.data32[0] == window.x11_delete_atom)
    {
      window.is_hidden = true;
      return WINDOW_MESSAGE_HIDE;
    }
    return -1;
  }

  iresult hide_x11_window(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    xcb_unmap_window(ctx.x11_connection, window.x11_window);
    xcb_flush(ctx.x11_connection);
    window.is_hidden = true;
    return 0;
  }

  iresult destroy_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept {
    if (!window.surface)
    {
      return 0;
    }
    OBERON_ASSERT(ctx.instance);
    OBERON_DECLARE_PFN(ctx.ld, DestroySurfaceKHR);
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
    auto& win = reference_cast<detail::window_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(dependency<context>().implementation());
    detail::hide_x11_window(ctx, win);
    detail::destroy_vulkan_surface(ctx, win);
    detail::remove_window_from_context(ctx, win.x11_window);
    detail::destroy_x11_window(ctx, win);
  }

  window::window(context& ctx, const ptr<detail::window_impl> impl) : object{ impl } {
    store_dependency<context>(ctx);
  }

  window::window(context& ctx) : object{ new detail::window_impl{ } } {
    store_dependency<context>(ctx);
  }

  window::window(context& ctx, const bounding_rect& bounds) : object{ new detail::window_impl{ } } {
    store_dependency<context>(ctx);
    auto& win = reference_cast<detail::window_impl>(implementation());
    auto& ctx_impl = reference_cast<detail::context_impl>(dependency<context>().implementation());
    detail::create_x11_window(ctx_impl, win, bounds);
    detail::add_window_to_context(ctx_impl, win.x11_window, this);
    if (OBERON_IS_IERROR(detail::create_vulkan_surface(ctx_impl, win)))
    {
      throw fatal_error{ "Failed to create Vulkan window surface." };
    }
    detail::display_x11_window(ctx_impl, win);
  }

  window::~window() noexcept {
    dispose();
  }

  imax window::id() const {
    auto& win = reference_cast<detail::window_impl>(implementation());
    return win.x11_window;
  }

  bool window::is_hidden() const {
    auto& win = reference_cast<detail::window_impl>(implementation());
    return win.is_hidden;
  }

  window& window::show() {
    auto& win = reference_cast<detail::window_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(dependency<context>().implementation());
    if (win.is_hidden)
    {
      detail::display_x11_window(ctx, win);
    }
    return *this;
  }

  window& window::hide() {
    auto& win = reference_cast<detail::window_impl>(implementation());
    auto& ctx = reference_cast<detail::context_impl>(dependency<context>().implementation());
    if (!win.is_hidden)
    {
      detail::hide_x11_window(ctx, win);
    }
    return *this;
  }

  const extent_2d& window::size() const {
    auto& win = reference_cast<detail::window_impl>(implementation());
    return win.bounds.size;
  }

  usize window::width() const {
    return size().width;
  }

  usize window::height() const {
    return size().height;
  }
/*
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

  window_message window::translate_message(const events::window_message_data& message) const {
    auto q = q_ptr<detail::window_impl>();
    auto result = window_message{ };
    translate_x11_message(*q, message.content, result);
    return result;
  }
*/
}
