/**
 * @file graphics_device.cpp
 * @brief Graphics device object implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/graphics_device.hpp"

#include <array>

#include "configuration.hpp"

#include "oberon/internal/base/graphics_device_impl.hpp"

#ifdef CONFIGURATION_OPERATING_SYSTEM_LINUX
  #include "oberon/internal/linux/utility.hpp"
#endif

namespace oberon {

  graphics_device::graphics_device(ptr<internal::base::graphics_device_impl>&& impl) :
  m_impl{ std::exchange(impl, nullptr) } { }

  graphics_device::implementation_type& graphics_device::implementation() {
    return *m_impl;
  }

  graphics_device_type graphics_device::type() const {
    return static_cast<graphics_device_type>(m_impl->physical_device().properties_1_0().deviceType);
  }

  std::string graphics_device::name() const {
    return m_impl->physical_device().properties_1_0().deviceName;
  }

  std::string graphics_device::driver_name() const {
    return m_impl->physical_device().properties_1_2().driverName;
  }

  std::string graphics_device::driver_info() const {
    return m_impl->physical_device().properties_1_2().driverInfo;
  }

  u32 graphics_device::vendor_id() const {
    return m_impl->physical_device().properties_1_0().vendorID;
  }

  u32 graphics_device::device_id() const {
    return m_impl->physical_device().properties_1_0().deviceID;
  }

  usize graphics_device::total_memory() const {
    const auto& memory_properties = m_impl->physical_device().memory_properties();
    auto result = usize{ };
    for (auto i = u32{ 0 }; i < memory_properties.memoryHeapCount; ++i)
    {
      const auto MASK = memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
      result += memory_properties.memoryHeaps[i].size & -!!MASK;
    }
    return result;
  }

  std::string graphics_device::uuid() const {
#ifdef CONFIGURATION_OPERATING_SYSTEM_LINUX
    auto buffer = std::array<char, 37>{ };
    uuid_unparse(m_impl->physical_device().properties_1_1().deviceUUID, buffer.data());
    return buffer.data();
#else
    return "";
#endif
  }

  std::string to_string(const graphics_device_type type) {
#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) case oberon::graphics_device_type::name: return #name;
    switch (type)
    {
    OBERON_GRAPHICS_DEVICE_TYPES
    default:
      return "";
    }
#undef OBERON_GRAPHICS_DEVICE_TYPE
  }
}
