#ifndef OBERON_DETAIL_VULKAN_HPP
#define OBERON_DETAIL_VULKAN_HPP

#include <type_traits>

#include "vkfl.hpp"
// Explicitly enable VK_KHR_xcb_surface types. I don't think this is really necessary but it helps to soothe clangd.
#define VK_USE_PLATFORM_XCB_KHR 1
// Don't declare *any* prototypes. Require loading of function pointers globally.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define OBERON_INIT_VK_STRUCT(value, type) \
  do { \
    std::memset(&(value), 0, sizeof((value))); \
    (value).sType = VK_STRUCTURE_TYPE_##type; \
  } while (0)

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName);

#define OBERON_DECLARE_VK_PFN(dl, cmd) \
  auto vk##cmd = reinterpret_cast<PFN_vk##cmd>((dl)(vkfl::command::cmd))

#endif
