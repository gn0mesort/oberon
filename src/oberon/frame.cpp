#include "oberon/frame.hpp"

#include "oberon/errors.hpp"

#include "oberon/internal/base/frame_impl.hpp"

namespace oberon {

  frame::frame(internal::base::frame_impl& impl, internal::base::frame_info& info) :
  m_impl{ &impl },
  m_info{ &info } { }

  frame::implementation_type& frame::implementation() {
    return *m_impl;
  }

  frame::information_type& frame::information() {
    return *m_info;
  }

  bool frame::is_retired() const {
    return !m_impl && !m_info;
  }

  void frame::draw_test_image() {
    m_impl->draw_test_image();
  }

  void frame::draw(camera& c, mesh& m) {
    m_impl->draw(c, m);
  }

}
