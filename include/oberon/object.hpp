#ifndef OBERON_OBJECT_HPP
#define OBERON_OBJECT_HPP

#include "memory.hpp"
#include "interfaces/disposable.hpp"

namespace oberon {
namespace detail {

  struct object_impl;
  struct object_dtor final {
    void operator()(const ptr<object_impl> impl) const noexcept;
  };

}

  class object : public interfaces::disposable {
  private:
    std::unique_ptr<detail::object_impl, detail::object_dtor> m_impl{ };

    bool v_is_disposed() const noexcept override final;
    void v_set_disposed() noexcept override final;
  protected:
    object(const ptr<detail::object_impl> impl);
  public:
    inline virtual ~object() noexcept = 0;

    const detail::object_impl& implementation() const;
    detail::object_impl& implementation();
  };

  object::~object() noexcept { }

}

#endif
