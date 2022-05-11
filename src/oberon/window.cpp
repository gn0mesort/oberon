#include "oberon/window.hpp"

#include "oberon/debug.hpp"

#include "oberon/detail/window_impl.hpp"

namespace oberon {

  window::window(context& ctx, const std::string_view title, const bounding_rect& bounds) :
  m_impl{ new detail::window_impl{ ctx, title, bounds } } { }

  bool window::is_destroy_signaled() const {
    return m_impl->get_signals() & window_signal_bits::destroy_bit;
  }

  void window::clear_destroy_signal() {
    m_impl->clear_signals(window_signal_bits::destroy_bit);
  }

  bool window::is_shown() const {
    return m_impl->get_flags() & window_flag_bits::shown_bit;
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
