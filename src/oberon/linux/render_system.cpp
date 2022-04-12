#include "oberon/linux/detail/render_system_impl.hpp"

#include <vector>

#include "oberon/debug.hpp"
#include "oberon/linux/detail/vulkan.hpp"

namespace {
  using oberon::u32;
  using oberon::linux::vk_device_create_failed_error;

  static inline std::tuple<VkDevice, u32, VkQueue> vk_create_device_common(const VkPhysicalDevice physical_device,
                                                                           VkDeviceCreateInfo& info, vkfl::loader& dl) {
    OBERON_DECLARE_VK_PFN(dl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    info.pQueueCreateInfos = &queue_info;
    info.queueCreateInfoCount = 1;
    auto device = VkDevice{ };
    OBERON_VK_SUCCEEDS(vkCreateDevice(physical_device, &info, nullptr, &device), vk_device_create_failed_error{ });
    dl.load(device);
    OBERON_DECLARE_VK_PFN(dl, GetDeviceQueue);
    auto work_queue = VkQueue{ };
    vkGetDeviceQueue(device, 0, 0, &work_queue);
    return { device, 0, work_queue };
  }

}

namespace oberon::linux::detail {

  VkInstance vk_create_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
                                const readonly_ptr<cstring> extensions, const u32 extension_count,
                                const ptr<void> next, vkfl::loader& dl) {
    auto app_info = VkApplicationInfo{ };
    app_info.sType = OBERON_VK_STRUCT(APPLICATION_INFO);
    app_info.apiVersion = VK_API_VERSION_1_2;
    auto instance_info = VkInstanceCreateInfo{ };
    instance_info.sType = OBERON_VK_STRUCT(INSTANCE_CREATE_INFO);
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledLayerNames = layers;
    instance_info.enabledLayerCount = layer_count;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount = extension_count;
    instance_info.pNext = next;
    OBERON_DECLARE_VK_PFN(dl, CreateInstance);
    auto instance = VkInstance{ };
    OBERON_VK_SUCCEEDS(vkCreateInstance(&instance_info, nullptr, &instance), vk_instance_create_failed_error{ });
    dl.load(instance);
    return instance;
  }

  VkDebugUtilsMessengerEXT vk_create_debug_utils_messenger(const VkInstance instance,
                                                           const VkDebugUtilsMessengerCreateInfoEXT& info,
                                                           const vkfl::loader& dl) {
    OBERON_PRECONDITION(instance != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() == instance);
    OBERON_DECLARE_VK_PFN(dl, CreateDebugUtilsMessengerEXT);
    auto debug_messenger = VkDebugUtilsMessengerEXT{ };
    OBERON_VK_SUCCEEDS(vkCreateDebugUtilsMessengerEXT(instance, &info, nullptr, &debug_messenger),
                       vk_debug_messenger_create_failed_error{ });
    return debug_messenger;
  }

