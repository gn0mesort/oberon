#ifndef OBERON_DETAIL_VULKAN_FUNCTION_TABLE_HPP
#define OBERON_DETAIL_VULKAN_FUNCTION_TABLE_HPP

#include "graphics.hpp"

namespace oberon {
namespace detail {
  struct vulkan_function_table {
    inline virtual ~vulkan_function_table() noexcept = 0;

    // Global
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{ };
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
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr{ };
    PFN_vkDestroyInstance vkDestroyInstance{ };

    // Device
    PFN_vkDestroyDevice vkDestroyDevice{ };
  };

  vulkan_function_table::~vulkan_function_table() noexcept { }
}
}

#endif
