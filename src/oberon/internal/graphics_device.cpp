#include "oberon/internal/graphics_device.hpp"

#include "oberon/errors.hpp"

#define VK_DECLARE_PFN(dl, cmd) OBERON_INTERNAL_VK_DECLARE_PFN(dl, cmd)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_VK_SUCCEEDS(exp)
#define VK_STRUCT(name) OBERON_INTERNAL_VK_STRUCT(name)

namespace oberon::internal {

  VkPhysicalDeviceProperties2 graphics_device::query_properties(const vkfl::loader& dl,
                                                                const VkPhysicalDevice physical_device,
                                                                const ptr<void> next) {
    auto result = VkPhysicalDeviceProperties2{ };
    result.sType = VK_STRUCT(PHYSICAL_DEVICE_PROPERTIES_2);
    result.pNext = next;
    VK_DECLARE_PFN(dl, vkGetPhysicalDeviceProperties2);
    vkGetPhysicalDeviceProperties2(physical_device, &result);
    return result;
  }

  VkPhysicalDeviceFeatures2 graphics_device::query_features(const vkfl::loader& dl,
                                                            const VkPhysicalDevice physical_device,
                                                            const ptr<void> next) {
    auto result = VkPhysicalDeviceFeatures2{ };
    result.sType = VK_STRUCT(PHYSICAL_DEVICE_FEATURES_2);
    result.pNext = next;
    VK_DECLARE_PFN(dl, vkGetPhysicalDeviceFeatures2);
    vkGetPhysicalDeviceFeatures2(physical_device, &result);
    return result;
  }

  VkDeviceQueueCreateInfo graphics_device::pack_device_queue_info(const u32 queue_family, const u32 count,
                                                                  const readonly_ptr<f32> priorities) {
    auto result = VkDeviceQueueCreateInfo{ };
    result.sType = VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    result.queueFamilyIndex = queue_family;
    result.queueCount = count;
    result.pQueuePriorities = priorities;
    return result;
  }

