/**
 * @file environment.cpp
 * @brief Linux implementation of the enviroment class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/environment.hpp"

#include <cstring>

#include "oberon/debug.hpp"

#include "oberon/linux/system.hpp"
#include "oberon/linux/input.hpp"
#include "oberon/linux/window.hpp"

#define OBERON_ENVIRONMENT_PRECONDITIONS \
  OBERON_PRECONDITION(m_system); \
  OBERON_PRECONDITION(m_input); \
  OBERON_PRECONDITION(m_window)

namespace {

  void null_key_event_cb(oberon::environment&) { }

}

namespace oberon::linux {

  environment::environment(class system& sys, class input& inpt, class window& win) :
  m_system{ &sys }, m_input{ &inpt }, m_window{ &win } {
    attach_key_event_callback(null_key_event_cb);
  }

  oberon::system& environment::system() {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    return *m_system;
  }

  oberon::input& environment::input() {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    return *m_input;
  }

  oberon::window& environment::window() {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    return static_cast<oberon::window&>(*m_window);
  }

  void environment::attach_key_event_callback(const std::function<key_event_callback>& fn) {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    m_key_event_cb = fn;
  }

  void environment::detach_key_event_callback() {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    attach_key_event_callback(null_key_event_cb);
  }

  void environment::handle_x_error(const ptr<xcb_generic_error_t> err) {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    auto code = err->error_code;
    std::free(err);
    throw x_error{ "An error was read from the X server.", (static_cast<i32>(code) << 8 | 0x01) };
  }

  void environment::handle_x_event(const u8 event_type, const ptr<xcb_generic_event_t> ev) {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    if (event_type == m_system->keyboard_event_code())
    {
      m_input->update_keyboard(ev);
    }
    switch (event_type)
    {
    case XCB_CLIENT_MESSAGE:
      {
        auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(ev);
        if (client_message->type == m_system->atom_from_name(OBERON_LINUX_X_ATOM_WM_PROTOCOLS))
        {
          // Handle WM_DELETE_WINDOW messages.
          if (client_message->data.data32[0] == m_system->atom_from_name(OBERON_LINUX_X_ATOM_WM_DELETE_WINDOW))
          {
            m_window->request_quit();
          }
          // Handle _NET_WM_PING responses.
          else if (client_message->data.data32[0] == m_system->atom_from_name(OBERON_LINUX_X_ATOM_NET_WM_PING))
          {
            // sizeof(xcb_client_message_event_t) < sizeof(xcb_generic_event_t)
            // The size of a message sent by xcb_send_event must be sizeof(xcb_generic_event). Since
            // xcb_client_message_t is smaller it is necessary to play these games with the underlying buffer.
            auto event = xcb_generic_event_t{ };
            const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(&event);
            std::memcpy(client_message, ev, sizeof(xcb_client_message_event_t));
            client_message->window = m_window->unique_id();
            // These parameters are required by EWMH.
            // Per https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm45240719179424
            constexpr const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
            xcb_send_event(m_system->connection(), false, m_system->default_screen()->root, mask,
                           reinterpret_cast<cstring>(&event));
          }
        }
      }
      break;
    case XCB_KEY_PRESS:
      {
        auto key_press = reinterpret_cast<ptr<xcb_key_press_event_t>>(ev);
        m_input->update_key(key_press->detail, true);
        m_key_event_cb(*this);
      }
      break;
    case XCB_KEY_RELEASE:
      {
        auto key_release = reinterpret_cast<ptr<xcb_key_release_event_t>>(ev);
        m_input->update_key(key_release->detail, false);
        m_key_event_cb(*this);
      }
      break;
    default:
      break;
    }
    std::free(ev);
  }
  void environment::drain_event_queue() {
    OBERON_ENVIRONMENT_PRECONDITIONS;
    auto ev = ptr<xcb_generic_event_t>{ };
    while ((ev = xcb_poll_for_event(m_system->connection())))
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

}
