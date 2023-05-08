#define VMA_IMPLEMENTATION 1
#include "oberon/internal/base/vulkan.hpp"

#include <iostream>

VkBool32 vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                          VkDebugUtilsMessageTypeFlagsEXT /* messageTypes */,
                                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                          void* /* pUserData */) {
  std::cerr << "[VK_";
  switch (messageSeverity)
  {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    std::cerr << "ERROR";
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    std::cerr << "WARN ";
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    std::cerr << "INFO ";
    break;
  default:
    std::cerr << "DEBUG";
  }
  std::cerr << "]: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
