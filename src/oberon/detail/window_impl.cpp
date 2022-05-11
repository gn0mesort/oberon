#include "oberon/detail/window_impl.hpp"

#include <cstdlib>
#include <cstring>

#include "oberon/bounds.hpp"
#include "oberon/debug.hpp"
#include "oberon/application.hpp"

#include "oberon/detail/io_subsystem.hpp"
#include "oberon/detail/graphics_subsystem.hpp"

namespace oberon::detail {

  void window_impl_dtor::operator()(const ptr<window_impl> p) const noexcept {
    delete p;
  }

  // Pre: no io, no graphics
  // Post: io, graphics
  void window_impl::open_parent_systems(io_subsystem& io, graphics_subsystem& graphics) {
    OBERON_PRECONDITION(!m_io);
    OBERON_PRECONDITION(!m_graphics);
    m_io = &io;
    m_graphics = &graphics;
    OBERON_POSTCONDITION(m_io);
    OBERON_POSTCONDITION(m_graphics);
  }

  // Post: no io, no graphics
  void window_impl::close_parent_systems() noexcept {
    m_io = nullptr;
    m_graphics = nullptr;
    OBERON_POSTCONDITION(!m_io);
    OBERON_POSTCONDITION(!m_graphics);
  }

  // Pre: no window
  // Post: window
  void window_impl::open_x_window(const std::string_view title, const bounding_rect& bounds) {
    OBERON_PRECONDITION(m_window_id == XCB_NONE);
    const auto conn = m_io->x_connection();
    const auto parent = m_io->x_screen()->root;
    const auto window_id = xcb_generate_id(conn);
    const auto valmask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    const auto eventmask = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS |
                           XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE |
                           XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    const auto vallist = std::array<u32, 3>{ m_io->x_screen()->black_pixel, false, eventmask };
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, window_id, parent, bounds.position.x, bounds.position.y,
                      bounds.size.width, bounds.size.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT,
                      valmask, std::data(vallist));
    m_window_id = window_id;
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_WM_NAME), XCB_ATOM_STRING, 8,
                        std::size(title), std::data(title));
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_NET_WM_NAME),
                        m_io->x_atom(X_ATOM_UTF8_STRING), 8, std::size(title), std::data(title));
    const auto hostname = m_io->hostname();
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_WM_CLIENT_MACHINE),
                        XCB_ATOM_STRING, 8, std::size(hostname), std::data(hostname));
    const auto wm_protocols = m_io->x_atom(X_ATOM_WM_PROTOCOLS);
    const auto wm_delete_window = m_io->x_atom(X_ATOM_WM_DELETE_WINDOW);
    xcb_change_property(conn, XCB_PROP_MODE_APPEND, m_window_id, wm_protocols, XCB_ATOM_ATOM, 32, 1,
                        &wm_delete_window);
    const auto net_wm_ping = m_io->x_atom(X_ATOM_NET_WM_PING);
    xcb_change_property(conn, XCB_PROP_MODE_APPEND, m_window_id, wm_protocols, XCB_ATOM_ATOM, 32, 1, &net_wm_ping);
    const auto pid = m_io->process_id();
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_NET_WM_PID), XCB_ATOM_CARDINAL,
                        32, 1, &pid);
    auto hints = x_size_hints{ };
    hints.flags = X_SIZE_HINT_PROGRAM_MIN_SIZE | X_SIZE_HINT_PROGRAM_MAX_SIZE;
    hints.min_width = bounds.size.width;
    hints.min_height = bounds.size.height;
    hints.max_width = bounds.size.width;
    hints.max_height = bounds.size.height;
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_WM_NORMAL_HINTS),
                        XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(x_size_hints) >> 2, &hints);
    OBERON_POSTCONDITION(m_window_id != XCB_NONE);
  }

  // Post: no window
  void window_impl::close_x_window() noexcept {
    if (m_window_id != XCB_NONE)
    {
      hide();
      xcb_destroy_window(m_io->x_connection(), m_window_id);
      m_window_id = XCB_NONE;
    }
    OBERON_POSTCONDITION(m_window_id == XCB_NONE);
  }


  // Pre: no surface, graphics->instance
  // Post: surface
  void window_impl::open_vk_surface() {
    OBERON_PRECONDITION(m_graphics->vk_instance());
    OBERON_PRECONDITION(m_surface == VK_NULL_HANDLE);
    const auto& vkdl = m_graphics->vk_loader();
    auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
    surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_info.window = m_window_id;
    surface_info.connection = m_io->x_connection();
    OBERON_DECLARE_VK_PFN(vkdl, CreateXcbSurfaceKHR);
    OBERON_VK_SUCCEEDS(vkCreateXcbSurfaceKHR(m_graphics->vk_instance(), &surface_info, nullptr, &m_surface),
                       oberon::errors::vk_create_surface_failed_error{ });
    OBERON_POSTCONDITION(m_surface != VK_NULL_HANDLE);
  }

  // Pre: graphics->instance
  // Post: no surface
  void window_impl::close_vk_surface() noexcept {
    OBERON_PRECONDITION(m_graphics->vk_instance());
    if (m_surface != VK_NULL_HANDLE)
    {
      const auto& vkdl = m_graphics->vk_loader();
      OBERON_DECLARE_VK_PFN(vkdl, DestroySurfaceKHR);
      vkDestroySurfaceKHR(m_graphics->vk_instance(), m_surface, nullptr);
      m_surface = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(m_surface == VK_NULL_HANDLE);
  }

  window_impl::window_impl(context& ctx, const std::string_view title, const bounding_rect& bounds) {
    open_parent_systems(ctx.io(), ctx.graphics());
    open_x_window(title, bounds);
    open_vk_surface();
  }

  window_impl::~window_impl() noexcept {
    close_vk_surface();
    close_x_window();
    close_parent_systems();
  }

  bitmask window_impl::get_signals() const {
    return  m_window_signals;
  }

  void window_impl::clear_signals(const bitmask sigs) {
    m_window_signals = m_window_signals & ~sigs;
  }

  bitmask window_impl::get_flags() const {
    return m_window_state;
  }

  void window_impl::show() {
    xcb_map_window(m_io->x_connection(), m_window_id);
    xcb_flush(m_io->x_connection());
    m_window_state = m_window_state | window_flag_bits::shown_bit;
  }

  void window_impl::hide() {
    xcb_unmap_window(m_io->x_connection(), m_window_id);
    xcb_flush(m_io->x_connection());
    m_window_state = m_window_state & ~window_flag_bits::shown_bit;
  }

  void window_impl::accept_message(const ptr<void> message) {
    auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(message);
    OBERON_ASSERT(client_message->window == m_window_id);
    if (client_message->type == m_io->x_atom(X_ATOM_WM_PROTOCOLS))
    {
      if (client_message->data.data32[0] == m_io->x_atom(X_ATOM_NET_WM_PING))
      {
        auto root = m_io->x_screen()->root;
        client_message->window = root;
        auto buffer = xcb_generic_event_t{ };
        std::memcpy(reinterpret_cast<ptr<char>>(&buffer), client_message, sizeof(xcb_client_message_event_t));
        m_io->x_send_event(root, buffer, 0, false);
        m_last_ping = client_message->data.data32[1];
      }
      if (client_message->data.data32[0] == m_io->x_atom(X_ATOM_WM_DELETE_WINDOW))
      {
        m_window_signals = m_window_signals | window_signal_bits::destroy_bit;
        hide();
      }
    }
  }

}
