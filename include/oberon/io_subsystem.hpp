#ifndef OBERON_IO_SUBSYSTEM_HPP
#define OBERON_IO_SUBSYSTEM_HPP

#include "subsystem.hpp"

namespace oberon {

  class abstract_io_subsystem : public abstract_subsystem {
  protected:
    abstract_io_subsystem() = default;

    virtual ~abstract_io_subsystem() noexcept = default;
  public:
    constexpr subsystem_type type() noexcept override final { return subsystem_type::io; }
  };

  template <typename Type>
  struct is_io_subsystem final {
    static constexpr bool value{ inherits_from<Type, abstract_io_subsystem> && is_subsystem_v<Type> };
  };

  template <typename Type>
  constexpr bool is_io_subsystem_v = is_io_subsystem<Type>::value;

  static_assert(is_io_subsystem_v<abstract_io_subsystem>);

}

#endif
