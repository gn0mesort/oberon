#include "oberon/window.hpp"

#include "oberon/debug.hpp"

#include "oberon/detail/window_impl.hpp"

namespace oberon {
  window::window(context& ctx, const std::string_view title, const bounding_rect& bounds) :
  m_impl{ new detail::window_impl{ ctx, title, bounds } } { }


  void window::show() {
    m_impl->show();
  }

  void window::hide() {
    m_impl->hide();
  }
}
