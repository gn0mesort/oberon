#ifndef OBERON_LINUX_VULKAN_HPP
#define OBERON_LINUX_VULKAN_HPP

#include "vkfl.hpp"

#include "x11.hpp"

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xcb.h>

#include "errors.hpp"

//#include "vk_mem_alloc.h"

#define OBERON_DECLARE_VK_PFN(dl, cmd) \
  auto vk##cmd = (reinterpret_cast<PFN_vk##cmd>((dl).get(vkfl::command::cmd)))

#define OBERON_VK_STRUCT(name) VK_STRUCTURE_TYPE_##name

#define OBERON_VK_SUCCEEDS(exp, error) OBERON_INVARIANT(((exp) == VK_SUCCESS), (error))

// Used by default in the vkfl loader.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance, const char*);
// Used by VkDebugMessengerEXT
extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData);

OBERON_STATIC_EXCEPTION_TYPE(vk_create_instance_failed, "Failed to create Vulkan instance.", 1);
OBERON_STATIC_EXCEPTION_TYPE(vk_create_debug_messenger_failed, "Failed to create Vulkan debug messenger.", 1);
OBERON_STATIC_EXCEPTION_TYPE(vk_create_device_failed, "Failed to create Vulkan device.", 1);

#endif
