#include "oberon/linux/system.hpp"

#include <iostream>

#include "oberon/errors.hpp"
#include "oberon/linux/context.hpp"
#include "oberon/linux/render_target.hpp"

namespace oberon::linux {

  void onscreen_system::set_parameter(const umax param, const uptr value) {
    switch (param)
    {
    case SYS_PARAM_X_DISPLAYNAME:
      m_x_displayname = reinterpret_cast<cstring>(value);
      break;
    case SYS_PARAM_VULKAN_DEVICE_INDEX:
      m_vulkan_device_index = static_cast<u32>(value);
      break;
    case SYS_PARAM_VULKAN_REQUIRED_LAYERS:
      m_vulkan_layers = reinterpret_cast<readonly_ptr<cstring>>(value);
      break;
    case SYS_PARAM_VULKAN_REQUIRED_LAYER_COUNT:
      m_vulkan_layer_count = static_cast<u32>(value);
      break;
    case SYS_PARAM_VULKAN_DEBUG_MESSENGER_ENABLE:
      m_vulkan_debug_messenger_enable = static_cast<bool>(value);
      break;
    default:
      break;
    }
  }

  uptr onscreen_system::get_parameter(const umax param) const {
    switch (param)
    {
    case SYS_PARAM_X_DISPLAYNAME:
      return reinterpret_cast<uptr>(m_x_displayname);
    case SYS_PARAM_VULKAN_DEVICE_INDEX:
      return static_cast<uptr>(m_vulkan_device_index);
    case SYS_PARAM_VULKAN_REQUIRED_LAYERS:
      return reinterpret_cast<uptr>(m_vulkan_layers);
    case SYS_PARAM_VULKAN_REQUIRED_LAYER_COUNT:
      return static_cast<uptr>(m_vulkan_layer_count);
    case SYS_PARAM_VULKAN_DEBUG_MESSENGER_ENABLE:
      return static_cast<uptr>(m_vulkan_debug_messenger_enable);
    default:
      return bad_parameter;
    }
  }

  void onscreen_system::set_x_displayname(const cstring displayname) {
    m_x_displayname = displayname;
  }

  void onscreen_system::set_vulkan_device_index(const u32 device_index) {
    m_vulkan_device_index = device_index;
  }

  void onscreen_system::set_vulkan_required_layers(const readonly_ptr<cstring> layers, const u32 layer_count) {
    m_vulkan_layers = layers;
    m_vulkan_layer_count = layer_count;
  }

  void onscreen_system::set_vulkan_debug_messenger_enable(const bool enable) {
    m_vulkan_debug_messenger_enable = enable;
  }

  int onscreen_system::run(const ptr<entry_point> main) {
    try
    {
      auto ctx = new onscreen_context{ m_x_displayname, m_vulkan_device_index, m_vulkan_layers, m_vulkan_layer_count,
                                       m_vulkan_debug_messenger_enable };
      auto win = new render_window{ };
      auto res = main(*this, *ctx, *win);
      delete win;
      delete ctx;
      return res;
    }
    catch (const oberon::error& err)
    {
      std::cerr << err.message() << std::endl;
      return err.result();
    }
    catch (const std::exception& err)
    {
      std::cerr << err.what() << std::endl;
      return 1;
    }
  }

}
