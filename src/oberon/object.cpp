#include "oberon/detail/object_impl.hpp"

#include "oberon/debug.hpp"

namespace oberon {
namespace detail {

  void object_dtor::operator()(const ptr<object_impl> impl) const noexcept {
    delete impl;
  }

}

  object::object(const ptr<detail::object_impl> impl) : m_impl{ impl } {
    OBERON_ASSERT(impl);
  }

  object::object(const ptr<detail::object_impl> impl, const readonly_ptr<object> parent) :
  m_impl{ impl }, m_parent{ parent } {
    OBERON_ASSERT(impl);
    OBERON_ASSERT(parent);
  }

  void object::dispose() noexcept {
    if (!is_disposed())
    {
      v_dispose();
      m_impl.reset(nullptr); // Explicitly destroy m_impl
      m_parent = nullptr; // Erase parent pointer.
    }
  }

  bool object::is_disposed() const noexcept {
    return m_impl.get() == nullptr;
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

  const object& object::parent() const {
    OBERON_PRECONDITION(!is_disposed());
    OBERON_PRECONDITION(m_parent);
    return *m_parent;
  }

}
