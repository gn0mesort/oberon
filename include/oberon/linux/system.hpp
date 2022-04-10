#ifndef OBERON_LINUX_SYSTEM_HPP
#define OBERON_LINUX_SYSTEM_HPP

#include <unordered_map>

#include "../system.hpp"

namespace oberon::linux {
  enum system_parameters : umax {
      SYS_PARAM_X_DISPLAYNAME,
      SYS_PARAM_VULKAN_DEVICE_INDEX,
      SYS_PARAM_VULKAN_REQUIRED_LAYERS,
      SYS_PARAM_VULKAN_REQUIRED_LAYER_COUNT,
      SYS_PARAM_VULKAN_DEBUG_MESSENGER_ENABLE
  };

  class onscreen_system final : public oberon::onscreen_system {
  private:
    cstring m_x_displayname{ nullptr };
    u32 m_vulkan_device_index{ 0 };
    readonly_ptr<cstring> m_vulkan_layers{ nullptr };
    u32 m_vulkan_layer_count{ 0 };
    bool m_vulkan_debug_messenger_enable{ false };
  public:

    onscreen_system() = default;

    ~onscreen_system() noexcept = default;

    void set_parameter(const umax param, const uptr value) override;
    uptr get_parameter(const umax param) const override;

    void set_x_displayname(const cstring displayname);
    void set_vulkan_device_index(const u32 device_index);
    void set_vulkan_required_layers(const readonly_ptr<cstring> layers, const u32 layer_count);
    void set_vulkan_debug_messenger_enable(const bool enable);

    int run(const ptr<entry_point> main) override;
  };

}

#endif
