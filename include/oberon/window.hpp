#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include "basics.hpp"
#include "x11_vulkan.hpp"

namespace oberon {

  struct bounding_rect;

  class window final {
  private:
    xcb_window_t m_window_id{ };

    VkSurfaceKHR m_surface{ };
  public:
  };

}

#endif
