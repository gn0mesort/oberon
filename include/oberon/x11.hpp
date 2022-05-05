#ifndef OBERON_LINUX_X11_HPP
#define OBERON_LINUX_X11_HPP

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>

#include "oberon/types.hpp"
#include "oberon/errors.hpp"

namespace oberon {

  OBERON_STATIC_EXCEPTION_TYPE(x_connection_failed, "Failed to connect to X11 server.", 1);
  OBERON_DYNAMIC_EXCEPTION_TYPE(x_generic);

}

#endif
