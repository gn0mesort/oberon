#ifndef OBERON_CHILD_OBJECT_HPP
#define OBERON_CHILD_OBJECT_HPP

#include "memory.hpp"
#include "object.hpp"
#include "debug.hpp"

namespace oberon {

  class child_object : public object {
  private:
    readonly_ptr<object> m_parent{ };
  protected:
    child_object(const ptr<detail::object_impl> child_impl, const readonly_ptr<object> parent);

    template <typename Parent>
    readonly_ptr<Parent> parent_ptr() const {
      OBERON_PRECONDITION(m_parent);
      return static_cast<readonly_ptr<Parent>>(m_parent);
    }

    template <typename ParentImplementation>
    readonly_ptr<ParentImplementation> parent_q_ptr() const {
      OBERON_PRECONDITION(m_parent);
      return static_cast<readonly_ptr<ParentImplementation>>(&m_parent->implementation());
    }
  public:
    virtual ~child_object() noexcept = default;

    const object& parent() const;
  };

}

#endif
