/**
 * @file graphics_device_impl.hpp
 * @brief Internal graphics_device API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_BASE_GRAPHICS_DEVICE_IMPL_HPP
#define OBERON_INTERNAL_BASE_GRAPHICS_DEVICE_IMPL_HPP

#include <list>
#include <vector>

#include "vulkan.hpp"
#include "physical_graphics_device.hpp"

namespace oberon::internal::base {

  class graphics_context;
  class physical_graphics_device;

  struct image_allocation final {
    VkImage image{ };
    VmaAllocation allocation{ };
    VmaAllocationInfo info{ };
  };

  struct buffer_allocation final {
    VkBuffer buffer{ };
    VmaAllocation allocation{ };
    VmaAllocationInfo info{ };
  };

  /**
   * @class graphics_device_impl
   * @brief The base implementation of a Vulkan graphics device.
   *
   */
  class graphics_device_impl {
  protected:

    vkfl::loader m_dl;
    VmaAllocator m_allocator{ };
    physical_graphics_device m_physical_device;
    u32 m_complete_queue_family{ };
    VkQueue m_complete_queue{ };

    std::list<image_allocation> m_image_allocations{ };
    std::list<buffer_allocation> m_buffer_allocations{ };

    graphics_device_impl(graphics_context& gfx, const physical_graphics_device& physical_device);
  public:
    using image_iterator = std::list<image_allocation>::iterator;
    using buffer_iterator = std::list<buffer_allocation>::iterator;

    /// @cond
    graphics_device_impl(const graphics_device_impl& other) = delete;
    graphics_device_impl(graphics_device_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `graphics_device_impl`.
     */
    virtual ~graphics_device_impl() noexcept;

    /// @cond
    graphics_device_impl& operator=(const graphics_device_impl& rhs) = delete;
    graphics_device_impl& operator=(graphics_device_impl&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the `physical_graphics_device` on which the device is based.
     * @return A reference to the underlying `physical_graphics_device`.
     */
    const physical_graphics_device& physical_device() const;

    /**
     * @brief Retrieve the device's Vulkan function pointer loader and dispatch table.
     * @return A dispatch table and loader initialized with device-level functionality.
     */
    const vkfl::loader& dispatch_loader();

    /**
     * @brief Retrieve the `VkInstance` that this `graphics_device_impl` represents.
     * @details Usually, all `graphics_device` instances represent the same `VkInstance`.
     * @return The `VkInstance` corresponding to this `graphics_device_impl`.
     */
    VkInstance instance_handle();

    /**
     * @brief Retrieve the `VkDevice` that this `graphics_device_impl` represents.
     * @return The `VkDevice` corresponding to this `graphics_device_impl`.
     */
    VkDevice device_handle();

    /**
     * @brief Retrieve the `VmaAllocator` associated with this `graphics_device_impl`.
     * @return The handle of the `graphics_device_impl`'s `VmaAllocator`.
     */
    VmaAllocator allocator();

    /**
     * @brief Retrieve the queue family index of the `graphics_device_impl`'s complete queue.
     * @return The 32-bit index of the queue family supporting graphics, transfer, and presentation operations.
     */
    u32 queue_family() const;

    /**
     * @brief Retrieve the complete queue for this `graphics_device_impl`.
     * @return A `VkQueue` representing a complete queue.
     */
    VkQueue queue();

    /**
     * @brief Block until all pending operations on this device are complete.
     */
    void wait_for_idle();

    /**
     * @brief Select a `VkFormat` from a set of desired `VkFormat`s that matches a `VkImageTiling` and
     *        `VkFormatFeatureFlags`.
     * @param desired A list of desired `VkFormat`s. The order of the list defines the preference (i.e., the first
     *                `VkFormat` that matches the requirements will be returned).
     * @param tiling The required `VkImageTiling`.
     * @param features The required `VkFormatFeatureFlags`.
     * @return The first corresponding `VkFormat` of the desired `VkFormat`s or `VK_FORMAT_UNDEFINED` if none are
     *         are acceptable.
     */
    VkFormat select_image_format(const std::vector<VkFormat>& desired, const VkImageTiling tiling,
                                 const VkFormatFeatureFlags features) const;

    /**
     * @brief Select a `VkFormat` from a set of desired `VkFormat`s that matches a `VkFormatFeatureFlags`.
     * @param desired A list of desired `VkFormat`s. The order of the list defines the preference (i.e., the first
     *                `VkFormat` that matches the requirements will be returned).
     * @param features The required `VkFormatFeatureFlags`.
     * @return The first corresponding `VkFormat` of the desired `VkFormat`s or `VK_FORMAT_UNDEFINED` if none are
     *         are acceptable.
     */
    VkFormat select_buffer_format(const std::vector<VkFormat>& desired, const VkFormatFeatureFlags features) const;

    /**
     * @brief Create a `VkImage` and pack it into a `image_iterator`.
     * @param image_info Information about how to create the `VkImage`.
     * @param allocation_info Information about how to allocate memory for the `VkImage`.
     * @return An `image_iterator` containing the new `VkImage` and its allocation info.
     */
    image_iterator create_image(const VkImageCreateInfo& image_info, const VmaAllocationCreateInfo& allocation_info);
    /**
     * @brief Destroy a `VkImage`.
     * @param image The `image_iterator` that contains the `VkImage` to destroy.
     */
    void destroy_image(image_iterator image);

    /**
     * @brief Create a `VkBuffer` and pack it into a `buffer_iterator`.
     * @param buffer_info Information about how to create the `VkBuffer`.
     * @param allocation_info Information about how to allocate memory for the `VkBuffer`.
     * @return A `buffer_iterator` containing the new `VkBuffer` and its allocation info.
     */
    buffer_iterator create_buffer(const VkBufferCreateInfo& buffer_info,
                                  const VmaAllocationCreateInfo& allocation_info);
    /**
     * @brief Destroy a `VkBuffer`.
     * @param buffer The `buffer_iterator` that contains the `VkBuffer` to destroy.
     */
    void destroy_buffer(buffer_iterator buffer);

    /**
     * @brief Flush a write to a `VkBuffer`.
     * @param buffer The `buffer_iterator` that contains the `VkBuffer` to destroy.
     */
    void flush_buffer(buffer_iterator buffer);

  };

}

#endif
