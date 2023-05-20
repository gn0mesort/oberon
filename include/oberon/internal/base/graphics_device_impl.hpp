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

    graphics_device_impl(const graphics_device_impl& other) = delete;
    graphics_device_impl(graphics_device_impl&& other) = delete;

    virtual ~graphics_device_impl() noexcept;

    graphics_device_impl& operator=(const graphics_device_impl& rhs) = delete;
    graphics_device_impl& operator=(graphics_device_impl&& rhs) = delete;


    const physical_graphics_device& physical_device() const;
    const vkfl::loader& dispatch_loader();
    VkInstance instance_handle();
    VkDevice device_handle();
    VmaAllocator allocator();
    u32 queue_family() const;
    VkQueue queue();
    void wait_for_idle();
    VkFormat select_image_format(const std::vector<VkFormat>& desired, const VkImageTiling tiling,
                                 const VkFormatFeatureFlags features) const;
    VkFormat select_buffer_format(const std::vector<VkFormat>& desired, const VkFormatFeatureFlags features) const;
    image_iterator create_image(const VkImageCreateInfo& image_info, const VmaAllocationCreateInfo& allocation_info);
    void destroy_image(image_iterator image);
    buffer_iterator create_buffer(const VkBufferCreateInfo& buffer_info,
                                  const VmaAllocationCreateInfo& allocation_info);
    void destroy_buffer(buffer_iterator buffer);
    void flush_buffer(buffer_iterator buffer);

  };

}

#endif
