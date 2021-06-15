#ifndef OBERON_OBJECT_HPP
#define OBERON_OBJECT_HPP

#include <memory>
/*
#if __has_include(<propagate_const>)
  #include <propagate_const>
#elif __has_include(<experimental/propagate_const>)
  #include <experimental/propagate_const>
  namespace std {
    template <typename Type>
    using propagate_const = std::experimental::propagate_const<Type>;
  }
#else
  #error objects require <propagate_const>
#endif
*/

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

    virtual void v_dispose() noexcept = 0;
  protected:
    object(const ptr<detail::object_impl> child_impl);

    void dispose() noexcept;

    ptr<detail::object_impl> d_ptr();
    readonly_ptr<detail::object_impl> d_ptr() const;

    template <typename Child>
    ptr<Child> q_ptr() {
      return static_cast<ptr<Child>>(d_ptr());
    }

    template <typename Child>
    readonly_ptr<Child> q_ptr() const {
      return static_cast<readonly_ptr<Child>>(d_ptr());
    }
  public:
    inline virtual ~object() noexcept = 0;

    bool is_disposed() const noexcept;

    detail::object_impl& implementation();
    const detail::object_impl& implementation() const;
  };

  object::~object() noexcept { }
}

#endif
