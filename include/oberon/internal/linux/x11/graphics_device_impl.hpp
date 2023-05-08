#ifndef OBERON_INTERNAL_LINUX_X11_GRAPHICS_DEVICE_IMPL_HPP
#define OBERON_INTERNAL_LINUX_X11_GRAPHICS_DEVICE_IMPL_HPP

#include <unordered_set>
#include <string>

#include "../../base/graphics_device_impl.hpp"

namespace oberon::internal::linux::x11 {

  class wsi_context;

  class graphics_device_impl final : public base::graphics_device_impl {
  private:
    ptr<wsi_context> m_wsi_context{ };

    u32 select_queue_family();
    std::unordered_set<std::string> available_extensions();
  public:
    graphics_device_impl(wsi_context& wsi, base::graphics_context& gfx,
                         const base::physical_graphics_device& physical_device,
                         const std::unordered_set<std::string>& required_extensions,
                         const std::unordered_set<std::string>& requested_extensions);
    graphics_device_impl(const graphics_device_impl& other) = delete;
    graphics_device_impl(graphics_device_impl&& other) = delete;

    ~graphics_device_impl() noexcept = default;

    graphics_device_impl& operator=(const graphics_device_impl& rhs) = delete;
    graphics_device_impl& operator=(graphics_device_impl&& rhs) = delete;

    wsi_context& wsi();
  };

}

#endif
