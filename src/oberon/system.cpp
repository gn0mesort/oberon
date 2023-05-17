#include "oberon/system.hpp"

#include <algorithm>

#include "oberon/errors.hpp"
#include "oberon/graphics_device.hpp"

#include "oberon/internal/base/system_impl.hpp"

namespace {

  using namespace oberon::fundamental_types;
  using oberon::graphics_device;
  using oberon::graphics_device_type;

  inline usize score_device(const graphics_device& device) {
    auto result = usize{ 0 };
    // This doesn't work if device.total_memory() >= 4EiB. If you have a device with more than 3 EiB of
    // VRAM then feel free to change this to something more appropriate.
    // Realistically, prefer hardware (i.e., discrete or integrated devicess) over software (i.e., cpu, virtualized,
    // or other devices).
    switch (device.type())
    {
    case graphics_device_type::discrete:
      result |= usize{ 0x80'00'00'00'00'00'00'00 };
      break;
    case graphics_device_type::integrated:
      result |= usize{ 0x40'00'00'00'00'00'00'00 };
      break;
    default:
      break;
    }
    // Weight the preference in favor of devices with greater total VRAM.
    result += device.total_memory();
    return result;
  }

}

namespace oberon {

  system::system(ptr<implementation_type>&& impl) : m_impl{ std::exchange(impl, nullptr) } { }

  system::implementation_type& system::implementation() {
    return *m_impl;
  }

  std::span<graphics_device> system::graphics_devices() {
    return m_impl->graphics_devices();
  }

  graphics_device& system::preferred_graphics_device() {
    auto devices = m_impl->graphics_devices();
    OBERON_CHECK_ERROR_MSG(!devices.empty(), 1, "There are no available graphics devices.");
    return *std::max_element(devices.begin(), devices.end(), [](const auto& a, const auto& b) -> bool {
      return score_device(a) < score_device(b);
    });
  }

}
