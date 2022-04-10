#include "oberon/linux/x11_vulkan.hpp"

#include <iostream>


namespace {

  oberon::cstring vulkan_severity_to_string(const VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      return "error";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      return "warning";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      return "info";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      return "info";
    default:
      return "";
    }
  }

}

extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void*) {
  auto* stream = &std::cout;
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
      messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    stream = &std::cerr;
  }
  (*stream) << "[vk_" << vulkan_severity_to_string(messageSeverity) << "]: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
