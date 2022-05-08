#include "oberon/detail/vulkan.hpp"

#include "oberon/memory.hpp"

#include <iostream>


namespace {

  oberon::cstring vk_severity_to_string(const VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      return "error";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      return "warning";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      return "info";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      return "verbose";
    default:
      return "";
    }
  }

}

extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void*) {
  if (messageSeverity < OBERON_VK_LOG_LEVEL)
  {
    return VK_FALSE;
  }
  auto* stream = &std::cout;
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
      messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    stream = &std::cerr;
  }
  (*stream) << "[vk_" << vk_severity_to_string(messageSeverity) << "]: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
