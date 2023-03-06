#include "oberon/linux/vk.hpp"

#include <iostream>

namespace oberon::linux {

  VkBool32 vk_debug_log(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "[VK_VALIDATION]: " << pCallbackData->pMessage << std::endl;
    // Always returns VK_FALSE (0).
    // See https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
    return VK_FALSE;
  }

}