  std::unordered_set<std::string> graphics_device::available_extensions(const vkfl::loader& dl,
                                                                        const VkPhysicalDevice physical_device) {
    auto sz = u32{ };
    VK_DECLARE_PFN(dl, vkEnumerateDeviceExtensionProperties);
    VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &sz, nullptr));
    auto extensions = std::vector<VkExtensionProperties>(sz);
    VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &sz, extensions.data()));
    auto result = std::unordered_set<std::string>{ };
    for (const auto& extension : extensions)
    {
      result.insert(extension.extensionName);
    }
    return result;
  }

  std::vector<cstring>
  graphics_device::select_extensions(std::unordered_set<std::string>& available_extensions,
                                     const std::unordered_set<std::string>& required_extensions,
                                     const std::unordered_set<std::string>& requested_extensions) {
    auto result = std::vector<cstring>{ };
    for (const auto& extension : required_extensions)
    {
      auto itr = available_extensions.find(extension);
      OBERON_CHECK_ERROR_MSG(itr != available_extensions.end(), 1, "The \"%s\" extension is required but not "
                             "supported by the Vulkan device.", extension.c_str());
      result.emplace_back(itr->c_str());
    }
    for (const auto& extension : requested_extensions)
    {
      auto itr = available_extensions.find(extension);
      if (itr != available_extensions.end())
      {
        result.emplace_back(itr->c_str());
      }
    }
    return result;
  }

  VkDeviceCreateInfo graphics_device::pack_device_info(const readonly_ptr<VkPhysicalDeviceFeatures> features,
                                                       const readonly_ptr<VkDeviceQueueCreateInfo> queue_infos,
                                                       const u32 queue_count, const readonly_ptr<cstring> extensions,
                                                       const u32 extension_count, const ptr<void> next) {
    auto result = VkDeviceCreateInfo{ };
    result.sType = VK_STRUCT(DEVICE_CREATE_INFO);
    result.pNext = next;
    result.pEnabledFeatures = features;
    result.pQueueCreateInfos = queue_infos,
    result.queueCreateInfoCount = queue_count;
    result.ppEnabledExtensionNames = extensions;
    result.enabledExtensionCount = extension_count;
    return result;
  }

  void graphics_device::intern_device_in_loader(vkfl::loader& dl, const VkDevice device) {
    dl.load(device);
  }

  VmaAllocatorCreateInfo graphics_device::pack_allocator_info(const vkfl::loader& dl,
                                                              const VkPhysicalDevice physical_device) {
    auto result = VmaAllocatorCreateInfo{ };
    result.instance = dl.loaded_instance();
    result.device = dl.loaded_device();
    result.physicalDevice = physical_device;
    result.vulkanApiVersion = VK_API_VERSION_1_3;
    auto vk_functions = VmaVulkanFunctions{ };
#define VKFL_GET(cmd) (reinterpret_cast<PFN_##cmd>(dl.get(vkfl::command::cmd)))
    vk_functions.vkGetInstanceProcAddr = VKFL_GET(vkGetInstanceProcAddr);
    vk_functions.vkGetDeviceProcAddr = VKFL_GET(vkGetDeviceProcAddr);
#undef VKFL_GET
    result.pVulkanFunctions = &vk_functions;
    return result;
  }

  graphics_device::graphics_device(const defer_construction&) { }

  graphics_device::graphics_device(const vkfl::loader& dl, const VkPhysicalDevice physical_device,
                                   const u32 graphics_queue_family,
                                   const std::unordered_set<std::string>& required_extensions,
                                   const std::unordered_set<std::string>& requested_extensions) :
  m_dl{ dl }, m_physical_device{ physical_device }, m_graphics_queue_family{ graphics_queue_family } {
    m_features_11.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
    m_features_12.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
    m_features_13.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
    m_features_11.pNext = &m_features_12;
    m_features_12.pNext = &m_features_13;
    auto features2 = query_features(m_dl, m_physical_device, &m_features_11);
    auto priority = 1.0f;
    auto queue_info = pack_device_queue_info(graphics_queue_family, 1, &priority);
    auto extensions = available_extensions(m_dl, m_physical_device);
    auto selected_extensions = select_extensions(extensions, required_extensions, requested_extensions);
    auto device_info = pack_device_info(nullptr, &queue_info, 1, selected_extensions.data(),
                                        selected_extensions.size(), &features2);
    VK_DECLARE_PFN(m_dl, vkCreateDevice);
    auto device = VkDevice{ };
    VK_SUCCEEDS(vkCreateDevice(m_physical_device, &device_info, nullptr, &device));
    intern_device_in_loader(m_dl, device);
    VK_DECLARE_PFN(m_dl, vkGetDeviceQueue);
    vkGetDeviceQueue(m_dl.loaded_device(), m_graphics_queue_family, 0, &m_graphics_queue);
    auto allocator_info = pack_allocator_info(m_dl, m_physical_device);
    VK_SUCCEEDS(vmaCreateAllocator(&allocator_info, &m_allocator));
    m_properties_11.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES);
    m_properties_12.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES);
    m_properties_13.sType = VK_STRUCT(PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES);
    m_properties_11.pNext = &m_properties_12;
    m_properties_12.pNext = &m_properties_13;
    auto properties2 = query_properties(m_dl, m_physical_device, &m_properties_11);
    // Unpack and unlink physical device info.
    m_properties = properties2.properties;
    m_properties_11.pNext = nullptr;
    m_properties_12.pNext = nullptr;
    m_properties_13.pNext = nullptr;
    m_features = features2.features;
    m_features_11.pNext = nullptr;
    m_features_12.pNext = nullptr;
    m_features_13.pNext = nullptr;
  }

  graphics_device::~graphics_device() noexcept {
    vmaDestroyAllocator(m_allocator);
    VK_DECLARE_PFN(m_dl, vkDestroyDevice);
    vkDestroyDevice(m_dl.loaded_device(), nullptr);
  }

}
