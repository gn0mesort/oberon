#ifndef OBERON_OBJECT_HPP
#define OBERON_OBJECT_HPP

#include "memory.hpp"

namespace oberon {
namespace detail {

  struct object_impl;
  struct object_dtor final {
    void operator()(const ptr<object_impl> impl) const noexcept;
  };

}

  class object {
  private:
    std::unique_ptr<detail::object_impl, detail::object_dtor> m_impl{ };
    readonly_ptr<object> m_parent{ };

    virtual void v_dispose() noexcept = 0;
  protected:
    object(const ptr<detail::object_impl> impl);
    object(const ptr<detail::object_impl> impl, const readonly_ptr<object> parent);
  public:
    inline virtual ~object() noexcept = 0;

    bool is_disposed() const noexcept;
    void dispose() noexcept;

    detail::object_impl& implementation();
    const detail::object_impl& implementation() const;
    const object& parent() const;
  };

  object::~object() noexcept { }

}

#endif
