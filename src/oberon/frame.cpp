/**
 * @file frame.cpp
 * @brief Render frame object implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/frame.hpp"

#include "oberon/errors.hpp"

#include "oberon/internal/base/frame_impl.hpp"

namespace oberon {

  frame::frame(internal::base::frame_impl& impl) : m_impl{ &impl } { }

  frame::implementation_type& frame::implementation() {
    OBERON_CHECK_ERROR_MSG(!is_retired(), 1, "Cannot retrieve the implementation of a frame that has already been "
                                              "retired.");
    return *m_impl;
  }

  bool frame::is_retired() const {
    return !m_impl;
  }

  void frame::draw_test_image() {
    if (is_retired())
    {
      return;
    }
    m_impl->draw_test_image();
  }

  void frame::draw(camera& c, mesh& m) {
    if (is_retired())
    {
      return;
    }
    m_impl->draw(c, m);
  }

}
