#ifndef OBERON_X11_HPP
#define OBERON_X11_HPP

#include <xcb/xcb.h>
//#include <xcb/xcb_atom.h>
//#include <xcb/xcb_aux.h>
//#include <xcb/xcb_util.h>
//#include <xcb/xcb_icccm.h>
//#include <xcb/xcb_ewmh.h>

// If vulkan.hpp (from oberon not the Khronos Vulkan-Hpp library) is included before this header then add
// vulkan_xcb.h.
#ifdef OBERON_VULKAN_HPP
  #include <vulkan/vulkan_xcb.h>
#endif

#include "basics.hpp"

#define OBERON_X_ATOMS \
  OBERON_X_ATOM(WM_NAME) \
  OBERON_X_ATOM(WM_PROTOCOLS) \
  OBERON_X_ATOM(WM_DELETE_WINDOW) \
  OBERON_X_ATOM(WM_TAKE_FOCUS) \
  OBERON_X_ATOM(WM_NORMAL_HINTS) \
  OBERON_X_ATOM(WM_CLIENT_MACHINE) \
  OBERON_X_ATOM(_NET_WM_PID) \
  OBERON_X_ATOM(_NET_WM_NAME) \
  OBERON_X_ATOM(_NET_WM_PING) \
  OBERON_X_ATOM(_NET_WM_SYNC_REQUEST) \
  OBERON_X_ATOM(UTF8_STRING)

namespace oberon {

#define OBERON_X_ATOM(name) X_ATOM_##name,
  enum x_atom {
    OBERON_X_ATOMS
    X_ATOM_MAX
  };
#undef OBERON_X_ATOM

  OBERON_STATIC_EXCEPTION_TYPE(x_connection_failed, "Failed to connect to X11 server.", 1);
  OBERON_DYNAMIC_EXCEPTION_TYPE(x_generic);

}

#endif
