#include "oberon/linux/vk.hpp"

#include <iostream>

namespace oberon::linux {

  VkBool32 vk_debug_log(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                        VkDebugUtilsMessageTypeFlagsEXT,
                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      std::cerr << "[VK WARN]: " << pCallbackData->pMessage << std::endl;
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      std::cerr << "[VK ERROR]: " << pCallbackData->pMessage << std::endl;
      break;
    default:
      break;
    }
    // Always returns VK_FALSE (0).
    // See https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
    return VK_FALSE;
  }

}
