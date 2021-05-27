#ifndef OBERON_DETAIL_GRAPHICS_HPP
#define OBERON_DETAIL_GRAPHICS_HPP

#include <xcb/xcb.h>

// clangd doesn't like it if I omit this.
#define VK_USE_PLATFORM_XCB_KHR 1
#include <vulkan/vulkan.hpp>

#include "../memory.hpp"

namespace oberon {
namespace detail {
  ptr<xcb_screen_t> screen_of_display(const ptr<xcb_connection_t>, const int screen);
}
}

#endif
