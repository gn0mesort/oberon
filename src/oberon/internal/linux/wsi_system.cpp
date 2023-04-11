#include "oberon/internal/linux/wsi_system.hpp"

#include "oberon/internal/vulkan.hpp"
#include "oberon/internal/graphics_context.hpp"
#include "oberon/internal/graphics_device.hpp"
#include "oberon/internal/linux/wsi_graphics_device.hpp"

#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_VK_DECLARE_PFN(dl, cmd)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_VK_SUCCEEDS(exp)
#define VK_STRUCT(name) OBERON_INTERNAL_VK_STRUCT(name)

namespace oberon::internal {

  wsi_system::wsi_system(const std::unordered_set<std::string>& requested_layers) : system{ defer_construction{ } } {
    auto required_instance_extensions = std::unordered_set<std::string>{ VK_KHR_SURFACE_EXTENSION_NAME };
#ifdef MESON_SYSTEM_LINUX_WSI_TYPE_X11
    required_instance_extensions.insert(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#ifndef NDEBUG
    m_graphics_context.reset(new debug_graphics_context{ requested_layers, required_instance_extensions, { } });
#else
    m_graphics_context.reset(new graphics_context{ requested_layers, required_instance_extensions, { } });
#endif
    {
      const auto& dl = m_graphics_context->dispatch_loader();
      VK_DECLARE_PFN(dl, vkEnumeratePhysicalDevices);
      auto sz = u32{ };
      VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_graphics_context->instance(), &sz, nullptr));
      auto physical_devices = std::vector<VkPhysicalDevice>(sz);
      VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_graphics_context->instance(), &sz, physical_devices.data()));
      auto properties_11 = VkPhysicalDeviceVulkan11Properties{ };
      properties_11.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES);
      auto properties_12 = VkPhysicalDeviceVulkan12Properties{ };
      properties_12.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES);
      auto properties_13 = VkPhysicalDeviceVulkan13Properties{ };
      properties_13.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES);
      properties_11.pNext = &properties_12;
      properties_12.pNext = &properties_13;
      auto features_11 = VkPhysicalDeviceVulkan11Features{ };
      features_11.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
      auto features_12 = VkPhysicalDeviceVulkan12Features{ };
      features_12.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
      auto features_13 = VkPhysicalDeviceVulkan13Features{ };
      features_13.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
      features_11.pNext = &features_12;
      features_12.pNext = &features_13;
      auto major = u32{ };
      auto minor = u32{ };
      auto presentation_support = VkBool32{ };
      auto queue_families = std::vector<VkQueueFamilyProperties>{ };
      auto graphics_queue_family = i64{ -1 };
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties);
#ifdef MESON_SYSTEM_LINUX_WSI_TYPE_X11
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceXcbPresentationSupportKHR);
  #define VK_PRESENT_SUPPORT(device, family) \
      (vkGetPhysicalDeviceXcbPresentationSupportKHR(device, family, m_wsi_context.connection(), \
                                                    m_wsi_context.default_screen()->root_visual))
#endif
      for (const auto physical_device : physical_devices)
      {
        auto properties2 = graphics_device::query_properties(dl, physical_device, &properties_11);
        major = VK_API_VERSION_MAJOR(properties2.properties.apiVersion);
        minor = VK_API_VERSION_MINOR(properties2.properties.apiVersion);
        graphics_device::query_features(dl, physical_device, &features_11);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, nullptr);
        queue_families.resize(sz);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, queue_families.data());
        {
          auto index = 0;
          for (auto cur = queue_families.begin(); cur != queue_families.end() && graphics_queue_family < 0; ++cur)
          {
            graphics_queue_family = (index++) & -!!(cur->queueFlags & VK_QUEUE_GRAPHICS_BIT);
          }
        }
        if (!(major == 1 && minor >= 3) || graphics_queue_family < 0 || !features_13.dynamicRendering)
        {
          graphics_queue_family = -1;
          continue;
        }
        presentation_support = VK_PRESENT_SUPPORT(physical_device, graphics_queue_family);
        if (presentation_support)
        {
          m_graphics_device_ptrs.emplace_back(new wsi_graphics_device{ m_wsi_context, dl, physical_device,
                                                                       static_cast<u32>(graphics_queue_family),
                                                                       { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, { } });
          const auto wsi_dev = static_cast<ptr<wsi_graphics_device>>(m_graphics_device_ptrs.back().get());
          m_wsi_graphics_devices.emplace_back(wsi_dev);
        }
        else
        {
          m_graphics_device_ptrs.emplace_back(new graphics_device{ dl, physical_device,
                                                                   static_cast<u32>(graphics_queue_family), { },
                                                                   { } });
        }
        m_graphics_devices.emplace_back(m_graphics_device_ptrs.back().get());
      }
    }
  }


  const std::vector<oberon::wsi_graphics_device>& wsi_system::wsi_graphics_devices() {
    return m_wsi_graphics_devices;
  }

}
