#ifndef OBERON_INTERNAL_LINUX_X11_SYSTEM_IMPL_HPP
#define OBERON_INTERNAL_LINUX_X11_SYSTEM_IMPL_HPP

#include <span>

#include "../../base/system_impl.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::linux::x11 {

  class wsi_context;

  class system_impl final : public base::system_impl {
  private:
    ptr<wsi_context> m_wsi_context{ };
    usize m_graphics_device_count{ };
    csequence m_graphics_device_binary{ };
  public:
    system_impl(ptr<wsi_context>&& wsi, ptr<base::graphics_context>&& gfx);
    system_impl(const system_impl& other) = delete;
    system_impl(system_impl&& other) = delete;

    ~system_impl() noexcept;

    system_impl& operator=(const system_impl& rhs) = delete;
    system_impl& operator=(system_impl&& rhs) = delete;

    std::span<graphics_device> graphics_devices() override;
  };

}

#endif
