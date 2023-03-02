/**
 * @file system.cpp
 * @brief System class implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/system.hpp"

#include <cstring>
#include <cstdlib>

#include "oberon/debug.hpp"
#include "oberon/environment.hpp"

#include "oberon/linux/input.hpp"
#include "oberon/linux/window.hpp"

#define OBERON_SYSTEM_PRECONDITIONS \
  OBERON_PRECONDITION(m_x_display); \
  OBERON_PRECONDITION(m_x_connection); \
  OBERON_PRECONDITION(m_x_screen)

namespace oberon::linux {

  system::system(const std::string& instance_name, const std::string& application_name) :
  m_instance_name{ instance_name },
  m_application_name{ application_name } {
    // Initialize X11
    {
      XInitThreads();
      m_x_display = XOpenDisplay(nullptr);
      XSetEventQueueOwner(m_x_display, XCBOwnsEventQueue);
      m_x_connection = XGetXCBConnection(m_x_display);
      OBERON_CHECK(!xcb_connection_has_error(m_x_connection));
      auto screenp = XDefaultScreen(m_x_display);
      auto setup = xcb_get_setup(m_x_connection);
      for (auto roots = xcb_setup_roots_iterator(setup); roots.rem; xcb_screen_next(&roots))
      {
        if (!(screenp--))
        {
          m_x_screen = roots.data;
        }
      }
      {
        xkb_x11_setup_xkb_extension(m_x_connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                    XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, &m_xkb_first_event,
                                    &m_xkb_first_error);
        m_xkb_keyboard = xkb_x11_get_core_keyboard_device_id(m_x_connection);
        OBERON_CHECK(m_xkb_keyboard >= 0);
        // No XKB events are actually selected at this point.
        // Inexplicable code per libxkbcommon
        // (https://github.com/xkbcommon/libxkbcommon/blob/master/tools/interactive-x11.c)
        constexpr const auto required_events = XCB_XKB_EVENT_TYPE_MAP_NOTIFY | XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
                                               XCB_XKB_EVENT_TYPE_STATE_NOTIFY;
        constexpr const auto map_parts = XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
                                         XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                                         XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_VIRTUAL_MODS |
                                         XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
        constexpr const auto state_details = XCB_XKB_STATE_PART_MODIFIER_BASE | XCB_XKB_STATE_PART_MODIFIER_LATCH |
                                             XCB_XKB_STATE_PART_MODIFIER_LOCK | XCB_XKB_STATE_PART_GROUP_BASE |
                                             XCB_XKB_STATE_PART_GROUP_LATCH | XCB_XKB_STATE_PART_GROUP_LOCK;
        auto details = xcb_xkb_select_events_details_t{ };
        details.affectNewKeyboard = XCB_XKB_NKN_DETAIL_KEYCODES;
        details.newKeyboardDetails = XCB_XKB_NKN_DETAIL_KEYCODES;
        details.affectState = state_details;
        details.stateDetails = state_details;
        xcb_xkb_select_events(m_x_connection, m_xkb_keyboard, required_events, 0, 0, map_parts, map_parts,
                                  &details);
        m_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        {
          constexpr const auto mask = XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT;
          auto request = xcb_xkb_per_client_flags(m_x_connection, m_xkb_keyboard, mask, mask, 0, 0, 0);
          auto reply = ptr<xcb_xkb_per_client_flags_reply_t>{ };
          OBERON_LINUX_X_SUCCEEDS(reply, xcb_xkb_per_client_flags_reply(m_x_connection, request, err));
          OBERON_CHECK(reply->supported & mask);
          OBERON_CHECK(reply->value & mask);
          std::free(reply);
        }
      }
    }
    OBERON_ASSERT(m_x_screen);
    OBERON_ASSERT(m_x_connection);
    OBERON_ASSERT(m_x_display);
    // Intern X11 atoms
    {
#define OBERON_LINUX_X_ATOM_NAME(name, str) (str),
      auto atom_names = std::array<cstring, OBERON_LINUX_X_ATOM_MAX>{ OBERON_LINUX_X_ATOMS };
#undef OBERON_LINUX_X_ATOM_NAME
      auto atom_requests = std::array<xcb_intern_atom_cookie_t, OBERON_LINUX_X_ATOM_MAX>{ };
      {
        auto cur = atom_requests.begin();
        for (const auto& atom_name : atom_names)
        {
          *(cur++) = begin_intern_atom(atom_name);
        }
      }
      // TODO: Vulkan instance initialization
      {
        auto cur = m_x_atoms.begin();
        for (const auto& request : atom_requests)
        {
          *(cur++) = end_intern_atom(request);
        }
      }
    }
  }

  xcb_intern_atom_cookie_t system::begin_intern_atom(const cstring name) {
    OBERON_SYSTEM_PRECONDITIONS;
    OBERON_PRECONDITION(name);
    return xcb_intern_atom(m_x_connection, false, std::strlen(name), name);
  }

  xcb_atom_t system::end_intern_atom(const xcb_intern_atom_cookie_t request) {
    OBERON_SYSTEM_PRECONDITIONS;
    auto reply = ptr<xcb_intern_atom_reply_t>{ };
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_intern_atom_reply(m_x_connection, request, err));
    auto result = reply->atom;
    std::free(reply);
    return result;
  }


  system::~system() noexcept {
    OBERON_SYSTEM_PRECONDITIONS;
    xkb_context_unref(m_xkb_context);
    XCloseDisplay(m_x_display);
  }

  std::string system::instance_name() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_instance_name;
  }

  std::string system::application_name() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_application_name;
  }

  ptr<xcb_connection_t> system::connection() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_connection;
  }

  ptr<xcb_screen_t> system::default_screen() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_screen;
  }

  xcb_window_t system::root_window_id() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_screen->root;
  }

  xcb_atom_t system::atom_from_name(const x_atom_name name) const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_atoms[name];
  }

  void null_key_event_callback(oberon::environment&) { }

  void system::attach_key_event_callback(const std::function<key_event_callback>& fn) {
    OBERON_SYSTEM_PRECONDITIONS;
    m_key_event_callback = fn;
  }

  void system::detach_key_event_callback() {
    OBERON_SYSTEM_PRECONDITIONS;
    m_key_event_callback = null_key_event_callback;
  }

  void system::handle_x_error(const ptr<xcb_generic_error_t> err) {
    OBERON_SYSTEM_PRECONDITIONS;
    auto code = err->error_code;
    std::free(err);
    throw x_error{ "An error was read from the X server.", (static_cast<i32>(code) << 8 | 0x01) };
  }

  void system::handle_x_event(const u8 event_type, const ptr<xcb_generic_event_t> ev) {
    OBERON_SYSTEM_PRECONDITIONS;
    if (event_type == m_xkb_first_event)
    {
      m_attached_input->update_keyboard(ev);
    }
    switch (event_type)
    {
    case XCB_CLIENT_MESSAGE:
      {
        auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(ev);
        if (client_message->type == m_x_atoms[OBERON_LINUX_X_ATOM_WM_PROTOCOLS])
        {
          // Handle WM_DELETE_WINDOW messages.
          if (client_message->data.data32[0] == m_x_atoms[OBERON_LINUX_X_ATOM_WM_DELETE_WINDOW])
          {
            m_attached_window->request_quit();
          }
          // Handle _NET_WM_PING responses.
          else if (client_message->data.data32[0] == m_x_atoms[OBERON_LINUX_X_ATOM_NET_WM_PING])
          {
            // sizeof(xcb_client_message_event_t) < sizeof(xcb_generic_event_t)
            // The size of a message sent by xcb_send_event must be sizeof(xcb_generic_event). Since
            // xcb_client_message_t is smaller it is necessary to play these games with the underlying buffer.
            auto event = xcb_generic_event_t{ };
            const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(&event);
            std::memcpy(client_message, ev, sizeof(xcb_client_message_event_t));
            client_message->window = m_attached_window->unique_id();
            // These parameters are required by EWMH.
            // Per https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45240719179424
            constexpr const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
            xcb_send_event(m_x_connection, false, m_x_screen->root, mask, reinterpret_cast<cstring>(&event));
          }
        }
      }
      break;
    case XCB_KEY_PRESS:
      {
        auto key_press = reinterpret_cast<ptr<xcb_key_press_event_t>>(ev);
        m_attached_input->update_key(key_press->detail, true);
        auto env = environment{ *static_cast<ptr<system>>(this), *static_cast<ptr<input>>(m_attached_input),
                                *static_cast<ptr<window>>(m_attached_window) };
        m_key_event_callback(env);
      }
      break;
    case XCB_KEY_RELEASE:
      {
        auto key_release = reinterpret_cast<ptr<xcb_key_release_event_t>>(ev);
        m_attached_input->update_key(key_release->detail, false);
        auto env = environment{ *static_cast<ptr<system>>(this), *static_cast<ptr<input>>(m_attached_input),
                                *static_cast<ptr<window>>(m_attached_window) };
        m_key_event_callback(env);
      }
      break;
    default:
      break;
    }
    std::free(ev);
  }

  void system::drain_event_queue() {
    OBERON_SYSTEM_PRECONDITIONS;
    auto ev = ptr<xcb_generic_event_t>{ };
    while ((ev = xcb_poll_for_event(m_x_connection)))
    {
      const auto event_type = ev->response_type & ~event_flag_bits::synthetic_bit;
      if (event_type)
      {
        handle_x_event(event_type, ev);
      }
      else
      {
        handle_x_error(reinterpret_cast<ptr<xcb_generic_error_t>>(ev));
      }
    }
  }

  ptr<xkb_context> system::keyboard_context() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xkb_context;
  }

  xcb_xkb_device_spec_t system::keyboard() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xkb_keyboard;
  }

  u8 system::keyboard_event_code() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xkb_first_event;
  }


  void system::attach_input(input& inpt) {
    OBERON_SYSTEM_PRECONDITIONS;
    m_attached_input = &inpt;
  }

  void system::attach_window(window& win) {
    OBERON_SYSTEM_PRECONDITIONS;
    m_attached_window = &win;
  }

}
