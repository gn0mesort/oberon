#include "oberon/graphics_device.hpp"

#include "oberon/internal/graphics_device.hpp"
#include "oberon/internal/wsi_graphics_device.hpp"

namespace oberon {

  graphics_device::graphics_device(const ptr<internal::graphics_device> impl) : m_impl{ impl } { }

  graphics_device::implementation_reference graphics_device::internal() {
    return *m_impl;
  }

  bool graphics_device::empty() const {
    // i.e., m_impl != nullptr
    return m_impl;
  }

  wsi_graphics_device::wsi_graphics_device(const ptr<internal::wsi_graphics_device> impl) : graphics_device{ impl } { }

}
