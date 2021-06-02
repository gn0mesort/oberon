#include "oberon/detail/object_impl.hpp"

namespace oberon {
namespace detail {
  void object_dtor::operator()(const ptr<object_impl> impl) const noexcept {
    delete impl;
  }
}

  object::object(const ptr<detail::object_impl> child_impl) : m_impl{ child_impl } { }

  ptr<detail::object_impl> object::d_ptr() {
    return m_impl.get();
  }

  readonly_ptr<detail::object_impl> object::d_ptr() const {
    return m_impl.get();
  }

  detail::object_impl& object::implementation() {
    return *m_impl;
  }

  const detail::object_impl& object::implementation() const {
    return *m_impl;
  }
}
