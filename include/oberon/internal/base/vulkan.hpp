#ifndef OBERON_INTERNAL_BASE_VULKAN_HPP
#define OBERON_INTERNAL_BASE_VULKAN_HPP

#include <limits>

#include "configuration.hpp"

#define VK_NO_PROTOTYPES 1
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if defined(CONFIGURATION_OPERATING_SYSTEM_LINUX) && defined (CONFIGURATION_WINDOW_SYSTEM_X11)
  #include <xcb/xcb.h>
  #include <vulkan/vulkan_xcb.h>
#endif

#include <vk_mem_alloc.h>

#include <vkfl.hpp>

#include "../../types.hpp"
#include "../../errors.hpp"

#define OBERON_INTERNAL_BASE_VK_STRUCT(name) (VK_STRUCTURE_TYPE_##name)

#define OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, pfn) \
  auto pfn = (reinterpret_cast<PFN_##pfn>((dl).get(vkfl::command::pfn)))

#define OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp) \
  do \
  { \
    auto result = (exp); \
    OBERON_CHECK_ERROR_MSG(result == VK_SUCCESS, result, "Vulkan command \"" #exp "\" failed with value \"%u\" (%x)", \
                           result, result); \
  } \
  while (0)

// This provides a static declaration of vkGetInstanceProcAddr for use with vkfl::loader.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char*);

// vkDebugUtilsMessengerCallbackEXT *can* be called from multiple threads simultaneously.
// Therefore it is crucial that this function be implemented in a thread-safe way.
extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL
vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                 VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
enum {
  VK_FOREVER = std::numeric_limits<oberon::u64>::max()
};

#endif
