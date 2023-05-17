#include "oberon/internal/linux/x11/graphics_device_impl.hpp"

#include "oberon/errors.hpp"

#include "oberon/internal/base/vulkan.hpp"
#include "oberon/internal/base/graphics_context.hpp"

#include "oberon/internal/linux/x11/xcb.hpp"
#include "oberon/internal/linux/x11/wsi_context.hpp"

#define VK_STRUCT(name) OBERON_INTERNAL_BASE_VK_STRUCT(name)
#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, cmd)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::linux::x11 {

  graphics_device_impl::graphics_device_impl(wsi_context& wsi, base::graphics_context& gfx,
                                             const base::physical_graphics_device& physical_device,
                                             const std::unordered_set<std::string>& required_extensions,
                                             const std::unordered_set<std::string>& requested_extensions) :
  base::graphics_device_impl{ gfx, physical_device },
  m_wsi_context{ &wsi } {
    auto device_info = VkDeviceCreateInfo{ };
    device_info.sType = VK_STRUCT(DEVICE_CREATE_INFO);
    auto features2 = VkPhysicalDeviceFeatures2{ };
    features2.sType = VK_STRUCT(PHYSICAL_DEVICE_FEATURES_2);
    features2.features = m_physical_device.features_1_0();
    auto features_1_1 = m_physical_device.features_1_1();
    features2.pNext = &features_1_1;
    auto features_1_2 = m_physical_device.features_1_2();
    features_1_1.pNext = &features_1_2;
    auto features_1_3 = m_physical_device.features_1_3();
    features_1_2.pNext = &features_1_3;
    device_info.pNext = &features2;
    device_info.pEnabledFeatures = nullptr;
    m_complete_queue_family = select_queue_family();
    const auto priority = 1.0f;
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueFamilyIndex = m_complete_queue_family;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    const auto available = available_extensions();
    auto selected = std::vector<cstring>{ };
    for (const auto& extension : required_extensions)
    {
      OBERON_CHECK_ERROR_MSG(available.contains(extension), 1, "The \"%s\" extension, which was required, is not "
                             "available on on the device \"%s\".", extension.c_str(),
                             m_physical_device.properties_1_0().deviceName);
      selected.push_back(extension.c_str());
    }
    for (const auto& extension : requested_extensions)
    {
      if (available.contains(extension))
      {
        selected.push_back(extension.c_str());
      }
    }
    device_info.enabledExtensionCount = selected.size();
    device_info.ppEnabledExtensionNames = selected.data();
    VK_DECLARE_PFN(m_dl, vkCreateDevice);
    auto device = VkDevice{ };
    VK_SUCCEEDS(vkCreateDevice(m_physical_device.handle(), &device_info, nullptr, &device));
    m_dl.load(device);
    VK_DECLARE_PFN(m_dl, vkGetDeviceQueue);
    vkGetDeviceQueue(m_dl.loaded_device(), m_complete_queue_family, 0, &m_complete_queue);
    {
      auto allocator_info = VmaAllocatorCreateInfo{ };
      allocator_info.instance = m_dl.loaded_instance();
      allocator_info.physicalDevice = m_physical_device.handle();
      allocator_info.device = m_dl.loaded_device();
      auto vk_functions = VmaVulkanFunctions{ };
#define VKFL_GET(name) (reinterpret_cast<PFN_##name>(m_dl.get(vkfl::command::name)))
      vk_functions.vkGetInstanceProcAddr = VKFL_GET(vkGetInstanceProcAddr);
      vk_functions.vkGetDeviceProcAddr = VKFL_GET(vkGetDeviceProcAddr);
#undef VKFL_GET
      allocator_info.pVulkanFunctions = &vk_functions;
      allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
      VK_SUCCEEDS(vmaCreateAllocator(&allocator_info, &m_allocator));
    }
  }

  u32 graphics_device_impl::select_queue_family() {
    auto res = i64{ -1 };
    auto index = u32{ 0 };
    for (const auto& queue_family : m_physical_device.queue_family_properties())
    {
      const auto graphics_transfer_supported = queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
      VK_DECLARE_PFN(m_dl, vkGetPhysicalDeviceXcbPresentationSupportKHR);
      const auto visual = m_wsi_context->default_screen()->root_visual;
      const auto present_supported = vkGetPhysicalDeviceXcbPresentationSupportKHR(m_physical_device.handle(), index,
                                                                                  m_wsi_context->connection(),
                                                                                  visual);
      const auto condition = graphics_transfer_supported && present_supported;
       res = (res & -!(condition)) + (index & -(condition));
      ++index;
    }
    OBERON_CHECK_ERROR_MSG(res >= 0, 1, "On the device \"%s\" there was no queue supporting graphics, transfer, and "
                           "presentation operations.", m_physical_device.properties_1_0().deviceName);
    return res;
  }

  std::unordered_set<std::string> graphics_device_impl::available_extensions() {
    auto sz = u32{ };
    VK_DECLARE_PFN(m_dl, vkEnumerateDeviceExtensionProperties);
    VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(m_physical_device.handle(), nullptr, &sz, nullptr));
    auto properties = std::vector<VkExtensionProperties>(sz);
    VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(m_physical_device.handle(), nullptr, &sz, properties.data()));
    auto res = std::unordered_set<std::string>{ };
    for (const auto& property : properties)
    {
      res.insert(property.extensionName);
    }
    return res;
  }

  wsi_context& graphics_device_impl::wsi() {
    return *m_wsi_context;
  }

}
