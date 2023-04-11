#ifndef OBERON_INTERNAL_LINUX_WSI_SYSTEM_HPP
#define OBERON_INTERNAL_LINUX_WSI_SYSTEM_HPP

#ifndef MESON_SYSTEM_LINUX
  #error linux/wsi_system.hpp can only be built on Linux platforms.
#endif

#include "../system.hpp"

#include "wsi_context.hpp"

namespace oberon::internal {

  class wsi_system final : public system {
  private:
    wsi_context m_wsi_context{ };
    std::vector<oberon::wsi_graphics_device> m_wsi_graphics_devices{ };
  public:
    wsi_system(const std::unordered_set<std::string>& requested_layers);
    wsi_system(const wsi_system& other) = delete;
    wsi_system(wsi_system&& other) = delete;

    ~wsi_system() noexcept = default;

    wsi_system& operator=(const wsi_system& rhs) = delete;
    wsi_system& operator=(wsi_system&& rhs) = delete;

    const std::vector<oberon::wsi_graphics_device>& wsi_graphics_devices();
  };

}

#endif
