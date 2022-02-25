#ifndef OBERON_INTERFACES_RENDERING_SYSTEM_HPP
#define OBERON_INTERFACES_RENDERING_SYSTEM_HPP

#include "../macros.hpp"

namespace oberon {
OBERON_INLINE_V_0_0 namespace v0_0 {
namespace interfaces {
  class window;
  class renderer;

  class rendering_system {
  public:
    inline virtual ~rendering_system() noexcept = 0;

    virtual renderer& create_surface_renderer(const window& win) const = 0;
    virtual renderer& create_image_renderer() = 0;
    virtual void destroy_renderer(renderer& rndr) = 0;
  };


  rendering_system::~rendering_system() noexcept { }

}
}
}

#endif
