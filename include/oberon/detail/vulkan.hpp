#ifndef OBERON_DETAIL_VULKAN_HPP
#define OBERON_DETAIL_VULKAN_HPP

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan_core.h>

#include "vkfl.hpp"

#include "../basics.hpp"

//#include "vk_mem_alloc.h"

#define OBERON_DECLARE_VK_PFN(dl, cmd) \
  auto vk##cmd = (reinterpret_cast<PFN_vk##cmd>((dl).get(vkfl::command::cmd)))

#define OBERON_VK_STRUCT(name) VK_STRUCTURE_TYPE_##name

// Vulkan success checks are a type of invariant.
// Bypass at your own risk.
#if OBERON_INVARIANTS_ENABLED
  #define OBERON_VK_SUCCEEDS(exp) \
    do\
    {\
      if (auto res = (exp); res < VK_SUCCESS) \
      { \
        throw oberon::vulkan_error{ "\'" #exp "\' failed.", res }; \
      } \
    }\
    while (0)
#else
  #define OBERON_VK_SUCCEEDS(exp) ((void) (exp))
#endif

#ifndef OBERON_VK_LOG_LEVEL
  #define OBERON_VK_LOG_LEVEL VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
#endif

// Used by default in the vkfl loader.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance, const char*);
// Used by VkDebugMessengerEXT
extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData);

#endif
