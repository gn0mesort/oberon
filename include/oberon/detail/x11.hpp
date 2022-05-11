#ifndef OBERON_DETAIL_X11_HPP
#define OBERON_DETAIL_X11_HPP

#include <array>

#include <xcb/xcb.h>
//#include <xcb/xcb_atom.h>
//#include <xcb/xcb_aux.h>
//#include <xcb/xcb_util.h>
//#include <xcb/xcb_icccm.h>
//#include <xcb/xcb_ewmh.h>

// If vulkan.hpp (from oberon not the Khronos Vulkan-Hpp library) is included before this header then add
// vulkan_xcb.h.
#ifdef OBERON_DETAIL_VULKAN_HPP
  #include <vulkan/vulkan_xcb.h>
#endif

#include "../basics.hpp"

#define OBERON_X_ATOM(name) OBERON_X_ATOM_NAME(name, #name)

#define OBERON_X_ATOMS \
  OBERON_X_ATOM(WM_NAME) \
  OBERON_X_ATOM(WM_PROTOCOLS) \
  OBERON_X_ATOM(WM_DELETE_WINDOW) \
  OBERON_X_ATOM(WM_TAKE_FOCUS) \
  OBERON_X_ATOM(WM_NORMAL_HINTS) \
  OBERON_X_ATOM(WM_CLIENT_MACHINE) \
  OBERON_X_ATOM_NAME(NET_WM_PID, "_NET_WM_PID") \
  OBERON_X_ATOM_NAME(NET_WM_NAME, "_NET_WM_NAME") \
  OBERON_X_ATOM_NAME(NET_WM_PING, "_NET_WM_PING") \
  OBERON_X_ATOM_NAME(NET_WM_SYNC_REQUEST, "_NET_WM_SYNC_REQUEST") \
  OBERON_X_ATOM(UTF8_STRING)

namespace oberon::detail {

#define OBERON_X_ATOM_NAME(name, str) X_ATOM_##name,
  enum x_atom {
    OBERON_X_ATOMS
    X_ATOM_MAX
  };
#undef OBERON_X_ATOM_NAME

  enum x_size_hint_flags {
    X_SIZE_HINT_NONE = 0,
    X_SIZE_HINT_USER_POSITION = 0x1,
    X_SIZE_HINT_USER_SIZE = 0x2,
    X_SIZE_HINT_PROGRAM_POSITION = 0x4,
    X_SIZE_HINT_PROGRAM_SIZE = 0x8,
    X_SIZE_HINT_PROGRAM_MIN_SIZE = 0x10,
    X_SIZE_HINT_PROGRAM_MAX_SIZE = 0x20,
    X_SIZE_HINT_PROGRAM_RESIZE_INCREMENT = 0x40,
    X_SIZE_HINT_PROGRAM_ASPECT = 0x60,
    X_SIZE_HINT_PROGRAM_BASE_SIZE = 0x80,
    X_SIZE_HINT_PROGRAM_WINDOW_GRAVITY = 0x100
  };

  struct x_size_hints final {
    u32 flags{ };
    u32 pad[4]{ };
    i32 min_width{ };
    i32 min_height{ };
    i32 max_width{ };
    i32 max_height{ };
    i32 width_inc{ };
    i32 height_inc{ };
    i32 min_aspect_x{ };
    i32 min_aspect_y{ };
    i32 max_aspect_x{ };
    i32 max_aspect_y{ };
    i32 base_width{ };
    i32 base_height{ };
    i32 win_gravity{ };
  };

  struct x_generic_event final {
    std::array<char, 32> data{ };
  };

}

namespace oberon::errors {

  OBERON_STATIC_EXCEPTION_TYPE(x_connection_failed, "Failed to connect to X11 server.", 1);
  OBERON_DYNAMIC_EXCEPTION_TYPE(x_generic);

}

#endif
