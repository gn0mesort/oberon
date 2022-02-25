#ifndef OBERON_INTERFACES_WINDOW_HPP
#define OBERON_INTERFACES_WINDOW_HPP

#include <concepts>

#include "../macros.hpp"

namespace oberon {
OBERON_INLINE_V_0_0 namespace v0_0 {
namespace interfaces {

  template <typename Type>
  concept window = std::destructible<Type>;

  class window_base {
  public:
    inline virtual ~window_base() noexcept = 0;
  };

  window_base::~window_base() noexcept { }

}
}
}

#endif
