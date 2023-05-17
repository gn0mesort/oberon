#include "oberon/internal/linux/x11/system_impl.hpp"

#include <new>
#include <iostream>

#include "oberon/graphics_device.hpp"

#include "oberon/internal/base/vulkan.hpp"
#include "oberon/internal/base/graphics_context.hpp"
#include "oberon/internal/base/physical_graphics_device.hpp"

#include "oberon/internal/linux/utility.hpp"

#include "oberon/internal/linux/x11/xcb.hpp"
#include "oberon/internal/linux/x11/wsi_context.hpp"
#include "oberon/internal/linux/x11/graphics_device_impl.hpp"

#define VK_DECLARE_PFN(dl, name) OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, name)

namespace oberon::internal::linux::x11 {

  void system_impl::initialize_only(const basic_readonly_sequence<u8> uuid) {
    if (!uuid)
    {
      return;
    }
    uuid_copy(s_exclusive_device_uuid.data(), uuid);
  }

  system_impl::system_impl(ptr<wsi_context>&& wsi, ptr<base::graphics_context>&& gfx) :
  base::system_impl{ std::move(gfx) }, m_wsi_context{ std::exchange(wsi, nullptr) } {
    auto available_physical_devices = std::vector<base::physical_graphics_device>{ };
    for (const auto& handle : m_graphics_context->physical_devices())
    {
      constexpr auto HAS_COMPLETE_QUEUE = bitmask{ 0x1 };
      constexpr auto HAS_DYNAMIC_RENDERING = bitmask{ 0x2 };
      constexpr auto HAS_VULKAN_1_3 = bitmask{ 0x4 };
      constexpr auto IS_COMPLETE = HAS_COMPLETE_QUEUE | HAS_DYNAMIC_RENDERING | HAS_VULKAN_1_3;
      auto physical_device = base::physical_graphics_device{ m_graphics_context->dispatch_loader(), handle };
      auto bits = bitmask{ };
      const auto api_major = physical_device.properties_1_2().conformanceVersion.major;
      const auto api_minor = physical_device.properties_1_2().conformanceVersion.minor;
      bits |= HAS_VULKAN_1_3 & -(api_major == 1 && api_minor >= 3);
      bits |= HAS_DYNAMIC_RENDERING & -(physical_device.features_1_3().dynamicRendering);
      auto counter = u32{ 0 };
      for (const auto& queue_family : physical_device.queue_family_properties())
      {
        const auto graphics_transfer_supported = queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
        VK_DECLARE_PFN(m_graphics_context->dispatch_loader(), vkGetPhysicalDeviceXcbPresentationSupportKHR);
        const auto visual = m_wsi_context->default_screen()->root_visual;
        const auto present_supported = vkGetPhysicalDeviceXcbPresentationSupportKHR(physical_device.handle(), counter,
                                                                                    m_wsi_context->connection(),
                                                                                    visual);
        bits |= HAS_COMPLETE_QUEUE & -(graphics_transfer_supported && present_supported);
        ++counter;
      }
      const auto is_exclusive_device_mode = !uuid_is_null(s_exclusive_device_uuid.data());
      const auto is_exclusive_device = !uuid_compare(s_exclusive_device_uuid.data(),
                                                     physical_device.properties_1_1().deviceUUID);
      if (bits == IS_COMPLETE && ((is_exclusive_device_mode && is_exclusive_device) || !is_exclusive_device_mode))
      {
        std::cerr << physical_device.properties_1_0().deviceName << std::endl;
        available_physical_devices.push_back(physical_device);
      }
    }
    m_graphics_device_binary = new char[available_physical_devices.size() * sizeof(graphics_device)];
    m_graphics_device_count = available_physical_devices.size();
    {
      auto offset = usize{ 0 };
      auto impl = ptr<base::graphics_device_impl>{ };
      const auto required_extensions = std::unordered_set<std::string>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
      for (const auto& available_physical_device : available_physical_devices)
      {
        impl = new graphics_device_impl{ *m_wsi_context, *m_graphics_context, available_physical_device,
                                          required_extensions, { } };
        // This is slightly weird C++ esoterica.
        // new (placement-params) type{ constructor-params }
        // is the "placement new" operator. Essentially, this invokes
        // void* operator new(placement-params..., constructor-params...)
        // In this case, a new graphics device is constructed in the memory starting at the address
        // m_graphics_device_binary + offset. Here, this is advantageous because graphics_devices are immovable and
        // non-copyable. A std::vector would require at least move or copy construction. Instead, we simply construct
        // the object at the provided address (i.e., placing the object at the address).
        new (m_graphics_device_binary + offset) graphics_device{ std::move(impl) };
        offset += sizeof(graphics_device);
      }
    }
  }

  system_impl::~system_impl() noexcept {
    // As a result of using "placement new" to create the graphics_devices their destructors must be invoked
    // explicitly. If the destructors are not called any resources that they allocated will not be released when the
    // underlying memory (i.e., m_graphics_device_binary) is deleted.
    const auto p = reinterpret_cast<ptr<graphics_device>>(m_graphics_device_binary);
    for (auto i = usize{ 0 }; i < m_graphics_device_count; ++i)
    {
      p[i].~graphics_device();
    }
    delete[] m_graphics_device_binary;
    delete m_wsi_context;
  }

  std::span<graphics_device> system_impl::graphics_devices() {
    const auto p = reinterpret_cast<ptr<graphics_device>>(m_graphics_device_binary);
    return std::span<graphics_device>{ p, m_graphics_device_count };
  }

}
