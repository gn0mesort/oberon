/**
 * @file wsi_worker.cpp
 * @brief Internal Linux+X11 WSI worker thread implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/internal/linux/x11/wsi_worker.hpp"

#include <iostream>

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>

#include "oberon/errors.hpp"


#define SENDMSG(socket, msg, flags) \
  OBERON_CHECK_ERROR_MSG(!nng_sendmsg((socket), (msg), (flags)), 1, "Failed to send an NNG message.")

namespace oberon::internal::linux::x11 {

  ptr<void> wsi_pub_worker(ptr<void> arg) {
    const auto& params = *reinterpret_cast<ptr<wsi_pub_worker_params>>(arg);
    auto socket = nng_socket{ };
    OBERON_CHECK_ERROR_MSG(!nng_pub0_open(&socket), 1, "Failed to open an NNG socket for the WSI event worker.");
    auto listener = nng_listener{ };
    OBERON_CHECK_ERROR_MSG(!nng_listen(socket, WSI_WORKER_ENDPOINT, &listener, 0), 1, "Failed to start an NNG "
                                       "listener for the WSI event worker.");
    auto quit = false;
    auto ev = ptr<xcb_generic_event_t>{ nullptr };
    auto protocol_message = ptr<nng_msg>{ nullptr };
    while (!quit)
    {
      protocol_message = nullptr;
      try
      {
        ev = xcb_wait_for_event(params.connection);
        OBERON_CHECK_ERROR_MSG(!nng_msg_alloc(&protocol_message, sizeof(wsi_event_message)), 1,
                               "Failed to allocate NNG message body.");
        auto& message = *reinterpret_cast<ptr<wsi_event_message>>(nng_msg_body(protocol_message));
        OBERON_CHECK_ERROR_MSG(ev, 1, "Received an I/O error during a call to xcb_wait_for_event.");
        const auto type = ev->response_type & ~response_type_bits::synthetic_bit;
        switch (type)
        {
        case XCB_ERROR:
          {
            const auto error = reinterpret_cast<ptr<xcb_generic_error_t>>(ev);
            const auto code = error->error_code > 17 ? 0 : error->error_code;
            const auto major = error->major_code;
            const auto minor = error->minor_code;
            const auto resource = error->resource_id;
            // An early free is required because we will throw.
            std::free(error);
            constexpr const auto error_names = std::array<cstring, 18>{ "Unknown", "Request", "Value", "Window",
                                                                        "Pixmap", "Atom", "Cursor", "Font", "Match",
                                                                        "Drawable", "Access", "Alloc", "Colormap",
                                                                        "GContext", "IDChoice", "Name", "Length",
                                                                        "Implementation" };
            switch (code)
            {
            // Window error.
            case 3:
            // Pixmap error.
            case 4:
            // Atom error.
            case 5:
            // Cursor error.
            case 6:
            // Font error.
            case 7:
            // Drawable error.
            case 9:
            // Colormap error.
            case 12:
            // GContext error.
            case 13:
            // IDChoice error.
            case 14:
              OBERON_CHECK_ERROR_MSG(false, code, "An X11 \"%s\" error was received from the event queue. The opcode "
                                     "of the failing request was \"%hhd.%hd\". The failing resource was \"0x%08x\".",
                                     error_names[code], major, minor, resource);
              break;
            default:
              OBERON_CHECK_ERROR_MSG(false, code, "An X11 \"%s\" error was received from the event queue. The opcode "
                                     "of the failing request was \"%hhd.%hd\".", error_names[code], major, minor);
              break;
            }
          }
          break;
        case XCB_CLIENT_MESSAGE:
          {
            const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(ev);
            if (client_message->type == params.atoms[OBERON_SYSTEM_SIGNAL_ATOM] &&
                client_message->data.data32[0] == QUIT_SYSTEM_SIGNAL)
            {
              quit = true;
              nng_msg_free(protocol_message);
            }
            else if (client_message->type == params.atoms[WM_PROTOCOLS_ATOM] &&
                     client_message->data.data32[0] == params.atoms[WM_DELETE_WINDOW_ATOM])
            {

              message.window = client_message->window;
              message.data.type = event_type::window_close;
              // There's no data associated with a window close event. Just close the window.
              SENDMSG(socket, protocol_message, 0);
            }
            else if (client_message->type == params.atoms[WM_PROTOCOLS_ATOM] &&
                     client_message->data.data32[0] == params.atoms[NET_WM_PING_ATOM])
            {
              message.window = client_message->window;
              message.data.type = event_type::platform;
              *reinterpret_cast<ptr<xcb_generic_event_t>>(&message.data.data.platform.pad[0]) = *ev;
              SENDMSG(socket, protocol_message, 0);
            }
          }
          break;
        case XCB_CONFIGURE_NOTIFY:
          {
            const auto configure_notify = reinterpret_cast<ptr<xcb_configure_notify_event_t>>(ev);
            message.window = configure_notify->window;
            message.data.type = event_type::geometry_reconfigure;
            message.data.data.geometry_reconfigure.geometry.offset = { configure_notify->x, configure_notify->y };
            message.data.data.geometry_reconfigure.geometry.extent = { configure_notify->width,
                                                                       configure_notify->height };
            SENDMSG(socket, protocol_message, 0);
          }
          break;
        case XCB_GE_GENERIC:
          {
            const auto ge_generic = reinterpret_cast<ptr<xcb_ge_generic_event_t>>(ev);
            if (ge_generic->extension == params.xi_major_opcode)
            {
              switch (ge_generic->event_type)
              {
              case XCB_INPUT_KEY_PRESS:
                {
                  const auto key_press = reinterpret_cast<ptr<xcb_input_key_press_event_t>>(ge_generic);
                  message.window = key_press->event;
                  message.data.type = event_type::key_press;
                  message.data.data.key_press.key = key_press->detail;
                  message.data.data.key_press.echoing = key_press->flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT;
                  SENDMSG(socket, protocol_message, 0);
                }
                break;
              case XCB_INPUT_KEY_RELEASE:
                {
                  const auto key_release = reinterpret_cast<ptr<xcb_input_key_release_event_t>>(ge_generic);
                  message.window = key_release->event;
                  message.data.type = event_type::key_release;
                  message.data.data.key_release.key = key_release->detail;
                  SENDMSG(socket, protocol_message, 0);
                }
                break;
              case XCB_INPUT_BUTTON_PRESS:
                {
                  const auto button_press = reinterpret_cast<ptr<xcb_input_button_press_event_t>>(ge_generic);
                  message.window = button_press->event;
                  message.data.type = event_type::button_press;
                  message.data.data.button_press.button = button_press->detail;
                  SENDMSG(socket, protocol_message, 0);
                }
                break;
              case XCB_INPUT_BUTTON_RELEASE:
                {
                  const auto button_release = reinterpret_cast<ptr<xcb_input_button_release_event_t>>(ge_generic);
                  message.window = button_release->event;
                  message.data.type = event_type::button_release;
                  message.data.data.button_release.button = button_release->detail;
                  SENDMSG(socket, protocol_message, 0);
                }
                break;
              case XCB_INPUT_MOTION:
                {
                  const auto motion = reinterpret_cast<ptr<xcb_input_motion_event_t>>(ge_generic);
                  message.window = motion->event;
                  message.data.type = event_type::motion;
                  // Positions are encoded as XInput 2 FP1616 (i.e., fixed point 16.16 types) with 16 bits of signed
                  // integer in the high 16 bits and 16 bits of unsigned fraction in the low 16 bits. Broadly, mouse
                  // positions are in screen coordinates which should not be fractional. I don't have enough hardware
                  // to actually test this in dozens of configurations but there's no indication in documentation
                  // that their values ever have fractions.
                  // This is defined by
                  // https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/blob/master/specs/XI2proto.txt
                  constexpr const auto MASK = u32{ 0xffff0000 };
                  message.data.data.motion.screen_offset.x = static_cast<i16>((motion->root_x & MASK) >> 16);
                  message.data.data.motion.screen_offset.y = static_cast<i16>((motion->root_y & MASK) >> 16);
                  message.data.data.motion.window_offset.x = static_cast<i16>((motion->event_x & MASK) >> 16);
                  message.data.data.motion.window_offset.y = static_cast<i16>((motion->event_y & MASK) >> 16);
                  SENDMSG(socket, protocol_message, 0);
                }
              default:
                break;
              }
            }
          }
          break;
        default:
          {
            // Weird handling is required because XKB does not use GE_GENERIC events.
            if (type == params.xkb_first_event)
            {
              // XKB events are sent to all windows (i.e., window == params.leader).
              const auto xkb_event = reinterpret_cast<ptr<xcb_xkb_generic_event_t>>(ev);
              message.window = params.leader;
              message.data.type = event_type::platform;
              *reinterpret_cast<ptr<xcb_xkb_generic_event_t>>(&message.data.data.platform.pad[0]) = *xkb_event;
              SENDMSG(socket, protocol_message, 0);
            }
          }
          break;
        }
        std::free(ev);
      }
      catch (const error& err)
      {
        std::cerr << err.type() << " (" << err.location().file_name() << ":" << err.location().line() << ":"
                  << err.location().column() << "): " << err.message() << std::endl;
        std::cerr << "Status: " << err.result() << std::endl;
        nng_msg_free(protocol_message);
      }
    }
    // These functions can both return errors but at this point they're irrelevant.
    nng_listener_close(listener);
    nng_close(socket);
    return arg;
  }

}
