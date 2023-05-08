#ifndef OBERON_INTERNAL_LINUX_X11_XCB_HPP
#define OBERON_INTERNAL_LINUX_X11_XCB_HPP

#include <cstdlib>

#include <xcb/xcb.h>
#include <xcb/xinput.h>
// Don't use C++ keywords in C headers please!
// Clang emits a warning for defining a macro over a keyword.
// GCC doesn't currently do this. At least not by default.
#ifdef USING_CLANG
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define explicit xcb_xkb_explicit
#include <xcb/xkb.h>
#undef explicit
#ifdef USING_CLANG
  #pragma clang diagnostic pop
#endif
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include "../../../errors.hpp"
#include "../../../memory.hpp"

#define OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST(request, connection, ...) \
  (request((connection) __VA_OPT__(, __VA_ARGS__)))
#define OBERON_INTERNAL_LINUX_X11_XCB_AWAIT_REPLY(request, connection, cookie, error) \
  (request##_reply((connection), (cookie), (error)))
#define OBERON_INTERNAL_LINUX_X11_XCB_HANDLE_ERROR(reply, error, msg) \
  do \
  { \
    if (!(reply)) \
    { \
      auto error_code = (error)->error_code; \
      std::free((error)); \
      throw oberon::check_failed_error{ (msg), error_code }; \
    } \
  } \
  while (0)
#define OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST_SYNC(reply, request, connection, ...) \
  do \
  { \
    auto cookie = OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST(request, connection __VA_OPT__(, __VA_ARGS__)); \
    auto error = oberon::ptr<xcb_generic_error_t>{ }; \
    (reply) = OBERON_INTERNAL_LINUX_X11_XCB_AWAIT_REPLY(request, connection, cookie, &error); \
    OBERON_INTERNAL_LINUX_X11_XCB_HANDLE_ERROR(reply, error, "Failed to get reply for \"" #request "\"."); \
  } \
  while (0)

#define XCB_ERROR 0

/**
 * @brief A generic XKB event.
 * @details Neither libxcb nor libxcb-xkb provides this event type. Therefore it is provided here.
 * @see https://www.x.org/releases/current/doc/kbproto/xkbproto.html#appD::Events
 */
struct xcb_xkb_generic_event_t final {
  /**
   * @brief The event type code.
   * @details This is equivalent to xcb_generic_event_t::response_type.
   */
  oberon::u8 code{ };

  /**
   * @brief The XKB event code.
   * @details When enabled, the XKB extension provides a base event code. The XKB protocol uses this code for all of
   *          its events. To further differentiate, every XKB event also carries this XKB code value that can be
   *          used to identify the specific type of XKB event.
   */
  oberon::u8 xkb_code{ };

  /**
   * @brief Padding values.
   */
  oberon::u8 pad[30];
};

/**
 * @brief A structure representing a list of XInput 2 event mask entries.
 * @details This is not defined by libxcb_xinput.
 */
struct xcb_input_xi_event_mask_list_t {
  /**
   * @brief The head of the event mask list.
   */
  xcb_input_event_mask_t head{ };

  /**
   * @brief The event mask.
   */
  xcb_input_xi_event_mask_t mask{ };
};

/**
 * @brief A WM_SIZE_HINTS structure compatible with the X11 protocol.
 * @details libxcb does not provide this type. Therefore, it is provided here.
 *          Additionally, when flags & (size_hints_flag_bits::min_size | size_hints_flag_bits::max_size) is not 0
 *          and (min_width, min_height) is equal to (max_width, max_height) the window is not resizable.
 * @see https://www.x.org/releases/current/doc/xorg-docs/icccm/icccm.html#WM_NORMAL_HINTS_Property
 */
struct xcb_size_hints_t final {
  /**
   * @brief A set of size_hints_flag_bits values indicating to the window manager how to interpret the
   *        WM_NORMAL_HINTS value.
   */
  oberon::u32 flags{ };

  /**
   * @brief Padding values.
   */
  oberon::u32 pad[4]{ };

  /**
   * @brief The minimum width of the window.
   */
  oberon::i32 min_width{ };

  /**
   * @brief The minimum height of the window.
   */
  oberon::i32 min_height{ };

  /**
   * @brief The maximum width of the window.
   */
  oberon::i32 max_width{ };

  /**
   * @brief The maximum height of the window.
   */
  oberon::i32 max_height{ };

  /**
   * @brief The increment by which the window should grow along the X-axis.
   */
  oberon::i32 width_inc{ };

  /**
   * @brief The increment by which the window should grow along the Y-axis.
   */
  oberon::i32 height_inc{ };

  /**
   * @brief The minimum X-axis aspect.
   */
  oberon::i32 min_aspect_x{ };

  /**
   * @brief The minimum Y-axis aspect.
   */
  oberon::i32 min_aspect_y{ };

  /**
   * @brief The maximum X-axis aspect.
   */
  oberon::i32 max_aspect_x{ };

  /**
   * @brief The maximum Y-axis aspect.
   */
  oberon::i32 max_aspect_y{ };

  /**
   * @brief The base width of the window.
   */
  oberon::i32 base_width{ };

  /**
   * @brief The base height of the window.
   */
  oberon::i32 base_height{ };

  /**
   * @brief The X11 window gravity assigned to the window.
   */
  oberon::i32 win_gravity{ };
};

struct xcb_hints_t final {
  oberon::u32 flags{ };
  oberon::u32 input{ };
  oberon::u32 initial_state{ };
  xcb_pixmap_t icon_pixmap{ };
  xcb_window_t icon_window{ };
  oberon::i32 icon_x{ };
  oberon::i32 icon_y{ };
  xcb_pixmap_t icon_mask{ };
  xcb_window_t window_group{ };
};

struct xcb_wm_state_t final {
  oberon::u32 state{ };
  xcb_window_t icon{ };
};

namespace oberon::internal::linux::x11 {

  enum source_indication : u32 {
    UNSUPPORTED_SOURCE = 0,
    APPLICATION_SOURCE = 1,
    PAGER_SOURCE = 2
  };

  enum ewmh_state_action : u32 {
    REMOVE_WM_STATE_ACTION = 0,
    ADD_WM_STATE_ACTION = 1,
    TOGGLE_WM_STATE_ACTION = 2
  };

  enum compositor_mode : u32 {
    NO_PREFERENCE_COMPOSITOR = 0,
    DISABLE_COMPOSITOR = 1,
    ENABLE_COMPOSITOR = 2
  };

  enum window_state : u32 {
    WITHDRAWN_STATE = 0,
    NORMAL_STATE = 1,
    ICONIC_STATE = 3
  };

namespace hint_flag_bits {
  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(input_hint, 0);
  OBERON_DEFINE_BIT(state_hint, 1);
  OBERON_DEFINE_BIT(icon_pixmap_hint, 2);
  OBERON_DEFINE_BIT(icon_window_hint, 3);
  OBERON_DEFINE_BIT(icon_position_hint, 4);
  OBERON_DEFINE_BIT(icon_mask_hint, 5);
  OBERON_DEFINE_BIT(window_group_hint, 6);
  OBERON_DEFINE_BIT(message_hint, 7);
  OBERON_DEFINE_BIT(urgency_hint, 8);

}

// These are defined by https://www.x.org/releases/current/doc/xorg-docs/icccm/icccm.html#WM_NORMAL_HINTS_Property
namespace size_hint_flag_bits {

  /**
   * @brief A constant representing no size hint flag bits.
   */
  OBERON_DEFINE_ZERO_BIT(none);

  /**
   * @brief A constant indicating that the window position is being set by the user.
   */
  OBERON_DEFINE_BIT(user_position, 0);

  /**
   * @brief A constant indicating that the window size is being set by the user.
   */
  OBERON_DEFINE_BIT(user_size, 1);

  /**
   * @brief A constant indicating that the window position is being set by the program.
   */
  OBERON_DEFINE_BIT(program_position, 2);

  /**
   * @brief A constant indicating that the window size is being set by the program.
   */
  OBERON_DEFINE_BIT(program_size, 3);

  /**
   * @brief A constant indicating that the window size hints contain a program selected minimum size for the window.
   */
  OBERON_DEFINE_BIT(program_min_size, 4);

  /**
   * @brief A constant indicating that the window size hints contain a maximum size for the window.
   */
  OBERON_DEFINE_BIT(program_max_size, 5);

  /**
   * @brief A constant indicating that the window size hints contain a resize increment.
   */
  OBERON_DEFINE_BIT(program_resize_increment, 6);

  /**
   * @brief A constant indicating that the window size hints contain an aspect ratio.
   */
  OBERON_DEFINE_BIT(program_aspect, 7);

  /**
   * @brief A constant indicating that the window size hints contain a base size.
   */
  OBERON_DEFINE_BIT(program_base_size, 8);

  /**
   * @brief A constant indicating that the window size hints contain a window gravity setting.
   */
  OBERON_DEFINE_BIT(program_window_gravity, 9);

}

namespace response_type_bits {

  OBERON_DEFINE_ZERO_BIT(none);

  /**
   * @brief A constant indicating that an event is synthetic.
   * @details A large amount of XCB documentation indicates that, during event polling, the value of
   *          xcb_generic_event_t::response_type must have the 7th bit (128s place) cleared. The reason for this is
   *          that valid X11 events range from 0 (error) to 127. The 7th bit, instead of indicating a further 128
   *          possible values, incidates whether or not the event is "synthetic". A synthetic event is an event
   *          generated by an X11 SendEvent request as opposed to events generated by the server in the usual course
   *          of operation. Therefore, the proper usage of xcb_generic_event_t::response_type is to clear that
   *          bit before interpretting the response_type. This can be achieved by applying the bitwise-and operation
   *          to ~0x80 (i.e., ~128). To make code clearer, this constant is defined so that you may instead write
   *          ~response_type_bits::synthetic.
   * @see https://www.x.org/releases/current/doc/xproto/x11protocol.html#event_format
   */
  OBERON_DEFINE_BIT(synthetic, 7);

}

  constexpr const usize MAX_EVENT{ 128 };
  constexpr const usize MAX_XI_EVENT{ 33 };
  constexpr const usize MAX_XKB_EVENT{ 12 };
}

#endif
