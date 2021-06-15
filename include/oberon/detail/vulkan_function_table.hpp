#ifndef OBERON_DETAIL_VULKAN_FUNCTION_TABLE_HPP
#define OBERON_DETAIL_VULKAN_FUNCTION_TABLE_HPP

#include "../types.hpp"

#include "vulkan.hpp"

namespace oberon {
namespace detail {
  // Simple struct to hold a variety of Vulkan function pointers.
  // Previously, I was under the impression that Vulkan extensions would, generally, mutually exclude similar
  // extensions. For example, I was under the impression that VK_KHR_xcb_surface and VK_KHR_xlib_surface mutually
  // excluded each other and would cause an error if I attempted to enable both. Further fiddling, and reading the
  // initialization code for vkQuake3, has shown this isn't true. Based on that I'm merging separate function tables
  // into a single global table. I can't imagine the memory overhead (sizeof(void*) bytes) of extra potentially unused
  // function pointers justifies the mental overhead of an expansive family of types.
  struct vulkan_function_table final {
    // Global
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion{ };
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties{ };
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties{ };
    PFN_vkCreateInstance vkCreateInstance{ };

    // Instance
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices{ };
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties{ };
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties{ };
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures{ };
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties{ };
    PFN_vkCreateDevice vkCreateDevice{ };
    PFN_vkDestroyInstance vkDestroyInstance{ };
    // VK_EXT_debug_utils
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT{ };
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{ };
    // VK_KHR_xcb_surface
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR{ };

    // Device
    PFN_vkGetDeviceQueue vkGetDeviceQueue{ };
    PFN_vkDestroyDevice vkDestroyDevice{ };
  };

  iresult load_vulkan_pfns(vulkan_function_table& vkft) noexcept;
  iresult load_vulkan_pfns(vulkan_function_table& vkft, const VkInstance instance) noexcept;
  iresult load_vulkan_pfns(vulkan_function_table& vkft, const VkDevice device) noexcept;
}
}

#endif
