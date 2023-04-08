#ifndef OBERON_INTERNAL_VULKAN_HPP
#define OBERON_INTERNAL_VULKAN_HPP

#include "../errors.hpp"

#include "vkfl.hpp"

#define OBERON_INTERNAL_VK_STRUCT(name) (VK_STRUCTURE_TYPE_##name)

#define OBERON_INTERNAL_VK_DECLARE_PFN(dl, pfn) \
  auto pfn = (reinterpret_cast<PFN_##pfn>((dl).get(vkfl::command::pfn)))

#define OBERON_INTERNAL_VK_SUCCEEDS(exp) \
  do \
  { \
    auto result = (exp); \
    OBERON_CHECK_ERROR_MSG(result == VK_SUCCESS, result, "Vulkan command \"" #exp "\" failed with value \"%u\" (%x)", \
                           result, result); \
  } \
  while (0)

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan_core.h>

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char*);
// This MUST be thread safe.
extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL
vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                 VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

#endif
