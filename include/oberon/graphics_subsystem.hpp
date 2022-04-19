#ifndef OBERON_GRAPHICS_SUBSYSTEM_HPP
#define OBERON_GRAPHICS_SUBSYSTEM_HPP

#include "subsystem.hpp"

namespace oberon {

  class abstract_graphics_subsystem : public abstract_subsystem {
  protected:
    abstract_graphics_subsystem() = default;

    virtual ~abstract_graphics_subsystem() noexcept = default;
  public:
    constexpr subsystem_type type() noexcept final override { return subsystem_type::graphics; }
  };

  template <typename Type>
  struct is_graphics_subsystem final {
    static constexpr bool value{ inherits_from<Type, abstract_graphics_subsystem> && is_subsystem_v<Type> };
  };

  template <typename Type>
  constexpr bool is_graphics_subsystem_v = is_graphics_subsystem<Type>::value;

  static_assert(is_graphics_subsystem_v<abstract_graphics_subsystem>);

}

#endif
