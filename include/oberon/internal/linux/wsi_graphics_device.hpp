#ifndef OBERON_INTERNAL_LINUX_WSI_GRAPHICS_DEVICE_HPP
#define OBERON_INTERNAL_LINUX_WSI_GRAPHICS_DEVICE_HPP

#ifndef MESON_SYSTEM_LINUX
  #error linux/wsi_graphics_device.hpp can only be built on Linux platforms.
#endif

#include "../graphics_device.hpp"

#include "wsi_context.hpp"

namespace oberon::internal {

  class wsi_graphics_device final : public graphics_device {
  private:
    ptr<wsi_context> m_wsi_context{ };
  public:
    wsi_graphics_device(wsi_context& wsi, const vkfl::loader& dl, const VkPhysicalDevice physical_device,
                        const u32 graphics_queue_family,
                        const std::unordered_set<std::string>& required_extensions,
                        const std::unordered_set<std::string>& requested_extensions);
    wsi_graphics_device(const wsi_graphics_device& other) = delete;
    wsi_graphics_device(wsi_graphics_device&& other) = delete;

    ~wsi_graphics_device() noexcept = default;

    wsi_graphics_device& operator=(const wsi_graphics_device& rhs) = delete;
    wsi_graphics_device& operator=(wsi_graphics_device&& rhs) = delete;
  };

}

#endif
