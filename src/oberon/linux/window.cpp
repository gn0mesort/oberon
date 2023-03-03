/**
 * @file window.cpp
 * @brief Window class implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/window.hpp"

#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "oberon/debug.hpp"
#include "oberon/linux/system.hpp"

#define OBERON_WINDOW_PRECONDITIONS \
  OBERON_PRECONDITION(m_parent); \
  OBERON_PRECONDITION(m_window_id != XCB_NONE)

namespace oberon::linux {

  window::window(system& sys) : m_parent{ &sys } {
    OBERON_PRECONDITION(m_parent);
    auto connection = m_parent->connection();
    auto screen = m_parent->default_screen();
    auto instance_name = m_parent->instance_name();
    auto application_name = m_parent->application_name();
    m_window_id = xcb_generate_id(connection);
    OBERON_ASSERT(m_window_id != XCB_NONE);
    auto depth = XCB_COPY_FROM_PARENT;
    auto parent = m_parent->root_window_id();
    auto border = u16{ 0 };
    auto visual = XCB_COPY_FROM_PARENT;
    auto window_class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
    auto value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    auto event_mask = u32 { XCB_EVENT_MASK_EXPOSURE |
                            XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                            XCB_EVENT_MASK_KEY_PRESS |
                            XCB_EVENT_MASK_KEY_RELEASE |
                            XCB_EVENT_MASK_POINTER_MOTION |
                            XCB_EVENT_MASK_BUTTON_PRESS |
                            XCB_EVENT_MASK_BUTTON_RELEASE |
                            XCB_EVENT_MASK_ENTER_WINDOW |
                            XCB_EVENT_MASK_FOCUS_CHANGE |
                            XCB_EVENT_MASK_PROPERTY_CHANGE };
    auto override_redirect = false;
    auto value_array = std::array<u32, 3>{ screen->black_pixel, override_redirect, event_mask };
    xcb_create_window(connection, depth, m_window_id, parent, 0, 0, 320, 180, border, window_class,
                      visual, value_mask, value_array.data());
    // Set WM_CLASS
    // WM_CLASS is an odd value in that both strings are explicitly null terminated (i.e., cstrings). Other string
    // atoms don't necessarily care about the null terminator.
    // Additionally, the first value (i.e., the instance name) must be acquired in accordance with ICCCM. This means
    // that the -name argument is checked first, then RESOURCE_NAME, and finally basename(argv[0]) is used.
    // see https://www.x.org/releases/current/doc/xorg-docs/icccm/icccm.html#WM_CLASS_Property
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_CLASS), XCB_ATOM_STRING, 8,
                        instance_name.size() + 1, instance_name.c_str());
    xcb_change_property(connection, XCB_PROP_MODE_APPEND, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_CLASS), XCB_ATOM_STRING, 8,
                        application_name.size() + 1, application_name.c_str());
    // Set _NET_WM_PID
    auto pid = getpid();
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_PID), XCB_ATOM_CARDINAL, 32, 1, &pid);
    // Set WM_CLIENT_MACHINE
    auto uname_buffer = utsname{ };
    OBERON_CHECK(uname(&uname_buffer) == 0);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_CLIENT_MACHINE), XCB_ATOM_STRING, 8,
                        std::strlen(uname_buffer.nodename), &uname_buffer.nodename[0]);
    // Set WM_PROTOCOLS
    auto protocol_atoms = std::array<xcb_atom_t, 2>{ m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_DELETE_WINDOW),
                                                     m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_PING) };
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_PROTOCOLS), XCB_ATOM_ATOM, 32, 2,
                        protocol_atoms.data());
    // Set WM_NAME and _NET_WM_NAME
    wm_set_title(application_name);
    // Set _NET_WM_BYPASS_COMPOSITOR
    wm_change_compositor_mode(OBERON_LINUX_X_NET_WM_BYPASS_COMPOSITOR_NO_PREFERENCE);
    // Set WM_NORMAL_HINTS
    wm_lock_resize({ 320, 180 });
  }

  window::~window() noexcept {
    OBERON_WINDOW_PRECONDITIONS;
    xcb_destroy_window(m_parent->connection(), m_window_id);
    xcb_flush(m_parent->connection());
  }

  void window::wm_send_message(const xcb_atom_t atom, const std::array<u32, 5>& message) {
    OBERON_WINDOW_PRECONDITIONS;
    // This is the proper form of an EWMH client message.
    // https://specifications.freedesktop.org/wm-spec/latest/ar01s03.html
    auto event = xcb_generic_event_t{ };
    std::memset(&event, 0, sizeof(xcb_generic_event_t));
    const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(&event);
    client_message->window = m_window_id;
    client_message->response_type = XCB_CLIENT_MESSAGE;
    client_message->type = atom;
    client_message->format = 32;
    std::memcpy(client_message->data.data32, message.data(), sizeof(xcb_client_message_data_t));
    const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
    xcb_send_event(m_parent->connection(), false, m_parent->root_window_id(), mask, reinterpret_cast<cstring>(&event));
  }

  void window::wm_change_state(const wm_state_mode mode, const xcb_atom_t first, const xcb_atom_t second) {
    OBERON_WINDOW_PRECONDITIONS;
    wm_send_message(m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE),
                    { mode, first, second, OBERON_LINUX_X_SOURCE_INDICATION_APPLICATION });
  }

  void window::wm_change_compositor_mode(const compositor_mode mode) {
    OBERON_WINDOW_PRECONDITIONS;
    xcb_change_property(m_parent->connection(), XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_BYPASS_COMPOSITOR), XCB_ATOM_CARDINAL,
                        32, 1, &mode);
  }

  oberon::window::id window::unique_id() const {
    OBERON_WINDOW_PRECONDITIONS;
    return m_window_id;
  }

  window& window::change_display_style(const display_style style) {
    OBERON_WINDOW_PRECONDITIONS;
    if (style == current_display_style())
    {
      return *this;
    }
    switch (style)
    {
    case display_style::windowed:
      wm_change_state(OBERON_LINUX_X_NET_WM_STATE_REMOVE,
                      m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_FULLSCREEN),
                      m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_ABOVE));
      wm_change_compositor_mode(OBERON_LINUX_X_NET_WM_BYPASS_COMPOSITOR_NO_PREFERENCE);
      m_display_style = display_style::windowed;
      break;
    // This approximates "borderless fullscreen window" mode.
    case display_style::fullscreen_composited:
      wm_change_state(OBERON_LINUX_X_NET_WM_STATE_ADD,
                      m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_FULLSCREEN), XCB_NONE);
      wm_change_state(OBERON_LINUX_X_NET_WM_STATE_REMOVE,
                      m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_ABOVE), XCB_NONE);
      wm_change_compositor_mode(OBERON_LINUX_X_NET_WM_BYPASS_COMPOSITOR_NO_PREFERENCE);
      m_display_style = display_style::fullscreen_composited;
      break;
    // This approximates "fullscreen exclusive" mode. I've had a difficult time finding any evidence that
    // "fullscreen exclusive" is actually a meaningful term. That's why I've chosen to call this
    // fullscreen uncomposited mode instead. In compliant window managers this should disable compositing.
    case display_style::fullscreen_uncomposited:
      wm_change_state(OBERON_LINUX_X_NET_WM_STATE_ADD,
                      m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_FULLSCREEN),
                      m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_ABOVE));
      wm_change_compositor_mode(OBERON_LINUX_X_NET_WM_BYPASS_COMPOSITOR_DISABLE);
      m_display_style = display_style::fullscreen_uncomposited;
      break;
    }
    xcb_flush(m_parent->connection());
    return *this;
  }

  oberon::window::display_style window::current_display_style() const {
    OBERON_WINDOW_PRECONDITIONS;
    auto result = display_style::windowed;
    auto fullscreen = false;
    auto above = false;
    auto request = xcb_get_property(m_parent->connection(), false, m_window_id,
                                    m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE), XCB_ATOM_ATOM, 0,
                                    1024);
    auto reply = ptr<xcb_get_property_reply_t>{ };
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_get_property_reply(m_parent->connection(), request, err));
    if (reply->format == 32 && reply->type == XCB_ATOM_ATOM)
    {
      auto states = static_cast<readonly_ptr<xcb_atom_t>>(xcb_get_property_value(reply));
      auto state_count = reply->length;
      for (auto i = usize{ 0 }; i < state_count; ++i)
      {
        if (states[i] == m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_FULLSCREEN))
        {
          fullscreen = true;
        }
        else if (states[i] == m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_STATE_ABOVE))
        {
          above = true;
        }
      }
    }
    std::free(reply);
    auto compositor = OBERON_LINUX_X_NET_WM_BYPASS_COMPOSITOR_NO_PREFERENCE;
    request = xcb_get_property(m_parent->connection(), false, m_window_id,
                               m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_BYPASS_COMPOSITOR),
                               XCB_ATOM_CARDINAL, 0, 1);
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_get_property_reply(m_parent->connection(), request, err));
    if (reply->format == 32 && reply->type == XCB_ATOM_CARDINAL)
    {
      auto mode_ptr = static_cast<readonly_ptr<u32>>(xcb_get_property_value(reply));
      compositor = static_cast<compositor_mode>(*mode_ptr);
    }
    std::free(reply);
    if (fullscreen)
    {
      if (above && compositor == OBERON_LINUX_X_NET_WM_BYPASS_COMPOSITOR_DISABLE)
      {
        result =  display_style::fullscreen_uncomposited;
      }
      result = display_style::fullscreen_composited;
    }
    return result;
  }

  window& window::show() {
    OBERON_WINDOW_PRECONDITIONS;
    xcb_map_window(m_parent->connection(), m_window_id);
    xcb_flush(m_parent->connection());
    return *this;
  }

  window& window::hide() {
    OBERON_WINDOW_PRECONDITIONS;
    xcb_unmap_window(m_parent->connection(), m_window_id);
    xcb_flush(m_parent->connection());
    return *this;
  }

  bool window::is_visible() const {
    OBERON_WINDOW_PRECONDITIONS;
    auto request = xcb_get_window_attributes(m_parent->connection(), m_window_id);
    auto reply = ptr<xcb_get_window_attributes_reply_t>{ };
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_get_window_attributes_reply(m_parent->connection(), request, err));
    // The window is visible if it's mapped.
    // This doesn't handle iconified windows.
    auto result = reply->map_state == XCB_MAP_STATE_VIEWABLE;
    std::free(reply);
    return result;
  }

  void window::wm_unlock_resize() {
    OBERON_WINDOW_PRECONDITIONS;
    auto sz = size_hints{ };
    sz.flags = size_hint_flag_bits::program_max_size_bit | size_hint_flag_bits::program_min_size_bit |
               size_hint_flag_bits::user_position_bit | size_hint_flag_bits::user_size_bit;
    // This should make the window sizable between 0x0 pixels and whatever the maximum is.
    // No one has a 65535x65535 monitor so this should be fine for the time being.
    sz.min_width = 0;
    sz.min_height = 0;
    sz.max_width = std::numeric_limits<u16>::max();
    sz.max_height = std::numeric_limits<u16>::max();
    xcb_change_property(m_parent->connection(), XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_NORMAL_HINTS), XCB_ATOM_WM_SIZE_HINTS, 32,
                        sizeof(size_hints) >> 2, &sz);
  }

  void window::wm_lock_resize(const window_extent& extent) {
    OBERON_WINDOW_PRECONDITIONS;
    auto sz = size_hints{ };
    sz.flags = size_hint_flag_bits::program_max_size_bit | size_hint_flag_bits::program_min_size_bit |
               size_hint_flag_bits::user_position_bit | size_hint_flag_bits::user_size_bit;
    // This should prevent resizing in EWMH window managers.
    sz.min_width = extent.width;
    sz.min_height = extent.height;
    sz.max_width = extent.width;
    sz.max_height = extent.height;
    xcb_change_property(m_parent->connection(), XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_NORMAL_HINTS), XCB_ATOM_WM_SIZE_HINTS, 32,
                        sizeof(size_hints) >> 2, &sz);
  }

  window& window::resize(const window_extent& size) {
    OBERON_WINDOW_PRECONDITIONS;
    wm_unlock_resize();
    auto mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    auto values = std::array<u32, 2>{ size.width, size.height };
    xcb_configure_window(m_parent->connection(), m_window_id, mask, values.data());
    wm_lock_resize(size);
    xcb_flush(m_parent->connection());
    return *this;
  }

  window& window::move_to(const window_offset& position) {
    OBERON_WINDOW_PRECONDITIONS;
    auto mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
    auto values = std::array<i32, 2>{ position.x, position.y };
    xcb_configure_window(m_parent->connection(), m_window_id, mask, values.data());
    xcb_flush(m_parent->connection());
    return *this;
  }

  window_rect window::current_drawable_rect() const {
    OBERON_WINDOW_PRECONDITIONS;
    auto result = window_rect{ };
    {
      auto request = xcb_get_geometry(m_parent->connection(), m_window_id);
      auto reply = ptr<xcb_get_geometry_reply_t>{ };
      OBERON_LINUX_X_SUCCEEDS(reply, xcb_get_geometry_reply(m_parent->connection(), request, err));
      result = window_rect{ { reply->x, reply->y }, { reply->width, reply->height } };
      std::free(reply);
    }
    {
      auto request = xcb_translate_coordinates(m_parent->connection(), m_window_id, m_parent->root_window_id(),
                                               result.offset.x, result.offset.y);
      auto reply = ptr<xcb_translate_coordinates_reply_t>{ };
      OBERON_LINUX_X_SUCCEEDS(reply, xcb_translate_coordinates_reply(m_parent->connection(), request, err));
      result.offset = { reply->dst_x, reply->dst_y };
      std::free(reply);
    }
    return result;
  }

  window_rect window::current_rect() const {
    OBERON_WINDOW_PRECONDITIONS;
    auto result = current_drawable_rect();
    auto request = xcb_get_property(m_parent->connection(), false, m_window_id,
                                    m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_FRAME_EXTENTS),
                                    XCB_ATOM_CARDINAL, 0, 4);
    auto reply = ptr<xcb_get_property_reply_t>{ };
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_get_property_reply(m_parent->connection(), request, err));
    if (reply->format == 32 && reply->type == XCB_ATOM_CARDINAL && reply->length == 4)
    {
      auto values = reinterpret_cast<readonly_ptr<u32>>(xcb_get_property_value(reply));
      result.offset = { static_cast<i16>(result.offset.x - values[0]),
                        static_cast<i16>(result.offset.y - values[2]) };
      result.extent = { static_cast<u16>(result.extent.width + values[0] + values[1]),
                        static_cast<u16>(result.extent.height + values[2] + values[3]) };
    }
    std::free(reply);
    return result;
  }

  void window::wm_set_title(const std::string& title) {
    OBERON_WINDOW_PRECONDITIONS;
    auto connection = m_parent->connection();
    // The encoding of WM_NAME is ambiguous.
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_WM_NAME), XCB_ATOM_STRING, 8, title.size(),
                        title.c_str());
    // _NET_WM_NAME should be preferred and is unambiguously UTF-8 encoded.
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window_id,
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_NAME),
                        m_parent->atom_from_name(OBERON_LINUX_X_ATOM_UTF8_STRING), 8, title.size(), title.c_str());
  }

  window& window::change_title(const std::string& title) {
    OBERON_WINDOW_PRECONDITIONS;
    wm_set_title(title);
    xcb_flush(m_parent->connection());
    return *this;
  }

  std::string window::current_title() const {
    OBERON_WINDOW_PRECONDITIONS;
    const auto utf = m_parent->atom_from_name(OBERON_LINUX_X_ATOM_UTF8_STRING);
    auto request = xcb_get_property(m_parent->connection(), false, m_window_id,
                                    m_parent->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_NAME),
                                    m_parent->atom_from_name(OBERON_LINUX_X_ATOM_UTF8_STRING), 0, 1024 << 2);
    auto reply = ptr<xcb_get_property_reply_t>{ };
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_get_property_reply(m_parent->connection(), request, err));
    auto result = std::string{ "" };
    if (reply->format == 8 && reply->type == utf)
    {
      result = reinterpret_cast<cstring>(xcb_get_property_value(reply));
    }
    std::free(reply);
    return result;
  }

  bool window::quit_requested() const {
    OBERON_WINDOW_PRECONDITIONS;
    return m_quit_requested;
  }

  window& window::request_quit() {
    OBERON_WINDOW_PRECONDITIONS;
    hide();
    m_quit_requested = true;
    return *this;
  }

  window& window::clear_quit_request() {
    OBERON_WINDOW_PRECONDITIONS;
    m_quit_requested = false;
    return *this;
  }

}
