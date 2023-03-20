#include "oberon/linux/vk_device.hpp"

#define DECLARE_VK_PFN(loader, command) OBERON_LINUX_VK_DECLARE_PFN(loader, command)
#define VK_SUCCEEDS(exp) OBERON_LINUX_VK_SUCCEEDS(exp)

namespace oberon::linux {

  void intel_vk_device::select_device_queues(const vkfl::loader&, const VkSurfaceKHR, const VkPhysicalDevice,
                                              i64& graphics_queue_family, i64& transfer_queue_family,
                                              i64& present_queue_family) {
    graphics_queue_family = 0;
    transfer_queue_family = 0;
    present_queue_family = 0;
  }

  intel_vk_device::intel_vk_device(const vkfl::loader& dl, VkSurfaceKHR surface,
                                     const VkPhysicalDevice physical_device, const VkExtent2D& desired_image_extent) :
  vk_device{ dl, surface, physical_device, desired_image_extent } { }

}
