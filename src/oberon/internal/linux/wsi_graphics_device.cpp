#include "oberon/internal/linux/wsi_graphics_device.hpp"

#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_VK_DECLARE_PFN(dl, cmd)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_VK_SUCCEEDS(exp)
#define VK_STRUCT(name) OBERON_INTERNAL_VK_STRUCT(name)

namespace oberon::internal {

  wsi_graphics_device::wsi_graphics_device(wsi_context& wsi, const vkfl::loader& dl,
                                           const VkPhysicalDevice physical_device, const u32 graphics_queue_family,
                                           const std::unordered_set<std::string>& required_extensions,
                                           const std::unordered_set<std::string>& requested_extensions) :
  graphics_device{ defer_construction{ } }, m_wsi_context{ &wsi } {
    m_dl = dl;
    m_physical_device = physical_device;
    m_graphics_queue_family = graphics_queue_family;
    auto required_device_extensions = required_extensions;
    required_device_extensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    m_features_11.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
    m_features_12.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
    m_features_13.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
    m_features_11.pNext = &m_features_12;
    m_features_12.pNext = &m_features_13;
    auto features2 = query_features(m_dl, m_physical_device, &m_features_11);
    auto priority = 1.0f;
    auto queue_info = pack_device_queue_info(graphics_queue_family, 1, &priority);
    auto extensions = available_extensions(m_dl, m_physical_device);
    auto selected_extensions = select_extensions(extensions, required_device_extensions, requested_extensions);
    auto device_info = pack_device_info(nullptr, &queue_info, 1, selected_extensions.data(),
                                        selected_extensions.size(), &features2);
    VK_DECLARE_PFN(m_dl, vkCreateDevice);
    auto device = VkDevice{ };
    VK_SUCCEEDS(vkCreateDevice(m_physical_device, &device_info, nullptr, &device));
    intern_device_in_loader(m_dl, device);
    VK_DECLARE_PFN(m_dl, vkGetDeviceQueue);
    vkGetDeviceQueue(m_dl.loaded_device(), m_graphics_queue_family, 0, &m_graphics_queue);
    auto allocator_info = pack_allocator_info(m_dl, m_physical_device);
    VK_SUCCEEDS(vmaCreateAllocator(&allocator_info, &m_allocator));
    m_properties_11.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES);
    m_properties_12.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES);
    m_properties_13.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES);
    m_properties_11.pNext = &m_properties_12;
    m_properties_12.pNext = &m_properties_13;
    auto properties2 = query_properties(m_dl, m_physical_device, &m_properties_11);
    // Unpack and unlink physical device info.
    m_properties = properties2.properties;
    m_properties_11.pNext = nullptr;
    m_properties_12.pNext = nullptr;
    m_properties_13.pNext = nullptr;
    m_features = features2.features;
    m_features_11.pNext = nullptr;
    m_features_12.pNext = nullptr;
    m_features_13.pNext = nullptr;
  }

}
