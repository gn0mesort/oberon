#ifndef OBERON_WINDOWING_SYSTEM_HPP
#define OBERON_WINDOWING_SYSTEM_HPP

#include "imaging_system.hpp"
#include "window.hpp"

namespace oberon {
inline namespace windowing_system_v01 {

  template <typename Type>
  concept windowing_system = requires {
    requires imaging_system<Type>;
    requires window<typename Type::window_type>;
  };

}
}

#endif
