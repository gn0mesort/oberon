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

#if OBERON_INVARIANTS_ENABLED
  #define OBERON_X_SUCCEEDS(target, reqexp) \
    do \
    { \
      auto err_ptr = ptr<xcb_generic_error_t>{ }; \
      auto err = &err_ptr; \
      (target) = (reqexp); \
      if (!(target)) \
      { \
        auto error_code = err_ptr->error_code; \
        std::free(err_ptr); \
        throw oberon::x11_error{ "Failed to get reply for \'" #reqexp "\'.", error_code }; \
      } \
    } \
    while (0)
#else
  #define OBERON_X_SUCCEEDS(target, reqexp) \
    do \
    { \
      auto err = ptr<ptr<xcb_generic_error_t>>{ nullptr }; \
      (target) = (reqexp); \
    } \
    while (0)
#endif

namespace oberon::detail::x_size_hint_flag_bits {
  OBERON_DEFINE_BIT(none, 0);
  OBERON_DEFINE_BIT(user_position, 1);
  OBERON_DEFINE_BIT(user_size, 2);
  OBERON_DEFINE_BIT(program_position, 3);
  OBERON_DEFINE_BIT(program_size, 4);
  OBERON_DEFINE_BIT(program_min_size, 5);
  OBERON_DEFINE_BIT(program_max_size, 6);
  OBERON_DEFINE_BIT(program_resize_increment, 7);
  OBERON_DEFINE_BIT(program_aspect, 8);
  OBERON_DEFINE_BIT(program_base_size, 9);
  OBERON_DEFINE_BIT(program_window_gravity, 10);
}

namespace oberon::detail {

#define OBERON_X_ATOM_NAME(name, str) X_ATOM_##name,
  enum x_atom {
    OBERON_X_ATOMS
    X_ATOM_MAX
  };
#undef OBERON_X_ATOM_NAME

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

#endif
