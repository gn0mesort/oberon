#ifndef OBERON_INTERFACES_WINDOWING_SYSTEM_HPP
#define OBERON_INTERFACES_WINDOWING_SYSTEM_HPP

#include <concepts>

#include "../macros.hpp"

#include "../bounds.hpp"

#include "window.hpp"

namespace oberon {
OBERON_INLINE_V_0_0 namespace v0_0 {
namespace interfaces {

  template <typename Type>
  concept windowing_system = requires (Type t, bounds_2d b) {

    requires std::destructible<Type>;
    { t.create_window(b) } -> window;
    { t.destroy_window() } -> std::same_as<void>;
  };

  class windowing_system_base {
  public:
    inline virtual ~windowing_system_base() noexcept = 0;

    virtual window& create_window(const bounds_2d& bnds) const = 0;
    virtual void destroy_window(window& win) const = 0;
  };

  windowing_system_base::~windowing_system_base() noexcept { }

}
}
}

#endif
