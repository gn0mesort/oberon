/**
 * @file physical_graphics_device.hpp
 * @brief Internal physical_graphics_device object.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_BASE_PHYSICAL_GRAPHICS_DEVICE_HPP
#define OBERON_INTERNAL_BASE_PHYSICAL_GRAPHICS_DEVICE_HPP

#include <vector>

#include "../../memory.hpp"

#include "vulkan.hpp"

namespace oberon::internal::base {

  /**
   * @class physical_graphics_device
   * @brief An object representing a `VkPhysicalDevice` and, in turn, a physical piece of graphics hardware.
   *
   */
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
    /**
     * @brief Create a `physical_graphics_device`.
     * @param dl A `vkfl::loader` to use when loading physical device information.
     * @param physical_device A `VkPhysicalDevice` handle that the new `physical_graphics_device` will represent.
     */
    physical_graphics_device(const vkfl::loader& dl, const VkPhysicalDevice physical_device);

    /**
     * @brief Create a copy of a `physical_graphics_device`.
     * @param other The `physical_graphics_device` to copy.
     */
    physical_graphics_device(const physical_graphics_device& other) = default;

    /// @cond
    physical_graphics_device(physical_graphics_device&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `physical_graphics_device`.
     */
    ~physical_graphics_device() noexcept = default;

    /**
     * @brief Assign a copy of a `physical_graphics_device`.
     * @param rhs The `physical_graphics_device` to copy.
     * @return A reference to the assigned `physical_graphics_device`.
     */
    physical_graphics_device& operator=(const physical_graphics_device& rhs) = default;

    /// @cond
    physical_graphics_device& operator=(physical_graphics_device&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the `physical_graphics_device`s underlying `VkPhysicalDevice`.
     * @return The underlying `VkPhysicalDevice`.
     */
    VkPhysicalDevice handle() const;

    /**
     * @brief Retrieve the Vulkan 1.0 `physical_graphics_device` properties.
     * @return A reference to the Vulkan 1.0 properties.
     */
    const VkPhysicalDeviceProperties& properties_1_0() const;

    /**
     * @brief Retrieve the Vulkan 1.1 `physical_graphics_device` properties.
     * @return A reference to the Vulkan 1.1 properties.
     */
    const VkPhysicalDeviceVulkan11Properties& properties_1_1() const;

    /**
     * @brief Retrieve the Vulkan 1.2 `physical_graphics_device` properties.
     * @return A reference to the Vulkan 1.2 properties.
     */
    const VkPhysicalDeviceVulkan12Properties& properties_1_2() const;

    /**
     * @brief Retrieve the Vulkan 1.3 `physical_graphics_device` properties.
     * @return A reference to the Vulkan 1.3 properties.
     */
    const VkPhysicalDeviceVulkan13Properties& properties_1_3() const;

    /**
     * @brief Retrieve the Vulkan 1.0 `physical_graphics_device` features.
     * @return A reference to the Vulkan 1.0 features.
     */
    const VkPhysicalDeviceFeatures& features_1_0() const;

    /**
     * @brief Retrieve the Vulkan 1.1 `physical_graphics_device` features.
     * @return A reference to the Vulkan 1.1 features.
     */
    const VkPhysicalDeviceVulkan11Features& features_1_1() const;

    /**
     * @brief Retrieve the Vulkan 1.2 `physical_graphics_device` features.
     * @return A reference to the Vulkan 1.2 features.
     */
    const VkPhysicalDeviceVulkan12Features& features_1_2() const;

    /**
     * @brief Retrieve the Vulkan 1.3 `physical_graphics_device` features.
     * @return A reference to the Vulkan 1.3 features.
     */
    const VkPhysicalDeviceVulkan13Features& features_1_3() const;

    /**
     * @brief Retrieve the Vulkan memory properties of the `physical_graphics_device`.
     * @return A reference to a `VkPhysicalDeviceMemoryProperties`.
     */
    const VkPhysicalDeviceMemoryProperties& memory_properties() const;

    /**
     * @brief Retrieve the Vulkan queue family properties of the `physical_graphics_device`.
     * @return A reference to a list of `VkQueueFamilyProperties`.
     */
    const std::vector<VkQueueFamilyProperties>& queue_family_properties() const;
  };

}

#endif
