#include "oberon/window.hpp"

#include "oberon/debug.hpp"

#include "oberon/detail/window_impl.hpp"

namespace oberon {

  window::window(context& ctx, const std::string_view title, const bounding_rect& bounds) :
  m_impl{ new detail::window_impl{ ctx, title, bounds } } { }

  bitmask window::get_signals() const {
    return m_impl->get_signals();
  }

  void window::clear_signals(const bitmask signals) {
    return m_impl->clear_signals(signals);
  }

  bitmask window::get_flags() const {
    return m_impl->get_flags();
  }

  void window::show() {
    m_impl->show();
  }

  void window::hide() {
    m_impl->hide();
  }

  void window::accept_message(const ptr<void> message) {
    m_impl->accept_message(message);
  }

}
