#include "oberon/detail/object_impl.hpp"

#include "oberon/debug.hpp"

namespace oberon {
namespace detail {

  void object_dtor::operator()(const ptr<object_impl> impl) const noexcept {
    delete impl;
  }

}

  bool object::v_is_disposed() const noexcept {
    return !m_impl;
  }

  void object::v_set_disposed() noexcept {
    OBERON_PRECONDITION(m_impl.get());
    m_impl.reset(nullptr);
  }


  object::object(const ptr<detail::object_impl> impl) : m_impl{ impl } {
    OBERON_ASSERT(impl);
  }

  detail::object_impl& object::implementation() {
    OBERON_PRECONDITION(!is_disposed());
    return *m_impl;
  }

  const detail::object_impl& object::implementation() const {
    OBERON_PRECONDITION(!is_disposed());
    return *m_impl;
  }


}
