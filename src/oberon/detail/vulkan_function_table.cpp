#include "oberon/detail/vulkan_function_table.hpp"

#include "oberon/debug.hpp"

#define OBERON_VK_PFN(target, context, name, required) \
  do { \
    (target).name = load_vulkan_pfn<PFN_##name>((context), #name); \
    OBERON_ASSERT_MSG(!(required) || (target).name, "Failed to load \"" #name "\" from Vulkan driver.\n"); \
  } while (0)


namespace oberon {
namespace detail {
  iresult load_vulkan_pfns(vulkan_function_table& vkft) noexcept {
    OBERON_VK_PFN(vkft, nullptr, vkEnumerateInstanceVersion, true);
    OBERON_VK_PFN(vkft, nullptr, vkEnumerateInstanceLayerProperties, true);
    OBERON_VK_PFN(vkft, nullptr, vkEnumerateInstanceExtensionProperties, true);
    OBERON_VK_PFN(vkft, nullptr, vkCreateInstance, true);
    return 0;
  }

  iresult load_vulkan_pfns(vulkan_function_table& vkft, const VkInstance instance) noexcept {
    OBERON_PRECONDITION(instance);
    // Instance
    OBERON_VK_PFN(vkft, instance, vkEnumeratePhysicalDevices, true);
    OBERON_VK_PFN(vkft, instance, vkEnumerateDeviceExtensionProperties, true);
    OBERON_VK_PFN(vkft, instance, vkGetPhysicalDeviceProperties, true);
    OBERON_VK_PFN(vkft, instance, vkGetPhysicalDeviceFeatures, true);
    OBERON_VK_PFN(vkft, instance, vkGetPhysicalDeviceQueueFamilyProperties, true);
    OBERON_VK_PFN(vkft, instance, vkCreateDevice, true);
    OBERON_VK_PFN(vkft, instance, vkDestroyInstance, true);
    // VK_EXT_debug_utils
    OBERON_VK_PFN(vkft, instance, vkCreateDebugUtilsMessengerEXT, false);
    OBERON_VK_PFN(vkft, instance, vkDestroyDebugUtilsMessengerEXT, false);
    // VK_KHR_xcb_surface
    OBERON_VK_PFN(vkft, instance, vkGetPhysicalDeviceXcbPresentationSupportKHR, false);
    return 0;
  }

  iresult load_vulkan_pfns(vulkan_function_table& vkft, const VkDevice device) noexcept {
    OBERON_PRECONDITION(device);
    OBERON_VK_PFN(vkft, device, vkGetDeviceQueue, true);
    OBERON_VK_PFN(vkft, device, vkDestroyDevice, true);
    return 0;
  }

}
}
