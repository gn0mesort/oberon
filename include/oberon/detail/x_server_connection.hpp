#ifndef OBERON_DETAIL_X_SERVER_CONNECTION_HPP
#define OBERON_DETAIL_X_SERVER_CONNECTION_HPP

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "../types.hpp"
#include "../memory.hpp"

namespace oberon {
namespace detail {

  class x_server_connection final {
  private:
    ptr<xcb_connection_t> m_connection{ nullptr };
    ptr<xcb_screen_t> m_screen{ nullptr };
  public:
    x_server_connection(const utf8_cstring display);
    x_server_connection(x_server_connection&& other) = default;

    ~x_server_connection() noexcept;

    x_server_connection& operator=(x_server_connection&& rhs) = default;

  };

}
}

#endif
