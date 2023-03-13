/**
 * @file platform.cpp
 * @brief Linux implementation of the platform class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/platform.hpp"

#include <cstring>

#include "oberon/debug.hpp"

#include "oberon/linux/system.hpp"
#include "oberon/linux/input.hpp"
#include "oberon/linux/window.hpp"
#include "oberon/linux/graphics.hpp"

#define OBERON_PLATFORM_PRECONDITIONS \
  OBERON_PRECONDITION(m_system); \
  OBERON_PRECONDITION(m_input); \
  OBERON_PRECONDITION(m_window); \
  OBERON_PRECONDITION(m_graphics)

namespace {

  void null_event_handler(oberon::linux::platform&, const oberon::u8, const oberon::ptr<xcb_generic_event_t>) { }
  void null_xi_event_handler(oberon::linux::platform&, const oberon::u16,
                             const oberon::ptr<xcb_ge_generic_event_t>) { }
  void null_xkb_event_handler(oberon::linux::platform&, const oberon::u8,
                              const oberon::ptr<xcb_xkb_generic_event_t>) { }

  void null_key_press_event_cb(oberon::platform&, const oberon::u32, const oberon::key, const bool) { }
  void null_key_release_event_cb(oberon::platform&, const oberon::u32, const oberon::key) { }
  void null_mouse_movement_event_cb(oberon::platform&, const oberon::mouse_offset&, const oberon::mouse_offset&) { }
  void null_mouse_button_press_event_cb(oberon::platform&, const oberon::u32, const oberon::mouse_button) { }
  void null_mouse_button_release_event_cb(oberon::platform&, const oberon::u32, const oberon::mouse_button) { }
  void null_window_move_event_cb(oberon::platform&, const oberon::window_offset&) { }
  void null_window_resize_event_cb(oberon::platform&, const oberon::window_extent&) { }

}

namespace oberon::linux {

  void platform::handle_error(platform&, const u8 type, const ptr<xcb_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_ERROR);
    auto error = reinterpret_cast<ptr<xcb_generic_error_t>>(ev);
    auto code = error->error_code;
    // Since we're going to throw we need to free the event here.
    std::free(ev);
    throw x_error{ to_string(static_cast<x_error_code>(code)), code };
  }

  void platform::handle_client_message(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_CLIENT_MESSAGE);
    auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(ev);
    auto& sys = *plt.m_system;
    auto& win = *plt.m_window;
    if (client_message->type == sys.atom_from_name(OBERON_LINUX_X_ATOM_WM_PROTOCOLS))
    {
      // Handle WM_DELETE_WINDOW messages.
      if (client_message->data.data32[0] == sys.atom_from_name(OBERON_LINUX_X_ATOM_WM_DELETE_WINDOW))
      {
        win.request_quit();
      }
      // Handle _NET_WM_PING responses.
      else if (client_message->data.data32[0] == sys.atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_PING))
      {
        // sizeof(xcb_client_message_event_t) < sizeof(xcb_generic_event_t)
        // The size of a message sent by xcb_send_event must be sizeof(xcb_generic_event). Since
        // xcb_client_message_t is smaller it is necessary to play these games with the underlying buffer.
        auto event = xcb_generic_event_t{ };
        const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(&event);
        std::memcpy(client_message, ev, sizeof(xcb_client_message_event_t));
        client_message->window = win.unique_id();
        // These parameters are required by EWMH.
        // Per https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45240719179424
        constexpr const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        xcb_send_event(sys.connection(), false, sys.default_screen()->root, mask, reinterpret_cast<cstring>(&event));
      }
    }
  }

  void platform::handle_configure_notify(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_CONFIGURE_NOTIFY);
    auto configure_notify = reinterpret_cast<ptr<xcb_configure_notify_event_t>>(ev);
    auto& win = *plt.m_window;
    auto is_move = !win.matches_current_offset({ configure_notify->x, configure_notify->y });
    auto is_resize = !win.matches_current_extent({ configure_notify->width, configure_notify->height });
    win.update_window_rect({ { configure_notify->x, configure_notify->y },
                             { configure_notify->width, configure_notify ->height } });
    if (is_move)
    {
      plt.m_window_move_event_cb(plt, { configure_notify->x, configure_notify->y });
    }
    if (is_resize)
    {
      // Explicitly dirty the renderer when a move event if detected.
      // The renderer can be dirtied in 3 places. First, at the beginning of any frame (i.e., during image
      // acquisition). Second, at the end of any frame (i.e., during image presentation). Third, in between frames at
      // any point. This nicely handles the third case by ensuring that the renderer is explicitly marked dirty
      // without requiring the client to fiddle with it.
      plt.m_graphics->dirty_renderer();
      plt.m_window_resize_event_cb(plt, { configure_notify->width, configure_notify->height });
    }
  }

  void platform::handle_ge_generic_event(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_GE_GENERIC);
    auto ge_generic = reinterpret_cast<ptr<xcb_ge_generic_event_t>>(ev);
    // XInput events are GE_GENERIC events with the XInput major opcode in the extension field.
    // Why is XKB not this way?
    // I don't know!
    if (ge_generic->extension == plt.m_system->xi_major_opcode())
    {
      plt.m_xi_event_handlers[ge_generic->event_type](plt, ge_generic->event_type, ge_generic);
    }
  }

  void platform::handle_xi_key_press_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_INPUT_KEY_PRESS);
    auto key_press = reinterpret_cast<ptr<xcb_input_key_press_event_t>>(ev);
    auto& inpt = *plt.m_input;
    auto echoing = key_press->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT;
    inpt.update_key(key_press->detail, true, echoing);
    plt.m_key_press_event_cb(plt, key_press->detail, inpt.translate_keycode(key_press->detail), echoing);
  }

  void platform::handle_xi_key_release_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_INPUT_KEY_RELEASE);
    auto key_release = reinterpret_cast<ptr<xcb_input_key_release_event_t>>(ev);
    auto& inpt = *plt.m_input;
    inpt.update_key(key_release->detail, false, false);
    plt.m_key_release_event_cb(plt, key_release->detail, inpt.translate_keycode(key_release->detail));
  }

  void platform::handle_xi_button_press_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_INPUT_BUTTON_PRESS);
    auto button_press = reinterpret_cast<ptr<xcb_input_button_press_event_t>>(ev);
    auto& inpt = *plt.m_input;
    inpt.update_mouse_button(button_press->detail, true);
    plt.m_mouse_button_press_event_cb(plt, button_press->detail, inpt.translate_buttoncode(button_press->detail));
  }

  void platform::handle_xi_button_release_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_INPUT_BUTTON_RELEASE);
    auto button_release = reinterpret_cast<ptr<xcb_input_button_release_event_t>>(ev);
    auto& inpt = *plt.m_input;
    inpt.update_mouse_button(button_release->detail, false);
    plt.m_mouse_button_release_event_cb(plt, button_release->detail,
                                        inpt.translate_buttoncode(button_release->detail));
  }

  void platform::handle_xi_motion_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_INPUT_MOTION);
    auto motion = reinterpret_cast<ptr<xcb_input_motion_event_t>>(ev);
    auto& inpt = *plt.m_input;
    // Positions are encoded as XInput 2 FP1616 (i.e., fixed point 16.16 types) with 16 bits of signed integer in the
    // high 16 bits and 16 bits of unsigned fraction in the low 16 bits. Broadly, mouse positions are in screen
    // coordinates which should not be fractional. I don't have enough hardware to actually test this in dozens of
    // configurations but there's no indication if documentation that their values ever have fractions.
    // This is defined by https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/blob/master/specs/XI2proto.txt
    constexpr const auto MASK = OBERON_LINUX_X_XI_FP1616_INT_MASK;
    auto screen_position = mouse_offset{ static_cast<i16>((motion->root_x & MASK) >> 16),
                                         static_cast<i16>((motion->root_y & MASK) >> 16) };
    auto window_position = mouse_offset{ static_cast<i16>((motion->event_x & MASK) >> 16),
                                         static_cast<i16>((motion->event_y & MASK) >> 16) };
    inpt.update_mouse_position(screen_position, window_position);
    plt.m_mouse_movement_event_cb(plt, screen_position, window_position);
  }

  void platform::handle_xkb_event(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev) {
    OBERON_PRECONDITION(type == plt.m_system->xkb_event_code());
    auto xkb_event = reinterpret_cast<ptr<xcb_xkb_generic_event_t>>(ev);
    plt.m_xkb_event_handlers[xkb_event->xkb_code](plt, xkb_event->xkb_code, xkb_event);
  }

  void platform::handle_xkb_state_notify(platform& plt, const u8 type, const ptr<xcb_xkb_generic_event_t> ev) {
    OBERON_PRECONDITION(type == XCB_XKB_STATE_NOTIFY);
    // Modifiers updates could be handled in key press/key release but this provides some minor efficiency benefits.
    // State notify events only occur when a modifier is actually changed as opposed to any time any input happens
    // at all.
    auto state_notify = reinterpret_cast<ptr<xcb_xkb_state_notify_event_t>>(ev);
    plt.m_input->update_keyboard_state(state_notify->baseMods, state_notify->latchedMods, state_notify->lockedMods,
                                       state_notify->baseGroup, state_notify->latchedGroup, state_notify->lockedGroup);
  }

  void platform::handle_xkb_new_keyboard_notify_and_map_notify(platform& plt, const u8 type,
                                                               const ptr<xcb_xkb_generic_event_t>) {
    // Both of these events are handled identically. That is, by reinitializing the keyboard state.
    OBERON_PRECONDITION(type == XCB_XKB_NEW_KEYBOARD_NOTIFY || type == XCB_XKB_MAP_NOTIFY);
    plt.m_input->reinitialize_keyboard();
  }

  platform::platform(class system& sys, class input& inpt, class window& win, class graphics& gfx) :
  m_system{ &sys }, m_input{ &inpt }, m_window{ &win }, m_graphics{ &gfx } {
    // Initialize event handlers.
    for (auto& handler : m_event_handlers)
    {
      handler = null_event_handler;
    }
    m_event_handlers[XCB_ERROR] = handle_error;
    m_event_handlers[XCB_CLIENT_MESSAGE] = handle_client_message;
    m_event_handlers[XCB_CONFIGURE_NOTIFY] = handle_configure_notify;
    // XKB uses only a single event code
    m_event_handlers[m_system->xkb_event_code()] = handle_xkb_event;
    m_event_handlers[XCB_GE_GENERIC] = handle_ge_generic_event;
    // Initialize XInput event handlers.
    for (auto& handler : m_xi_event_handlers)
    {
      handler = null_xi_event_handler;
    }
    m_xi_event_handlers[XCB_INPUT_KEY_PRESS] = handle_xi_key_press_event;
    m_xi_event_handlers[XCB_INPUT_KEY_RELEASE] = handle_xi_key_release_event;
    m_xi_event_handlers[XCB_INPUT_BUTTON_PRESS] = handle_xi_button_press_event;
    m_xi_event_handlers[XCB_INPUT_BUTTON_RELEASE] = handle_xi_button_release_event;
    m_xi_event_handlers[XCB_INPUT_MOTION] = handle_xi_motion_event;
    // Initialize XKB event handlers.
    for (auto& handler : m_xkb_event_handlers)
    {
      handler = null_xkb_event_handler;
    }
    m_xkb_event_handlers[XCB_XKB_STATE_NOTIFY] = handle_xkb_state_notify;
    m_xkb_event_handlers[XCB_XKB_NEW_KEYBOARD_NOTIFY] = handle_xkb_new_keyboard_notify_and_map_notify;
    m_xkb_event_handlers[XCB_XKB_MAP_NOTIFY] = handle_xkb_new_keyboard_notify_and_map_notify;
    // Attach null event callbacks.
    attach_key_press_event_callback(null_key_press_event_cb);
    attach_key_release_event_callback(null_key_release_event_cb);
    attach_mouse_movement_event_callback(null_mouse_movement_event_cb);
    attach_mouse_button_press_event_callback(null_mouse_button_press_event_cb);
    attach_mouse_button_release_event_callback(null_mouse_button_press_event_cb);
    attach_window_move_event_callback(null_window_move_event_cb);
    attach_window_resize_event_callback(null_window_resize_event_cb);
  }

  oberon::system& platform::system() {
    OBERON_PLATFORM_PRECONDITIONS;
    return *m_system;
  }

  oberon::input& platform::input() {
    OBERON_PLATFORM_PRECONDITIONS;
    return *m_input;
  }

  oberon::window& platform::window() {
    OBERON_PLATFORM_PRECONDITIONS;
    return *m_window;
  }

  oberon::graphics& platform::graphics() {
    OBERON_PLATFORM_PRECONDITIONS;
    return *m_graphics;
  }

  void platform::attach_key_press_event_callback(const key_press_event_fn& fn) {
    OBERON_PLATFORM_PRECONDITIONS;
    m_key_press_event_cb = fn;
  }

  void platform::detach_key_press_event_callback() {
    OBERON_PLATFORM_PRECONDITIONS;
    attach_key_press_event_callback(null_key_press_event_cb);
  }

  void platform::attach_key_release_event_callback(const key_release_event_fn& fn) {
    OBERON_PLATFORM_PRECONDITIONS;
    m_key_release_event_cb = fn;
  }

  void platform::detach_key_release_event_callback() {
    OBERON_PLATFORM_PRECONDITIONS;
    attach_key_release_event_callback(null_key_release_event_cb);
  }

  void platform::attach_mouse_movement_event_callback(const mouse_movement_event_fn& fn) {
    OBERON_PLATFORM_PRECONDITIONS;
    m_mouse_movement_event_cb = fn;
  }

  void platform::detach_mouse_movement_event_callback() {
    OBERON_PLATFORM_PRECONDITIONS;
    attach_mouse_movement_event_callback(null_mouse_movement_event_cb);
  }

  void platform::attach_mouse_button_press_event_callback(const mouse_button_press_event_fn& fn) {
    OBERON_PLATFORM_PRECONDITIONS;
    m_mouse_button_press_event_cb = fn;
  }

  void platform::detach_mouse_button_press_event_callback() {
    OBERON_PLATFORM_PRECONDITIONS;
    attach_mouse_button_press_event_callback(null_mouse_button_press_event_cb);
  }

  void platform::attach_mouse_button_release_event_callback(const mouse_button_release_event_fn& fn) {
    OBERON_PLATFORM_PRECONDITIONS;
    m_mouse_button_release_event_cb = fn;
  }

  void platform::detach_mouse_button_release_event_callback() {
    OBERON_PLATFORM_PRECONDITIONS;
    attach_mouse_button_release_event_callback(null_mouse_button_release_event_cb);
  }


  void platform::attach_window_move_event_callback(const window_move_event_fn& fn) {
    OBERON_PLATFORM_PRECONDITIONS;
    m_window_move_event_cb = fn;
  }

  void platform::detach_window_move_event_callback() {
    OBERON_PLATFORM_PRECONDITIONS;
    attach_window_move_event_callback(null_window_move_event_cb);
  }

  void platform::attach_window_resize_event_callback(const window_resize_event_fn& fn) {
    OBERON_PLATFORM_PRECONDITIONS;
    m_window_resize_event_cb = fn;
  }

  void platform::detach_window_resize_event_callback() {
    OBERON_PLATFORM_PRECONDITIONS;
    attach_window_resize_event_callback(null_window_resize_event_cb);
  }

  void platform::drain_event_queue() {
    OBERON_PLATFORM_PRECONDITIONS;
    auto ev = ptr<xcb_generic_event_t>{ };
    while ((ev = xcb_poll_for_event(m_system->connection())))
    {
      // Clear the synthetic indicator bit.
      const auto type = ev->response_type & ~event_flag_bits::synthetic_bit;
      // Dispatch event to handler.
      m_event_handlers[type](*this, type, ev);
      // Free the event.
      std::free(ev);
    }
  }

}
