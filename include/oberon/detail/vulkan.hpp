#ifndef OBERON_DETAIL_VULKAN_HPP
#define OBERON_DETAIL_VULKAN_HPP

#include <type_traits>

// Explicitly enable VK_KHR_xcb_surface types. I don't think this is really necessary but it helps to soothe clangd.
#define VK_USE_PLATFORM_XCB_KHR
// Don't declare *any* prototypes. Require loading of function pointers globally.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define OBERON_INIT_VK_STRUCT(value, type) \
  do { \
    std::memset(&(value), 0, sizeof((value))); \
    (value).sType = VK_STRUCTURE_TYPE_##type; \
  } while (0)

// I'm providing prototypes for both device and instance function loading because I don't feel the adding
// annoyance of loading vkGetDeviceProcAddr manually is worth it compared to the benefit.
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName);
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName);

namespace oberon {
namespace detail {

  // This method is cool but the compile time type comparison is finicky.
  // After compilation the other branches are lost and I can't use gdb to figure out if something went wrong.
  // It's best to use std::remove_cvref_t<Type> unless lvalue refs, rvalue refs, and regular values need to be
  // differentiated.
  template <typename VulkanFunction, typename VulkanFunctionContext>
  VulkanFunction load_vulkan_pfn(VulkanFunctionContext&& context, const char *const name) {
    // Ensure type is in domain
    static_assert(
      std::is_same_v<std::remove_cvref_t<VulkanFunctionContext>, VkInstance> ||
      std::is_same_v<std::remove_cvref_t<VulkanFunctionContext>, VkDevice> ||
      std::is_same_v<std::remove_cvref_t<VulkanFunctionContext>, std::nullptr_t>
    );
    // Select correct implementation
    if constexpr (std::is_same_v<std::remove_cvref_t<VulkanFunctionContext>, VkDevice>)
    {
      return reinterpret_cast<VulkanFunction>(vkGetDeviceProcAddr(context, name));
    }
    else
    {
      return reinterpret_cast<VulkanFunction>(vkGetInstanceProcAddr(context, name));
    }
  }

}
}

#endif
