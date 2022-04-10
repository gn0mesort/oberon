#ifndef OBERON_LINUX_X11_VULKAN_HPP
#define OBERON_LINUX_X11_VULKAN_HPP

#include <xcb/xcb.h>

#include "vkfl.hpp"
#define VK_NO_PROTOTYPES 1
#define VK_USE_PLATFORM_XCB_KHR 1
#include <vulkan/vulkan.h>

#include "../errors.hpp"

#define OBERON_DECLARE_VK_PFN(dl, cmd) \
  auto vk##cmd = (reinterpret_cast<PFN_vk##cmd>((dl).get(vkfl::command::cmd)))

#define OBERON_VK_STRUCT(name) VK_STRUCTURE_TYPE_##name

#define OBERON_VK_SUCCEEDS(exp, error) OBERON_CHECK(exp, VK_SUCCESS, error)

// Used by default in the vkfl loader.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance, const char*);
// Used by VkDebugMessengerEXT
extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData);

OBERON_EXCEPTION_TYPE(generic_x, "An X11 error occurred.", 1);
OBERON_EXCEPTION_TYPE(generic_vulkan, "A Vulkan error occurred.", 1);

#endif
