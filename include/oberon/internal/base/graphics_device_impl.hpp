#ifndef OBERON_INTERNAL_BASE_GRAPHICS_DEVICE_IMPL_HPP
#define OBERON_INTERNAL_BASE_GRAPHICS_DEVICE_IMPL_HPP

#include "vulkan.hpp"
#include "physical_graphics_device.hpp"

namespace oberon::internal::base {

  class graphics_context;
  class physical_graphics_device;

  class graphics_device_impl {
  protected:
    vkfl::loader m_dl;
    physical_graphics_device m_physical_device;
    u32 m_complete_queue_family{ };
    VkQueue m_complete_queue{ };

    graphics_device_impl(graphics_context& gfx, const physical_graphics_device& physical_device);
  public:
    graphics_device_impl(const graphics_device_impl& other) = delete;
    graphics_device_impl(graphics_device_impl&& other) = delete;

    virtual ~graphics_device_impl() noexcept;

    graphics_device_impl& operator=(const graphics_device_impl& rhs) = delete;
    graphics_device_impl& operator=(graphics_device_impl&& rhs) = delete;

    const physical_graphics_device& physical_device() const;
    const vkfl::loader& dispatch_loader();
    VkInstance instance_handle();
    VkDevice device_handle();
    u32 queue_family() const;
    void wait_for_idle();
  };

}

#endif
