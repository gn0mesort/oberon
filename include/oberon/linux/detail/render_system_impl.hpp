#ifndef OBERON_LINUX_DETAIL_RENDER_SYSTEM_IMPL_HPP
#define OBERON_LINUX_DETAIL_RENDER_SYSTEM_IMPL_HPP

#include <tuple>

#include <xcb/xcb.h>

#include "../render_system.hpp"

#include "vulkan.hpp"

namespace oberon::linux::detail {

  struct render_system_impl final {
    ptr<vkfl::loader> dl{ };
    VkInstance instance{ };
    VkDebugUtilsMessengerEXT debug_messenger{ };
    VkPhysicalDevice physical_device{ };
    VkDevice device{ };

    u32 primary_queue_family{ };
    VkQueue primary_queue{ };
  };


  VkInstance vk_create_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
                                const readonly_ptr<cstring> extensions, const u32 extension_count,
                                const ptr<void> next, vkfl::loader& dl);
  VkDebugUtilsMessengerEXT vk_create_debug_utils_messenger(const VkInstance instance,
                                                           const VkDebugUtilsMessengerCreateInfoEXT& info,
                                                           const vkfl::loader& dl);
  VkPhysicalDevice vk_select_physical_device(const u32 index, const VkInstance instance,
                                             const ptr<xcb_connection_t> connection, const ptr<xcb_screen_t> screen,
                                             const vkfl::loader& dl);
  std::tuple<VkDevice, u32, VkQueue> vk_create_device(const VkPhysicalDevice physical_device,
                                                      const readonly_ptr<cstring> extensions, const u32 extension_count,
                                                      const ptr<void> next, vkfl::loader& dl);
  std::tuple<VkDevice, u32, VkQueue> vk_create_device_nvidia(const VkPhysicalDevice physical_device,
                                                             VkDeviceCreateInfo& info, vkfl::loader& dl);
  std::tuple<VkDevice, u32, VkQueue> vk_create_device_amd(const VkPhysicalDevice physical_device,
                                                          VkDeviceCreateInfo& info, vkfl::loader& dl);
  std::tuple<VkDevice, u32, VkQueue> vk_create_device_intel(const VkPhysicalDevice physical_device,
                                                            VkDeviceCreateInfo& info, vkfl::loader& dl);
  std::tuple<VkDevice, u32, VkQueue> vk_create_device_generic(const VkPhysicalDevice physical_device,
                                                              VkDeviceCreateInfo& info, vkfl::loader& dl);

  void vk_destroy_device(const VkDevice device, vkfl::loader& dl) noexcept;
  void vk_destroy_debug_messenger(const VkInstance instance,
                                  const VkDebugUtilsMessengerEXT debug_messenger, const vkfl::loader& dl) noexcept;
  void vk_destroy_instance(const VkInstance instance, vkfl::loader& dl) noexcept;
}

#endif
