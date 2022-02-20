#include "oberon/detail/x_server_connection.hpp"

#include "oberon/debug.hpp"

namespace oberon {
namespace detail {

  x_server_connection::x_server_connection(const utf8_cstring display) {
    auto default_screen = int{ 0 };
    m_connection = xcb_connect(reinterpret_cast<cstring>(display), &default_screen);
    if (xcb_connection_has_error(m_connection))
    {
      // TODO throw
    }
    // Select screen of display.
    {
      auto itr = xcb_setup_roots_iterator(xcb_get_setup(m_connection));
      while (itr.rem)
      {
        if (!default_screen--)
        {
          m_screen = itr.data;
        }
        xcb_screen_next(&itr);
      }
    }
    if (!m_screen)
    {
      // TODO throw
    }
  }

  x_server_connection::~x_server_connection() noexcept {
    xcb_disconnect(m_connection);
  }
}
}
