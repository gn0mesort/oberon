#include "oberon/detail/graphics.hpp"

namespace oberon {
namespace detail {
  // This is basically the canon implementation from
  // https://www.x.org/releases/X11R7.7/doc/libxcb/tutorial/index.html#screenofdisplay
  ptr<xcb_screen_t> screen_of_display(const ptr<xcb_connection_t> connection, int screen) {
    auto itr = xcb_setup_roots_iterator(xcb_get_setup(connection));
    while (itr.rem)
    {
      if (!screen--)
      {
        return itr.data;
      }
      xcb_screen_next(&itr);
    }
    return nullptr;
  }
}
}
