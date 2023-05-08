#include "oberon/internal/base/physical_graphics_device.hpp"

#define VK_STRUCT(name) OBERON_INTERNAL_BASE_VK_STRUCT(name)
#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, cmd)

namespace oberon::internal::base {

  physical_graphics_device::physical_graphics_device(const vkfl::loader& dl, const VkPhysicalDevice physical_device) :
  m_physical_device{ physical_device } {
    auto properties2 = VkPhysicalDeviceProperties2{ };
    properties2.sType = VK_STRUCT(PHYSICAL_DEVICE_PROPERTIES_2);
    m_properties_1_1.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES);
    properties2.pNext = &m_properties_1_1;
    m_properties_1_2.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES);
    m_properties_1_1.pNext = &m_properties_1_2;
    m_properties_1_3.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES);
    m_properties_1_2.pNext = &m_properties_1_3;
    VK_DECLARE_PFN(dl, vkGetPhysicalDeviceProperties2);
    vkGetPhysicalDeviceProperties2(m_physical_device, &properties2);
    m_properties_1_0 = properties2.properties;
    m_properties_1_1.pNext = nullptr;
    m_properties_1_2.pNext = nullptr;
    auto features2 = VkPhysicalDeviceFeatures2{ };
    features2.sType = VK_STRUCT(PHYSICAL_DEVICE_FEATURES_2);
    m_features_1_1.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
    features2.pNext = &m_features_1_1;
    m_features_1_2.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
    m_features_1_1.pNext = &m_features_1_2;
    m_features_1_3.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
    m_features_1_2.pNext = &m_features_1_3;
    VK_DECLARE_PFN(dl, vkGetPhysicalDeviceFeatures2);
    vkGetPhysicalDeviceFeatures2(m_physical_device, &features2);
    m_features_1_0 = features2.features;
    m_features_1_1.pNext = nullptr;
    m_features_1_2.pNext = nullptr;
    VK_DECLARE_PFN(dl, vkGetPhysicalDeviceMemoryProperties);
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_memory_properties);
    auto sz = u32{ };
    VK_DECLARE_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &sz, nullptr);
    m_queue_family_properties.resize(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &sz, m_queue_family_properties.data());
  }

  VkPhysicalDevice physical_graphics_device::handle() const {
    return m_physical_device;
  }

  const VkPhysicalDeviceProperties& physical_graphics_device::properties_1_0() const {
    return m_properties_1_0;
  }

  const VkPhysicalDeviceVulkan11Properties& physical_graphics_device::properties_1_1() const {
    return m_properties_1_1;
  }

  const VkPhysicalDeviceVulkan12Properties& physical_graphics_device::properties_1_2() const {
    return m_properties_1_2;
  }

  const VkPhysicalDeviceVulkan13Properties& physical_graphics_device::properties_1_3() const {
    return m_properties_1_3;
  }

  const VkPhysicalDeviceFeatures& physical_graphics_device::features_1_0() const {
    return m_features_1_0;
  }

  const VkPhysicalDeviceVulkan11Features& physical_graphics_device::features_1_1() const {
    return m_features_1_1;
  }
  const VkPhysicalDeviceVulkan12Features& physical_graphics_device::features_1_2() const {
    return m_features_1_2;
  }

  const VkPhysicalDeviceVulkan13Features& physical_graphics_device::features_1_3() const {
    return m_features_1_3;
  }

  const VkPhysicalDeviceMemoryProperties& physical_graphics_device::memory_properties() const {
    return m_memory_properties;
  }

  const std::vector<VkQueueFamilyProperties>& physical_graphics_device::queue_family_properties() const {
    return m_queue_family_properties;
  }

}
