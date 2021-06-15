#ifndef OBERON_DETAIL_X11_HPP
#define OBERON_DETAIL_X11_HPP

#include <xcb/xcb.h>

namespace oberon {
namespace detail {
  xcb_screen_t* screen_of_display(xcb_connection_t *const connection, int screen);
}
}

#endif
