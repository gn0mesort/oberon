#include "oberon/detail/object_impl.hpp"

#include "oberon/debug.hpp"

namespace oberon {
namespace detail {
  void object_dtor::operator()(const ptr<object_impl> impl) const noexcept {
    delete impl;
  }
}

  object::object(const ptr<detail::object_impl> child_impl) : m_impl{ child_impl } {
    OBERON_ASSERT(child_impl);
  }

  void object::dispose() noexcept {
    if (!is_disposed())
    {
      v_dispose();
      m_impl.reset(nullptr); // Explicitly destroy m_impl
    }
  }

  bool object::is_disposed() const noexcept {
    return m_impl.get() == nullptr;
  }

  ptr<detail::object_impl> object::d_ptr() {
    OBERON_PRECONDITION(!is_disposed());
    OBERON_PRECONDITION(m_impl.get());
    return m_impl.get();
  }

  readonly_ptr<detail::object_impl> object::d_ptr() const {
    OBERON_PRECONDITION(!is_disposed());
    OBERON_PRECONDITION(m_impl.get());
    return m_impl.get();
  }

  detail::object_impl& object::implementation() {
    OBERON_PRECONDITION(!is_disposed());
    OBERON_PRECONDITION(m_impl.get());
    return *m_impl;
  }

  const detail::object_impl& object::implementation() const {
    OBERON_PRECONDITION(!is_disposed());
    OBERON_PRECONDITION(m_impl.get());
    return *m_impl;
  }
}
