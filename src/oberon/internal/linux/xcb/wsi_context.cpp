#include "oberon/internal/linux/xcb/wsi_context.hpp"

#include "oberon/errors.hpp"

namespace oberon::internal {

  wsi_context::wsi_context() : wsi_context{ nullptr } { }

  wsi_context::wsi_context(const cstring displayname) {
    auto screenp = int{ };
    m_connection = xcb_connect(displayname, &screenp);
    OBERON_CHECK(!xcb_connection_has_error(m_connection));
    m_setup = xcb_get_setup(m_connection);
    for (auto roots = xcb_setup_roots_iterator(m_setup); roots.rem; xcb_screen_next(&roots))
    {
      if (!(screenp--))
      {
        m_default_screen = roots.data;
      }
    }
  }

  wsi_context::~wsi_context() noexcept {
    xcb_disconnect(m_connection);
  }

  ptr<xcb_connection_t> wsi_context::connection() {
    return m_connection;
  }

  ptr<xcb_screen_t> wsi_context::default_screen() {
    return m_default_screen;
  }

}
