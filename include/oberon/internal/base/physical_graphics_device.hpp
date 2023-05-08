#ifndef OBERON_INTERNAL_BASE_PHYSICAL_GRAPHICS_DEVICE_HPP
#define OBERON_INTERNAL_BASE_PHYSICAL_GRAPHICS_DEVICE_HPP

#include <vector>

#include "../../memory.hpp"

#include "vulkan.hpp"

namespace oberon::internal::base {

  class physical_graphics_device final {
  private:
    VkPhysicalDevice m_physical_device{ };
    VkPhysicalDeviceProperties m_properties_1_0{ };
    VkPhysicalDeviceVulkan11Properties m_properties_1_1{ };
    VkPhysicalDeviceVulkan12Properties m_properties_1_2{ };
    VkPhysicalDeviceVulkan13Properties m_properties_1_3{ };
    VkPhysicalDeviceFeatures m_features_1_0{ };
    VkPhysicalDeviceVulkan11Features m_features_1_1{ };
    VkPhysicalDeviceVulkan12Features m_features_1_2{ };
    VkPhysicalDeviceVulkan13Features m_features_1_3{ };
    VkPhysicalDeviceMemoryProperties m_memory_properties{ };
    std::vector<VkQueueFamilyProperties> m_queue_family_properties{ };
  public:
    physical_graphics_device(const vkfl::loader& dl, const VkPhysicalDevice physical_device);
    physical_graphics_device(const physical_graphics_device& other) = default;
    physical_graphics_device(physical_graphics_device&& other) = delete;

    ~physical_graphics_device() noexcept = default;

    physical_graphics_device& operator=(const physical_graphics_device& rhs) = default;
    physical_graphics_device& operator=(physical_graphics_device&& rhs) = delete;

    VkPhysicalDevice handle() const;
    const VkPhysicalDeviceProperties& properties_1_0() const;
    const VkPhysicalDeviceVulkan11Properties& properties_1_1() const;
    const VkPhysicalDeviceVulkan12Properties& properties_1_2() const;
    const VkPhysicalDeviceVulkan13Properties& properties_1_3() const;
    const VkPhysicalDeviceFeatures& features_1_0() const;
    const VkPhysicalDeviceVulkan11Features& features_1_1() const;
    const VkPhysicalDeviceVulkan12Features& features_1_2() const;
    const VkPhysicalDeviceVulkan13Features& features_1_3() const;
    const VkPhysicalDeviceMemoryProperties& memory_properties() const;
    const std::vector<VkQueueFamilyProperties>& queue_family_properties() const;
  };

}

#endif
