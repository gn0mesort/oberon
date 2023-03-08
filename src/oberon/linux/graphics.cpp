#include "oberon/linux/graphics.hpp"

#include <unordered_set>

#include "oberon/debug.hpp"

#include "oberon/linux/system.hpp"
#include "oberon/linux/window.hpp"
#include "oberon/linux/vk.hpp"

namespace oberon::linux {

  graphics::queue_selection graphics::select_queues_heuristic(const graphics& gfx, const u32,
                                                              const VkPhysicalDevice device) {
    auto& dl = gfx.m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
    auto sz = u32{ };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &sz, nullptr);
    auto queue_family_properties = std::vector<VkQueueFamilyProperties>(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &sz, queue_family_properties.data());
    auto result = queue_selection{ };
    auto surface_support = VkBool32{ };
    auto gfx_found = false;
    auto present_found = false;
    for (auto i = usize{ 0 }; i < queue_family_properties.size() && (!gfx_found || !present_found); ++i)
    {
      if (!gfx_found && queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        result.graphics_queue = i;
        gfx_found = true;
      }
      OBERON_LINUX_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, gfx.m_target->surface(),
                                                                    &surface_support));
      if (!present_found && surface_support)
      {
        result.presentation_queue = i;
        present_found = true;
      }
    }
    OBERON_POSTCONDITION(gfx_found && present_found);
    return result;
  }

  graphics::queue_selection graphics::select_queues_amd(const graphics&, const u32 vendor, const VkPhysicalDevice) {
    OBERON_PRECONDITION(vendor == OBERON_LINUX_VK_PCI_VENDOR_ID_AMD);
    // AMD presents a Graphics/Transfer/Compute/Present queue family as family 0.
    return { 0, 0 };
  }

  graphics::queue_selection graphics::select_queues_nvidia(const graphics&, const u32 vendor, const VkPhysicalDevice) {
    OBERON_PRECONDITION(vendor == OBERON_LINUX_VK_PCI_VENDOR_ID_NVIDIA);
    // Nvidia is a little different from other vendors in that they provide 16 queues in family 0.
    // Otherwise though, it's similar to AMD where there is a Graphics/Transfer/Compute/Present queue family as
    // family 0
    return { 0, 0 };
  }

  graphics::queue_selection graphics::select_queues_intel(const graphics&, const u32 vendor, const VkPhysicalDevice) {
    OBERON_PRECONDITION(vendor == OBERON_LINUX_VK_PCI_VENDOR_ID_INTEL);
    // Intel only provides one queue family and it's a Graphics/Transfer/Compute/Present queue family.
    return { 0, 0 };
  }



  graphics::graphics(system& sys, window& win) : m_parent{ &sys }, m_target{ &win } {
    auto& dl = m_parent->vk_dl();
    auto instance = m_parent->instance();
    // Load physical devices.
    auto all_physical_devices = std::vector<VkPhysicalDevice>{ };
    {
      auto sz = u32{ };
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkEnumeratePhysicalDevices);
      OBERON_LINUX_VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, nullptr));
      all_physical_devices.resize(sz);
      OBERON_LINUX_VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, all_physical_devices.data()));
    }
    // Filter out physical devices lacking basic capabilities.
    {
      auto sz = u32{ };
      auto physical_device_properties2 = VkPhysicalDeviceProperties2{ };
      physical_device_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
      auto physical_device_properties12 = VkPhysicalDeviceVulkan12Properties{ };
      physical_device_properties12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
      physical_device_properties2.pNext = &physical_device_properties12;
      auto queue_family_properties2 = std::vector<VkQueueFamilyProperties2>{ };
      auto physical_device_features2 = VkPhysicalDeviceFeatures2{ };
      physical_device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      auto physical_device_features13 = VkPhysicalDeviceVulkan13Features{ };
      physical_device_features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
      physical_device_features2.pNext = &physical_device_features13;
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceProperties2);
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceQueueFamilyProperties2);
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceFeatures2);
      // In general, device selection is going to involve a lot of nested loops like this.
      // It's important to keep in mind that the length of many of these arrays will be 1, 2, or 3.
      // It is uncommon, for example, for consumer systems to have more than one actual discrete GPU. A system
      // running Mesa with integrated and discrete GPUs may report 3 (discrete GPU, integrated GPU, llvmpipe)
      // It's also uncommon for discrete GPUs to present dozens of acceptable queue families.
      // For example, my (AMD) GPU presents only one family capable of graphics operations and only one queue in that
      // family.
      // tl;dr Most of these loops will run 3 times max.
      for (const auto& physical_device : all_physical_devices)
      {
        vkGetPhysicalDeviceProperties2(physical_device, &physical_device_properties2);
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &sz, nullptr);
        queue_family_properties2.resize(sz);
        for (auto& queue_family_property : queue_family_properties2)
        {
          queue_family_property.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &sz, queue_family_properties2.data());
        // Check for Vulkan 1.3 support.
        auto major = VK_API_VERSION_MAJOR(physical_device_properties2.properties.apiVersion);
        auto minor = VK_API_VERSION_MINOR(physical_device_properties2.properties.apiVersion);
        // Check support for graphics and presentation on window surface.
        auto has_gfx_queue_family = false;
        auto has_present_queue_family = false;
        sz = 0;
        for (const auto& queue_family_property : queue_family_properties2)
        {
          const auto& properties = queue_family_property.queueFamilyProperties;
          has_gfx_queue_family = has_gfx_queue_family || properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
          auto surface_support = VkBool32{ };
          OBERON_LINUX_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, sz, m_target->surface(),
                                                                        &surface_support));
          has_present_queue_family = has_present_queue_family || surface_support;
          ++sz;
        }
        // Check support for dynamic rendering.
        vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features2);
        if ((major == 1 && minor < 3) || !physical_device_features13.dynamicRendering || !has_gfx_queue_family ||
            !has_present_queue_family)
        {
          continue;
        }
        m_physical_devices.emplace_back(physical_device);
        {
          auto type = static_cast<graphics_device_type>(physical_device_properties2.properties.deviceType);
          auto handle = reinterpret_cast<uptr>(&m_physical_devices.back());
          auto name = std::string{ physical_device_properties2.properties.deviceName };
          auto driver_name = std::string{ physical_device_properties12.driverName };
          auto driver_info = std::string{ physical_device_properties12.driverInfo };
          m_graphics_devices.emplace_back(graphics_device{ type, handle, name, driver_name, driver_info });
        }
      }
    }
    initialize_device(m_physical_devices.front());
  }

  void graphics::initialize_device(const VkPhysicalDevice device) {
    auto device_info = VkDeviceCreateInfo{ };
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    auto& dl = m_parent->vk_dl();
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceFeatures2);
    auto physical_device_features = VkPhysicalDeviceFeatures2{ };
    physical_device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    auto physical_device_features_1_1 = VkPhysicalDeviceVulkan11Features{ };
    physical_device_features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    auto physical_device_features_1_2 = VkPhysicalDeviceVulkan12Features{ };
    physical_device_features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    auto physical_device_features_1_3 = VkPhysicalDeviceVulkan13Features{ };
    physical_device_features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    physical_device_features.pNext = &physical_device_features_1_1;
    physical_device_features_1_1.pNext = &physical_device_features_1_2;
    physical_device_features_1_2.pNext = &physical_device_features_1_3;
    vkGetPhysicalDeviceFeatures2(device, &physical_device_features);
    device_info.pNext = &physical_device_features;
    device_info.pEnabledFeatures = nullptr;
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetPhysicalDeviceProperties);
    auto queue_selectors = std::array<queue_selection(*)(const graphics&, const u32, const VkPhysicalDevice), 4>{
      select_queues_heuristic, select_queues_amd, select_queues_nvidia, select_queues_intel
    };
    auto properties = VkPhysicalDeviceProperties{ };
    vkGetPhysicalDeviceProperties(device, &properties);
    // Branchless selection of correct vendor:
    // 0x1002 = AMD
    // 0x10de = Nvidia
    // 0x8086 = Intel
    // Anything else defaults to the heuristic approach.
    const auto index = (1 & -(properties.vendorID == OBERON_LINUX_VK_PCI_VENDOR_ID_AMD)) +
                       (2 & -(properties.vendorID == OBERON_LINUX_VK_PCI_VENDOR_ID_NVIDIA)) +
                       (3 & -(properties.vendorID == OBERON_LINUX_VK_PCI_VENDOR_ID_INTEL));
    m_selected_queue_families = queue_selectors[index](*this, properties.vendorID, device);
    auto multi_queue = m_selected_queue_families.graphics_queue != m_selected_queue_families.presentation_queue;
    // 1 + static_cast<usize>(multi_queue) is either 1 or 2.
    auto queue_infos = std::vector<VkDeviceQueueCreateInfo>(1 + multi_queue);
    auto priority = 1.0f;
    queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_infos[0].queueFamilyIndex = m_selected_queue_families.graphics_queue;
    queue_infos[0].queueCount = 1;
    queue_infos[0].pQueuePriorities = &priority;
    if (multi_queue)
    {
      queue_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_infos[1].queueFamilyIndex = m_selected_queue_families.presentation_queue;
      queue_infos[1].queueCount = 1;
      queue_infos[1].pQueuePriorities = &priority;
    }
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.queueCreateInfoCount = queue_infos.size();
    auto available_extensions = std::unordered_set<std::string>{ };
    {
      OBERON_LINUX_VK_DECLARE_PFN(dl, vkEnumerateDeviceExtensionProperties);
      auto sz = u32{ };
      OBERON_LINUX_VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(device, nullptr, &sz, nullptr));
      auto extension_properties = std::vector<VkExtensionProperties>(sz);
      OBERON_LINUX_VK_SUCCEEDS(vkEnumerateDeviceExtensionProperties(device, nullptr, &sz,
                                                                    extension_properties.data()));
      for (const auto& extension_property : extension_properties)
      {
        available_extensions.insert(extension_property.extensionName);
      }
    }
    // Select extensions and check that required extensions are available.
    auto selected_extensions = std::vector<cstring>{ };
    {
#define OBERON_LINUX_VK_REQUIRE_EXTENSION(ext) \
  do \
  { \
    if (!available_extensions.contains((ext))) \
    { \
      throw vk_error { "The selected Vulkan device does not support \"" ext "\"", 1 }; \
    } \
  } \
  while (0)
      OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      OBERON_LINUX_VK_REQUIRE_EXTENSION(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
      // This has to be true according to the specification
      // see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceCreateInfo.html
      available_extensions.erase(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
      if (available_extensions.contains(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
      {
        available_extensions.erase(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
      }
      // The specification lists a whole bunch of checks for when certain extensions are selected but the corresponding
      // feature is not enabled. I'm going to just assume (since I enable everything that is available) that
      // everything is fine.
#undef OBERON_LINUX_VK_REQUIRE_EXTENSION
      for (const auto& extension : available_extensions)
      {
        selected_extensions.emplace_back(extension.c_str());
      }
    }
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkCreateDevice);
    OBERON_LINUX_VK_SUCCEEDS(vkCreateDevice(device, &device_info, nullptr, &m_vk_device));
    dl.load(m_vk_device);
    OBERON_LINUX_VK_DECLARE_PFN(dl, vkGetDeviceQueue);
    vkGetDeviceQueue(m_vk_device, m_selected_queue_families.graphics_queue, 0, &m_vk_graphics_queue);
    vkGetDeviceQueue(m_vk_device, m_selected_queue_families.presentation_queue, 0, &m_vk_present_queue);
    m_vk_selected_physical_device = device;
  }

  void graphics::deinitialize_device() {
    OBERON_LINUX_VK_DECLARE_PFN(m_parent->vk_dl(), vkDestroyDevice);
    vkDestroyDevice(m_vk_device, nullptr);
  }

  graphics::~graphics() noexcept {
    wait_for_device_to_idle();
    deinitialize_device();
  }

  const std::vector<graphics_device>& graphics::available_devices() const {
    return m_graphics_devices;
  }

  void graphics::select_device(const graphics_device& device) {
    wait_for_device_to_idle();
    deinitialize_device();
    initialize_device(*reinterpret_cast<ptr<VkPhysicalDevice>>(device.handle));
  }

  void graphics::wait_for_device_to_idle() {
    OBERON_LINUX_VK_DECLARE_PFN(m_parent->vk_dl(), vkDeviceWaitIdle);
    vkDeviceWaitIdle(m_vk_device);
  }

}
