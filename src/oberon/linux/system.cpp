#include "oberon/detail/linux/system_impl.hpp"

#include "oberon/linux/context.hpp"


namespace oberon::detail {
  void system_impl_dtor::operator()(const ptr<system_impl> p) const noexcept {
    delete p;
  }
}

namespace oberon {

  system::system() : m_impl{ new detail::system_impl{ } } { }

  void system::set_hint(const hint ht, const uptr value) {
    switch (ht)
    {
    case hint::debug_context:
      m_impl->is_debug = value;
      break;
    case hint::x_display:
      m_impl->x_display_string = reinterpret_cast<cstring>(value);
      break;
    case hint::vulkan_device_index:
      m_impl->vulkan_device_index = value;
      break;
    default:
      break;
    }
  }

  uptr system::get_hint(const hint ht) const {
    switch (ht)
    {
    case hint::debug_context:
      return m_impl->is_debug;
    case hint::x_display:
      return reinterpret_cast<uptr>(m_impl->x_display_string);
    case hint::vulkan_device_index:
      return m_impl->vulkan_device_index;
    default:
      return bad_hint;
    }
  }

  int system::run(const ptr<entry_point> main) {
    auto x_conf = x_configuration{ };
    x_conf.displayname = m_impl->x_display_string;
    auto vulkan_conf = vulkan_configuration{ };
    if (m_impl->is_debug)
    {
      vulkan_conf.layers = std::data(detail::system_impl::debug_vulkan_layers);
      vulkan_conf.layer_count = std::size(detail::system_impl::debug_vulkan_layers);
      vulkan_conf.require_debug_messenger = true;
    }
    vulkan_conf.device_index = m_impl->vulkan_device_index;
    auto ctx = context{ x_conf, vulkan_conf };
    return main(*this, ctx);
  }

}
