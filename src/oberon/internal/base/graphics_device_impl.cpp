#include "oberon/internal/base/graphics_device_impl.hpp"

#include "oberon/graphics_device.hpp"

#include "oberon/internal/base/graphics_context.hpp"

#define VK_STRUCT(name) OBERON_INTERNAL_BASE_VK_STRUCT(name)
#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, cmd)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::base {

  void graphics_device_impl_dtor::operator()(const ptr<graphics_device_impl> p) const noexcept {
    delete p;
  }

  graphics_device_impl::graphics_device_impl(graphics_context& gfx, const physical_graphics_device& physical_device) :
  m_dl{ gfx.dispatch_loader() },
  m_physical_device{ physical_device } { }

  graphics_device_impl::~graphics_device_impl() noexcept {
    VK_DECLARE_PFN(m_dl, vkDeviceWaitIdle);
    // vkDeviceWaitIdle is allowed to fail for a variety of reasons. However, since this is a destructor, there's
    // no real way to handle the case of the function failing. If an error is returned we destroy the device
    // regardless.
    vkDeviceWaitIdle(m_dl.loaded_device());
    for (auto& allocation : m_image_allocations)
    {
      vmaDestroyImage(m_allocator, allocation.image, allocation.allocation);
    }
    for (auto& allocation : m_buffer_allocations)
    {
      vmaDestroyBuffer(m_allocator, allocation.buffer, allocation.allocation);
    }
    vmaDestroyAllocator(m_allocator);
    VK_DECLARE_PFN(m_dl, vkDestroyDevice);
    vkDestroyDevice(m_dl.loaded_device(), nullptr);
  }

  const physical_graphics_device& graphics_device_impl::physical_device() const {
    return m_physical_device;
  }

  const vkfl::loader& graphics_device_impl::dispatch_loader() {
    return m_dl;
  }

  u32 graphics_device_impl::queue_family() const {
    return m_complete_queue_family;
  }

  VkInstance graphics_device_impl::instance_handle() {
    return m_dl.loaded_instance();
  }

  VkDevice graphics_device_impl::device_handle() {
    return m_dl.loaded_device();
  }

  VmaAllocator graphics_device_impl::allocator() {
    return m_allocator;
  }

  VkQueue graphics_device_impl::queue() {
    return m_complete_queue;
  }

  void graphics_device_impl::wait_for_idle() {
    VK_DECLARE_PFN(m_dl, vkDeviceWaitIdle);
    VK_SUCCEEDS(vkDeviceWaitIdle(m_dl.loaded_device()));
  }

  VkFormat graphics_device_impl::select_image_format(const std::vector<VkFormat>& desired, const VkImageTiling tiling,
                                                     const VkFormatFeatureFlags features) const {
    auto properties = VkFormatProperties{ };
    VK_DECLARE_PFN(m_dl, vkGetPhysicalDeviceFormatProperties);
    for (const auto& format : desired)
    {
      vkGetPhysicalDeviceFormatProperties(m_physical_device.handle(), format, &properties);
      switch (tiling)
      {
      case VK_IMAGE_TILING_OPTIMAL:
        if ((properties.optimalTilingFeatures & features) == features)
        {
          return format;
        }
        break;
      case VK_IMAGE_TILING_LINEAR:
        if ((properties.linearTilingFeatures & features) == features)
        {
          return format;
        }
      default:
        break;
      }
    }
    return VK_FORMAT_UNDEFINED;
  }

  VkFormat graphics_device_impl::select_buffer_format(const std::vector<VkFormat>& desired,
                                                      const VkFormatFeatureFlags features) const {
    auto properties = VkFormatProperties{ };
    VK_DECLARE_PFN(m_dl, vkGetPhysicalDeviceFormatProperties);
    for (const auto format : desired)
    {
      vkGetPhysicalDeviceFormatProperties(m_physical_device.handle(), format, &properties);
      if ((properties.bufferFeatures & features) == features)
      {
        return format;
      }
    }
    return VK_FORMAT_UNDEFINED;
  }

  graphics_device_impl::image_iterator
  graphics_device_impl::create_image(const VkImageCreateInfo& image_info,
                                     const VmaAllocationCreateInfo& allocation_info) {
    m_image_allocations.emplace_front();
    auto res = m_image_allocations.begin();
    VK_SUCCEEDS(vmaCreateImage(m_allocator, &image_info, &allocation_info, &res->image, &res->allocation, &res->info));
    return res;
  }

  void graphics_device_impl::destroy_image(graphics_device_impl::image_iterator image) {
    vmaDestroyImage(m_allocator, image->image, image->allocation);
    m_image_allocations.erase(image);
  }

  graphics_device_impl::buffer_iterator
  graphics_device_impl::create_buffer(const VkBufferCreateInfo& buffer_info,
                                      const VmaAllocationCreateInfo& allocation_info) {
    m_buffer_allocations.emplace_front();
    auto res = m_buffer_allocations.begin();
    VK_SUCCEEDS(vmaCreateBuffer(m_allocator, &buffer_info, &allocation_info, &res->buffer, &res->allocation,
                                &res->info));
    return res;
  }

  void graphics_device_impl::destroy_buffer(graphics_device_impl::buffer_iterator buffer) {
    vmaDestroyBuffer(m_allocator, buffer->buffer, buffer->allocation);
    m_buffer_allocations.erase(buffer);
  }

  void graphics_device_impl::flush_buffer(graphics_device_impl::buffer_iterator buffer) {
    VK_SUCCEEDS(vmaFlushAllocation(m_allocator, buffer->allocation, 0, VK_WHOLE_SIZE));
  }

}
