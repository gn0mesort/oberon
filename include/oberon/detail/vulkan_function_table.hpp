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
    PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT{ };
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{ };
    // VK_KHR_xcb_surface
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR{ };
    PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR{ };
    // VK_KHR_surface
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR{ };
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR{ };
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR{ };
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR{ };
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR{ };

    // Device
    PFN_vkGetDeviceQueue vkGetDeviceQueue{ };
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle{ };
    PFN_vkCreateImageView vkCreateImageView{ };
    PFN_vkCreateRenderPass vkCreateRenderPass{ };
    PFN_vkCreateFramebuffer vkCreateFramebuffer{ };
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer{ };
    PFN_vkDestroyRenderPass vkDestroyRenderPass{ };
    PFN_vkDestroyImageView vkDestroyImageView{ };
    PFN_vkDestroyDevice vkDestroyDevice{ };
    PFN_vkCreateShaderModule vkCreateShaderModule{ };
    PFN_vkDestroyShaderModule vkDestroyShaderModule{ };
    PFN_vkCreatePipelineCache vkCreatePipelineCache{ };
    PFN_vkDestroyPipelineCache vkDestroyPipelineCache{ };
    PFN_vkGetPipelineCacheData vkGetPipelineCacheData{ };
    PFN_vkMergePipelineCaches vkMergePipelineCaches{ };
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout{ };
    PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout{ };
    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines{ };
    PFN_vkDestroyPipeline vkDestroyPipeline{ };
    PFN_vkCreateCommandPool vkCreateCommandPool{ };
    PFN_vkDestroyCommandPool vkDestroyCommandPool{ };
    PFN_vkResetCommandPool vkResetCommandPool{ };
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers{ };
    PFN_vkFreeCommandBuffers vkFreeCommandBuffers{ };
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer{ };
    PFN_vkEndCommandBuffer vkEndCommandBuffer{ };
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass{ };
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass{ };
    PFN_vkCmdBindPipeline vkCmdBindPipeline{ };
    PFN_vkCmdDraw vkCmdDraw{ };
    PFN_vkCreateSemaphore vkCreateSemaphore{ };
    PFN_vkDestroySemaphore vkDestroySemaphore{ };
    PFN_vkQueueSubmit vkQueueSubmit{ };
    PFN_vkCreateFence vkCreateFence{ };
    PFN_vkDestroyFence vkDestroyFence{ };
    PFN_vkWaitForFences vkWaitForFences{ };
    PFN_vkResetFences vkResetFences{ };
    PFN_vkResetCommandBuffer vkResetCommandBuffer{ };
    // VK_KHR_swapchain
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR{ };
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR{ };
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR{ };
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR{ };
    PFN_vkQueuePresentKHR vkQueuePresentKHR{ };
  };

  /**
   * Load global Vulkan function pointers.
   *
   * @param vkft A vulkan_function_table to store global function pointers into.
   *
   * @return 0 in all valid cases.
   */
  iresult load_vulkan_pfns(vulkan_function_table& vkft) noexcept;

  /**
   * Load instance level Vulkan function pointers.
   *
   * Function pointers for extensions can be null after this function returns.
   *
   * @param vkft A vulkan_function_table to store instance level function pointers into.
   * @param instance A valid Vulkan instance handle.
   *
   * @return 0 in all valid cases.
   */
  iresult load_vulkan_pfns(vulkan_function_table& vkft, const VkInstance instance) noexcept;

  /**
   * Load device level Vulkan function pointers.
   *
   * Function pointers for extensions can be null after this function returns.
   *
   * @param vkft A vulkan_function_table to store device level function pointers into.
   * @param device A valid Vulkan device handle.
   *
   * @return 0 in all valid cases.
   */
  iresult load_vulkan_pfns(vulkan_function_table& vkft, const VkDevice device) noexcept;

}
}

#endif
