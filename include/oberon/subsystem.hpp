#ifndef OBERON_SUBSYSTEM_HPP
#define OBERON_SUBSYSTEM_HPP

#include "types.hpp"

namespace oberon {

  enum class subsystem_type {
    io,
    graphics
  };

  enum class subsystem_implementation {
    abstract,
    // IO
    linux_xcb,
    // GRAPHICS
    vulkan
  };

  template <typename Type>
  concept subsystem = requires (Type& t) {
    std::destructible<Type>;
    !std::movable<Type>;
    !std::copyable<Type>;
    { t.type() } noexcept -> std::same_as<subsystem_type>;
    { t.implementation() } noexcept -> std::same_as<subsystem_implementation>;
  };

  class abstract_subsystem {
  protected:
    abstract_subsystem() = default;

    virtual ~abstract_subsystem() noexcept = default;
  public:
    abstract_subsystem(const abstract_subsystem& other) = delete;
    abstract_subsystem(abstract_subsystem&& other) = delete;

    abstract_subsystem& operator=(const abstract_subsystem& rhs) = delete;
    abstract_subsystem& operator=(abstract_subsystem&& rhs) = delete;

    virtual subsystem_type type() noexcept = 0;
    virtual subsystem_implementation implementation() noexcept = 0;
  };

  template <typename Type>
  struct is_subsystem final {
    static constexpr bool value{ inherits_from<Type, abstract_subsystem> && subsystem<Type> };
  };

  template <typename Type>
  constexpr bool is_subsystem_v = is_subsystem<Type>::value;

  static_assert(is_subsystem_v<abstract_subsystem>);

}

#endif
