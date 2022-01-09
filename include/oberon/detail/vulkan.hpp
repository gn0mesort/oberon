#ifndef OBERON_DETAIL_VULKAN_HPP
#define OBERON_DETAIL_VULKAN_HPP

// Explicitly enable VK_KHR_xcb_surface types. I don't think this is really necessary but it helps to soothe clangd.
#define VK_USE_PLATFORM_XCB_KHR
// Don't declare *any* prototypes. Require loading of function pointers globally.
#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

// Bring in external vkfl loader.
#include "vkfl.hpp"

// Declare vkGetInstanceProcAddr so that it can be passed to vkfl.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName);

#define OBERON_GET_PFN(ld, cmd) (reinterpret_cast<PFN_vk##cmd>(ld(vkfl::command::cmd)))

#define OBERON_DECLARE_PFN(ld, cmd) \
  auto vk##cmd = OBERON_GET_PFN((ld), cmd);\
  OBERON_ASSERT(vk##cmd)

#define OBERON_INIT_VK_STRUCT(value, type) \
  do { \
    std::memset(&(value), 0, sizeof((value))); \
    (value).sType = VK_STRUCTURE_TYPE_##type; \
  } while (0)

#endif
