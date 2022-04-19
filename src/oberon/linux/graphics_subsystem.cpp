#include "oberon/linux/graphics_subsystem.hpp"

#include <array>

#include "oberon/debug.hpp"

#include "oberon/linux/io_subsystem.hpp"

namespace {

}

namespace oberon::linux {

  void graphics_subsystem::open_vk_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
                                            const readonly_ptr<cstring> extensions, const u32 extension_count,
                                            const ptr<void> next) {
    OBERON_PRECONDITION((!layer_count && !layers) || (layer_count && layers));
    OBERON_PRECONDITION((!extension_count && !extensions) || (extension_count && extensions));
    OBERON_PRECONDITION(m_vkdl.loaded_instance() == VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_vkdl.loaded_device() == VK_NULL_HANDLE);
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
    OBERON_DECLARE_VK_PFN(m_vkdl, CreateInstance);
    auto instance = VkInstance{ };
    OBERON_VK_SUCCEEDS(vkCreateInstance(&instance_info, nullptr, &instance), vk_create_instance_failed_error{ });
    m_vkdl.load(instance);
    OBERON_POSTCONDITION(m_vkdl.loaded_instance() == instance);
  }

  void graphics_subsystem::open_vk_debug_utils_messenger(const VkDebugUtilsMessengerCreateInfoEXT& debug_info) {
    OBERON_PRECONDITION(m_vkdl.loaded_instance() != VK_NULL_HANDLE);
    OBERON_DECLARE_VK_PFN(m_vkdl, CreateDebugUtilsMessengerEXT);
    OBERON_VK_SUCCEEDS(vkCreateDebugUtilsMessengerEXT(m_vkdl.loaded_instance(), &debug_info, nullptr,
                                                      &m_debug_messenger), vk_create_debug_messenger_failed_error{ });
  }

  void graphics_subsystem::open_vk_physical_devices(io_subsystem& io) {
    OBERON_PRECONDITION(m_vkdl.loaded_instance() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(!std::size(m_available_physical_devices));
    OBERON_DECLARE_VK_PFN(m_vkdl, EnumeratePhysicalDevices);
    auto sz = u32{ 0 };
    OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_vkdl.loaded_instance(), &sz, nullptr),
                       vk_enumerate_physical_devices_failed_error{ });
    auto all_physical_devices = std::vector<VkPhysicalDevice>(sz);
    OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_vkdl.loaded_instance(), &sz, std::data(all_physical_devices)),
                       vk_enumerate_physical_devices_failed_error{ });
    OBERON_DECLARE_VK_PFN(m_vkdl, GetPhysicalDeviceQueueFamilyProperties);
    OBERON_DECLARE_VK_PFN(m_vkdl, GetPhysicalDeviceXcbPresentationSupportKHR);
    auto queue_families = std::vector<VkQueueFamilyProperties>{ };
    for (const auto& physical_device : all_physical_devices)
    {
      auto has_primary_queue = false;
      auto queue_family_index = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, nullptr);
      queue_families.resize(sz);
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, std::data(queue_families));
      for (const auto& queue_family : queue_families)
      {
        const auto queue_flags = (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                                 (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT);
        const auto present_flag = vkGetPhysicalDeviceXcbPresentationSupportKHR(physical_device, queue_family_index++,
                                                                               io.x_connection(),
                                                                               io.x_screen()->root_visual);
        has_primary_queue = has_primary_queue || (queue_flags && present_flag);
      }
      if (has_primary_queue)
      {
        m_available_physical_devices.emplace_back(physical_device);
      }
    }
    OBERON_INVARIANT(std::size(m_available_physical_devices), vk_no_such_physical_device_error{ });
  }

  void graphics_subsystem::open_vk_selected_physical_device(const u32 index) {
    OBERON_PRECONDITION(std::size(m_available_physical_devices));
    OBERON_PRECONDITION(m_physical_device == VK_NULL_HANDLE);
    OBERON_INVARIANT(index < std::size(m_available_physical_devices), vk_bad_device_index_error{ });
    m_physical_device = m_available_physical_devices[index];
    OBERON_POSTCONDITION(m_physical_device != VK_NULL_HANDLE);
  }

  void graphics_subsystem::open_vk_device(const readonly_ptr<cstring> extensions, const u32 extension_count,
                                          const ptr<void> next) {
    OBERON_PRECONDITION(m_physical_device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_vkdl.loaded_instance() != VK_NULL_HANDLE);
    auto info = VkDeviceCreateInfo{ };
    info.sType = OBERON_VK_STRUCT(DEVICE_CREATE_INFO);
    info.ppEnabledExtensionNames = extensions;
    info.enabledExtensionCount = extension_count;
    auto features = VkPhysicalDeviceFeatures{ };
    OBERON_DECLARE_VK_PFN(m_vkdl, GetPhysicalDeviceFeatures);
    vkGetPhysicalDeviceFeatures(m_physical_device, &features);
    info.pEnabledFeatures = &features;
    info.pNext = next;
    OBERON_DECLARE_VK_PFN(m_vkdl, GetPhysicalDeviceProperties);
    auto properties = VkPhysicalDeviceProperties{ };
    vkGetPhysicalDeviceProperties(m_physical_device, &properties);
    switch (properties.vendorID)
    {
    case 0x10de: // Nvidia
      open_vk_device_nvidia(info);
      break;
    case 0x1002: // AMD
      open_vk_device_amd(info);
      break;
    case 0x8086: // Intel
      open_vk_device_intel(info);
      break;
    default:
      open_vk_device_generic(info);
      break;
    }
  }

  // Nvidia devices present 16 Graphics/Compute/Transfer/Present queues in queue family 0
  void graphics_subsystem::open_vk_device_nvidia(VkDeviceCreateInfo& info) {
    return open_vk_device_common(info);
  }

  // AMD devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  void graphics_subsystem::open_vk_device_amd(VkDeviceCreateInfo& info) {
    return open_vk_device_common(info);
  }

  // Intel devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  // Might be different with regard to Iris vs UHD vs HD
  // Intel discrete graphics aren't available yet so I have no idea what those will look like.
  void graphics_subsystem::open_vk_device_intel(VkDeviceCreateInfo& info) {
    return open_vk_device_common(info);
  }

  void graphics_subsystem::open_vk_device_generic(VkDeviceCreateInfo&) {
    throw not_implemented_error{ };
  }

  void graphics_subsystem::open_vk_device_common(VkDeviceCreateInfo& info) {
    OBERON_DECLARE_VK_PFN(m_vkdl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    info.pQueueCreateInfos = &queue_info;
    info.queueCreateInfoCount = 1;
    auto device = VkDevice{ };
    OBERON_VK_SUCCEEDS(vkCreateDevice(m_physical_device, &info, nullptr, &device), vk_create_device_failed_error{ });
    m_vkdl.load(device);
    OBERON_DECLARE_VK_PFN(m_vkdl, GetDeviceQueue);
    m_primary_queue_family = 0;
    vkGetDeviceQueue(device, 0, 0, &m_primary_queue);
  }

  void graphics_subsystem::close_vk_device() noexcept {
    OBERON_DECLARE_VK_PFN(m_vkdl, DestroyDevice);
    if (m_vkdl.loaded_device() != VK_NULL_HANDLE)
    {
      vkDestroyDevice(m_vkdl.loaded_device(), nullptr);
      m_vkdl.unload_device();
    }
  }

  void graphics_subsystem::close_vk_selected_physical_device() noexcept {
    m_physical_device = VK_NULL_HANDLE;
  }

  void graphics_subsystem::close_vk_physical_devices() noexcept {
    m_available_physical_devices.clear();
  }

  void graphics_subsystem::close_vk_debug_utils_messenger() noexcept {
    OBERON_PRECONDITION(m_vkdl.loaded_instance() != VK_NULL_HANDLE);
    if (m_debug_messenger != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(m_vkdl, DestroyDebugUtilsMessengerEXT);
      vkDestroyDebugUtilsMessengerEXT(m_vkdl.loaded_instance(), m_debug_messenger, nullptr);
      m_debug_messenger = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(m_debug_messenger == VK_NULL_HANDLE);
  }

  void graphics_subsystem::close_vk_instance() noexcept {
    OBERON_PRECONDITION(m_vkdl.loaded_device() == VK_NULL_HANDLE);
    if (m_vkdl.loaded_instance() != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(m_vkdl, DestroyInstance);
      vkDestroyInstance(m_vkdl.loaded_instance(), nullptr);
      m_vkdl.unload_instance();
    }
  }

  graphics_subsystem::graphics_subsystem(io_subsystem& io, const u32 device_index, const bool create_debug_instance) {
    if (create_debug_instance)
    {
      auto layers = std::array<cstring, 1>{ "VK_LAYER_KHRONOS_validation" };
      auto extensions = std::array<cstring, 4>{ VK_KHR_XCB_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME,
                                                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                                                VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME };
      auto validation_features = VkValidationFeaturesEXT{ };
      validation_features.sType = OBERON_VK_STRUCT(VALIDATION_FEATURES_EXT);
      auto validation_feature_enables =
        std::array<VkValidationFeatureEnableEXT, 4>{ VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                                                     VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
                                                     VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
                                                     VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT };
      validation_features.pEnabledValidationFeatures = std::data(validation_feature_enables);
      validation_features.enabledValidationFeatureCount = std::size(validation_feature_enables);
      auto debug_info = VkDebugUtilsMessengerCreateInfoEXT{ };
      debug_info.sType = OBERON_VK_STRUCT(DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
      debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
      debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
      debug_info.pfnUserCallback = vkDebugLog;
      validation_features.pNext = &debug_info;
      open_vk_instance(std::data(layers), std::size(layers), std::data(extensions), std::size(extensions),
                       &validation_features);
      open_vk_debug_utils_messenger(debug_info);
    }
    else
    {
      auto extensions = std::array<cstring, 2>{ VK_KHR_XCB_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME };
      open_vk_instance(nullptr, 0, std::data(extensions), std::size(extensions), nullptr);
    }
    open_vk_physical_devices(io);
    open_vk_selected_physical_device(device_index);
    {
      auto extensions = std::array<cstring, 1>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
      open_vk_device(std::data(extensions), std::size(extensions), nullptr);
    }
  }

  graphics_subsystem::~graphics_subsystem() noexcept {
    close_vk_device();
    close_vk_selected_physical_device();
    close_vk_physical_devices();
    close_vk_debug_utils_messenger();
    close_vk_instance();
  }

}