  VkPhysicalDevice vk_select_physical_device(const u32 index, const VkInstance instance,
                                             const ptr<xcb_connection_t> connection, const ptr<xcb_screen_t> screen,
                                             const vkfl::loader& dl) {
    OBERON_PRECONDITION(instance != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() == instance);
    OBERON_PRECONDITION(connection != nullptr);
    OBERON_PRECONDITION(screen != nullptr);
    auto sz = u32{ 0 };
    OBERON_DECLARE_VK_PFN(dl, EnumeratePhysicalDevices);
    OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, nullptr),
                       vk_physical_device_enumeration_failed_error{ });
    auto physical_devices = std::vector<VkPhysicalDevice>(sz);
    OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(instance, &sz, std::data(physical_devices)),
                       vk_physical_device_enumeration_failed_error{ });
    auto suitable_physical_devices = std::vector<VkPhysicalDevice>{ };
    {
      OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceQueueFamilyProperties);
      OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceXcbPresentationSupportKHR);
      auto has_graphics = false;
      auto has_present = false;
      auto queue_families = std::vector<VkQueueFamilyProperties>{ };
      for (const auto physical_device : physical_devices)
      {
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, nullptr);
        queue_families.resize(sz);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, std::data(queue_families));
        auto counter = u32{ 0 };
        for (const auto& queue_family : queue_families)
        {
          has_graphics = has_graphics || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
          has_present = has_present || vkGetPhysicalDeviceXcbPresentationSupportKHR(physical_device, counter++,
                                                                                    connection, screen->root_visual);
        }
        if (has_graphics && has_present)
        {
          suitable_physical_devices.emplace_back(physical_device);
        }
      }
    }
    OBERON_INVARIANT(index < std::size(suitable_physical_devices), vk_no_such_physical_device_error{ });
    return suitable_physical_devices[index];
  }

  std::tuple<VkDevice, u32, VkQueue> vk_create_device(const VkPhysicalDevice physical_device,
                                                      const readonly_ptr<cstring> extensions, const u32 extension_count,
                                                      const ptr<void> next, vkfl::loader& dl) {
    OBERON_PRECONDITION(physical_device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() != VK_NULL_HANDLE);
    auto info = VkDeviceCreateInfo{ };
    info.sType = OBERON_VK_STRUCT(DEVICE_CREATE_INFO);
    info.ppEnabledExtensionNames = extensions;
    info.enabledExtensionCount = extension_count;
    auto features = VkPhysicalDeviceFeatures{ };
    OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceFeatures);
    vkGetPhysicalDeviceFeatures(physical_device, &features);
    info.pEnabledFeatures = &features;
    info.pNext = next;
    OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceProperties);
    auto properties = VkPhysicalDeviceProperties{ };
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    switch (properties.vendorID)
    {
    case 0x10de: // Nvidia
      return vk_create_device_nvidia(physical_device, info, dl);
    case 0x1002: // AMD
      return vk_create_device_amd(physical_device, info, dl);
    case 0x8086: // Intel
      return vk_create_device_intel(physical_device, info, dl);
    default:
      return vk_create_device_generic(physical_device, info, dl);
    }
  }

  // Nvidia devices present 16 Graphics/Compute/Transfer/Present queues in queue family 0
  std::tuple<VkDevice, u32, VkQueue> vk_create_device_nvidia(const VkPhysicalDevice physical_device,
                                                             VkDeviceCreateInfo& info, vkfl::loader& dl) {
    return vk_create_device_common(physical_device, info, dl);
  }

  // AMD devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  std::tuple<VkDevice, u32, VkQueue> vk_create_device_amd(const VkPhysicalDevice physical_device,
                                                          VkDeviceCreateInfo& info, vkfl::loader& dl) {
    return vk_create_device_common(physical_device, info, dl);
  }

  // Intel devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  // Might be different with regard to Iris vs UHD vs HD
  // Intel discrete graphics aren't available yet so I have no idea what those will look like.
  std::tuple<VkDevice, u32, VkQueue> vk_create_device_intel(const VkPhysicalDevice physical_device,
                                                            VkDeviceCreateInfo& info, vkfl::loader& dl) {
    return vk_create_device_common(physical_device, info, dl);
  }

  std::tuple<VkDevice, u32, VkQueue> vk_create_device_generic(const VkPhysicalDevice, VkDeviceCreateInfo&,
                                                              vkfl::loader&) {
    throw not_implemented_error{ };
  }

  void vk_destroy_device(const VkDevice device, vkfl::loader& dl) noexcept {
    if (device != VK_NULL_HANDLE)
    {
      OBERON_ASSERT(dl.loaded_device() == device);
      OBERON_DECLARE_VK_PFN(dl, DestroyDevice);
      vkDestroyDevice(device, nullptr);
      dl.unload_device();
    }
  }

  void vk_destroy_debug_messenger(const VkInstance instance, const VkDebugUtilsMessengerEXT debug_messenger,
                                  const vkfl::loader& dl) noexcept {
    if (instance != VK_NULL_HANDLE && debug_messenger != VK_NULL_HANDLE)
    {
      OBERON_ASSERT(dl.loaded_instance() == instance);
      OBERON_DECLARE_VK_PFN(dl, DestroyDebugUtilsMessengerEXT);
      vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
  }

  void vk_destroy_instance(const VkInstance instance, vkfl::loader& dl) noexcept {
    if (instance != VK_NULL_HANDLE)
    {
      OBERON_ASSERT(dl.loaded_instance() == instance);
      OBERON_DECLARE_VK_PFN(dl, DestroyInstance);
      vkDestroyInstance(instance, nullptr);
      dl.unload_instance();
    }
  }

}

namespace oberon::linux {

  render_system::render_system(const ptr<detail::render_system_impl> impl) {
    OBERON_PRECONDITION(impl != nullptr);
    OBERON_PRECONDITION(impl->dl != nullptr);
    OBERON_PRECONDITION(impl->dl->loaded_instance() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(impl->instance != VK_NULL_HANDLE);
    OBERON_PRECONDITION(impl->dl->loaded_instance() == impl->instance);
    OBERON_PRECONDITION(impl->physical_device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(impl->device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(impl->dl->loaded_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(impl->dl->loaded_device() == impl->device);
    OBERON_PRECONDITION(impl->work_queue != VK_NULL_HANDLE);
    m_impl = impl;
    OBERON_POSTCONDITION(m_impl != nullptr);
  }

}
