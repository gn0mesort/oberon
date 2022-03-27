#ifndef OBERON_LINUX_SYSTEM_IMPL_HPP
#define OBERON_LINUX_SYSTEM_IMPL_HPP

#include <array>

#include "../../system.hpp"

namespace oberon::detail {

  struct system_impl final {
    static constexpr std::array<cstring, 1> debug_vulkan_layers{ "VK_LAYER_KHRONOS_validation" };

    cstring x_display_string{ };

    bool is_debug{ false };
    u32 vulkan_device_index{ 0 };
  };

}

#endif
