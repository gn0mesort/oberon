#ifndef OBERON_DETAIL_VULKAN_HPP
#define OBERON_DETAIL_VULKAN_HPP

// Explicitly enable VK_KHR_xcb_surface types. I don't think this is really necessary but it helps to soothe clangd.
#define VK_USE_PLATFORM_XCB_KHR
// Don't declare *any* prototypes. Require loading of function pointers globally.
#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

// These might be defined based on the header version and it's easier to just use the newer macro either way.
// I can't be bothered to figure out the relevant header version where this happened.
#ifndef VK_MAKE_API_VERSION
  #define VK_MAKE_API_VERSION(variant, major, minor, patch) (VK_MAKE_VERSION(major, minor, patch))
#endif

#ifndef VK_API_VERSION_VARIANT
  #define VK_API_VERSION_VARIANT(ver) (0)
#endif

#ifndef VK_API_VERSION_MAJOR
  #define VK_API_VERSION_MAJOR(ver) (VK_VERSION_MAJOR(ver))
#endif

#ifndef VK_API_VERSION_MINOR
  #define VK_API_VERSION_MINOR(ver) (VK_VERSION_MINOR(ver))
#endif

#ifndef VK_API_VERSION_PATCH
  #define VK_API_VERSION_PATCH(ver) (VK_VERSION_PATCH(ver))
#endif

#if defined(VK_VERSION_1_0) && !defined(VK_API_VERSION_1_0)
  #define VK_API_VERSION_1_0 (VK_MAKE_API_VERSION(0, 1, 0, 0))
#endif

#if defined(VK_VERSION_1_1) && !defined(VK_API_VERSION_1_1)
  #define VK_API_VERSION_1_1 (VK_MAKE_API_VERSION(0, 1, 1, 0))
#endif

#if defined(VK_VERSION_1_2) && !defined(VK_API_VERSION_1_2)
  #define VK_API_VERSION_1_2 (VK_MAKE_API_VERSION(0, 1, 2, 0))
#endif

// Bring in external vkfl loader.
#include "vkfl.hpp"

// Declare vkGetInstanceProcAddr so that it can be passed to vkfl.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName);

#define OBERON_GET_PFN(ld, cmd) (reinterpret_cast<PFN_vk##cmd>(ld(vkfl::command::cmd)))

#define OBERON_DECLARE_PFN(ld, cmd)\
  auto vk##cmd = OBERON_GET_PFN((ld), cmd);\
  OBERON_ASSERT(vk##cmd)

#define OBERON_INIT_VK_STRUCT(value, type)\
  do {\
    std::memset(&(value), 0, sizeof((value)));\
    (value).sType = VK_STRUCTURE_TYPE_##type;\
  } while (0)

#endif
