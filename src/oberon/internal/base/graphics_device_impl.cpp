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

}
