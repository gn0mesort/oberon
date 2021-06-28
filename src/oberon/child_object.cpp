#include "oberon/child_object.hpp"

namespace oberon {

  child_object::child_object(
    const ptr<detail::object_impl> child_impl,
    const readonly_ptr<object> parent
  ) : object{ child_impl }, m_parent{ parent } {
    OBERON_ASSERT(parent);
  }

  const object& child_object::parent() const {
    OBERON_PRECONDITION(m_parent);
    return *m_parent;
  }

}
