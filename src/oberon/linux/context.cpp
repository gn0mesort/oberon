#include "oberon/linux/context.hpp"

#include <vector>

namespace oberon::linux {

  void onscreen_context::connect_to_x_server(const cstring displayname) {
    auto screen_pref = int{ 0 };
    m_x_connection = xcb_connect(displayname, &screen_pref);
    if (xcb_connection_has_error(m_x_connection))
    {
      throw x_connection_failed_error{ };
    }
    {
      auto setup = xcb_get_setup(m_x_connection);
      for (auto roots = xcb_setup_roots_iterator(setup); roots.rem; xcb_screen_next(&roots))
      {
        if (screen_pref-- == 0)
        {
          m_x_screen = roots.data;
        }
      }
      if (!m_x_screen)
      {
        throw x_no_screen_error{ };
      }
    }
  }

  void onscreen_context::create_vulkan_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
                                                const readonly_ptr<cstring> extensions, const u32 extension_count,
                                                const ptr<void> next) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateInstance);

    auto app_info = VkApplicationInfo{ };
    app_info.sType = OBERON_VK_STRUCT(APPLICATION_INFO);
    app_info.pEngineName = "oberon";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    auto instance_info = VkInstanceCreateInfo{ };
    instance_info.sType = OBERON_VK_STRUCT(INSTANCE_CREATE_INFO);
    instance_info.pNext = next;
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledLayerNames = layers;
    instance_info.enabledLayerCount = layer_count;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount = extension_count;

    OBERON_VK_SUCCEEDS(vkCreateInstance(&instance_info, nullptr, &m_vulkan_instance),
                       vulkan_instance_create_failed_error);
    m_vulkan_dl.load(m_vulkan_instance);
  }

  void onscreen_context::create_vulkan_debug_messenger(const VkDebugUtilsMessengerCreateInfoEXT& debug_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDebugUtilsMessengerEXT);
    OBERON_VK_SUCCEEDS(vkCreateDebugUtilsMessengerEXT(m_vulkan_instance, &debug_info, nullptr,
                                                      &m_vulkan_debug_messenger),
                       vulkan_debug_messenger_create_failed_error);
  }

  void onscreen_context::select_vulkan_physical_device(const u32 device_index) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, EnumeratePhysicalDevices);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetPhysicalDeviceQueueFamilyProperties);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetPhysicalDeviceXcbPresentationSupportKHR);
    auto sz = u32{ 0 };
    OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_vulkan_instance, &sz, nullptr),
                       vulkan_couldnt_enumerate_physical_devices_error);
    auto physical_devices = std::vector<VkPhysicalDevice>(sz);
    OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_vulkan_instance, &sz, std::data(physical_devices)),
                       vulkan_couldnt_enumerate_physical_devices_error);
    auto acceptable_physical_devices = std::vector<VkPhysicalDevice>{ };
    for (const auto& physical_device : physical_devices) // Probably runs 1 or 2 times
    {
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, nullptr);
      auto queue_families = std::vector<VkQueueFamilyProperties>(sz);
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, std::data(queue_families));
      auto has_present = false;
      auto has_graphics = false;
      auto counter = u32{ 0 };
      for (const auto& queue_family : queue_families)
      {
        has_present = has_present || vkGetPhysicalDeviceXcbPresentationSupportKHR(physical_device, counter++,
                                                                                  m_x_connection,
                                                                                  m_x_screen->root_visual);
        has_graphics = has_graphics || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
      }
      if (has_graphics && has_present)
      {
        acceptable_physical_devices.emplace_back(physical_device);
      }
    }
    m_vulkan_physical_device = acceptable_physical_devices.at(device_index);
  }

  void onscreen_context::create_vulkan_device(const readonly_ptr<cstring> extensions, const u32 extension_count,
                                              const ptr<void> next) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetPhysicalDeviceFeatures);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetPhysicalDeviceProperties);
    auto device_info = VkDeviceCreateInfo{ };
    device_info.sType = OBERON_VK_STRUCT(DEVICE_CREATE_INFO);
    device_info.pNext = next;
    device_info.ppEnabledExtensionNames = extensions;
    device_info.enabledExtensionCount = extension_count;
    auto features = VkPhysicalDeviceFeatures{ };
    vkGetPhysicalDeviceFeatures(m_vulkan_physical_device, &features);
    device_info.pEnabledFeatures = &features;
    auto properties = VkPhysicalDeviceProperties{ };
    vkGetPhysicalDeviceProperties(m_vulkan_physical_device, &properties);
    switch (properties.vendorID)
    {
    case 0x10de: // Nvidia
      create_vulkan_device_nvidia(device_info);
      break;
    case 0x1002: // AMD
      create_vulkan_device_amd(device_info);
      break;
    case 0x8086: // Intel
      create_vulkan_device_intel(device_info);
      break;
    default: // Generic
      create_vulkan_device_generic(device_info);
      break;
    }
  }

  // Nvidia devices present 16 Graphics/Compute/Transfer/Present queues in queue family 0
  void onscreen_context::create_vulkan_device_nvidia(VkDeviceCreateInfo& device_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = 1;
    OBERON_VK_SUCCEEDS(vkCreateDevice(m_vulkan_physical_device, &device_info, nullptr, &m_vulkan_device),
                       vulkan_device_create_failed_error);
    m_vulkan_dl.load(m_vulkan_device);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetDeviceQueue);
    vkGetDeviceQueue(m_vulkan_device, 0, 0, &m_vulkan_work_queue);
    m_vulkan_present_queue = m_vulkan_work_queue;
  }

  // AMD devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  void onscreen_context::create_vulkan_device_amd(VkDeviceCreateInfo& device_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = 1;
    OBERON_VK_SUCCEEDS(vkCreateDevice(m_vulkan_physical_device, &device_info, nullptr, &m_vulkan_device),
                       vulkan_device_create_failed_error);
    m_vulkan_dl.load(m_vulkan_device);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetDeviceQueue);
    vkGetDeviceQueue(m_vulkan_device, 0, 0, &m_vulkan_work_queue);
    m_vulkan_present_queue = m_vulkan_work_queue;
  }

  // Intel devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  // Might be different with regard to Iris vs UHD vs HD
  // Intel discrete graphics aren't available yet so I have no idea what those will look like.
  void onscreen_context::create_vulkan_device_intel(VkDeviceCreateInfo& device_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = 1;
    OBERON_VK_SUCCEEDS(vkCreateDevice(m_vulkan_physical_device, &device_info, nullptr, &m_vulkan_device),
                       vulkan_device_create_failed_error);
    m_vulkan_dl.load(m_vulkan_device);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetDeviceQueue);
    vkGetDeviceQueue(m_vulkan_device, 0, 0, &m_vulkan_work_queue);
    m_vulkan_present_queue = m_vulkan_work_queue;
  }

  void onscreen_context::create_vulkan_device_generic(VkDeviceCreateInfo&) {
    throw not_implemented_error{ };
  }

  void onscreen_context::destroy_vulkan_device() noexcept {
    if (m_vulkan_device != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(m_vulkan_dl, DestroyDevice);
      vkDestroyDevice(m_vulkan_device, nullptr);
      m_vulkan_device = VK_NULL_HANDLE;
    }
  }

  void onscreen_context::destroy_vulkan_debug_messenger() noexcept {
    if (m_vulkan_debug_messenger != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(m_vulkan_dl, DestroyDebugUtilsMessengerEXT);
      vkDestroyDebugUtilsMessengerEXT(m_vulkan_instance, m_vulkan_debug_messenger, nullptr);
      m_vulkan_debug_messenger = VK_NULL_HANDLE;
    }
  }

  void onscreen_context::destroy_vulkan_instance() noexcept {
    if (m_vulkan_instance != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(m_vulkan_dl, DestroyInstance);
      vkDestroyInstance(m_vulkan_instance, nullptr);
      m_vulkan_instance = VK_NULL_HANDLE;
    }
  }

  void onscreen_context::disconnect_from_x_server() noexcept {
    if (m_x_connection)
    {
      xcb_disconnect(m_x_connection);
      m_x_screen = nullptr;
      m_x_connection = nullptr;
    }
  }

  onscreen_context::onscreen_context(const cstring x_displayname, const u32 vulkan_device_index,
                                     const readonly_ptr<cstring> vulkan_layers, const u32 vulkan_layer_count,
                                     const bool vulkan_enable_debug_messenger) {
    connect_to_x_server(x_displayname);
    if (vulkan_enable_debug_messenger)
    {
      auto validation = VkValidationFeaturesEXT{ };
      validation.sType = OBERON_VK_STRUCT(VALIDATION_FEATURES_EXT);
      const auto validation_enables =
        std::array<VkValidationFeatureEnableEXT, 4>{ VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                                                     VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
                                                     VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
                                                     VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT };
      validation.pEnabledValidationFeatures = std::data(validation_enables);
      validation.enabledValidationFeatureCount = std::size(validation_enables);
      auto debug_info = VkDebugUtilsMessengerCreateInfoEXT{ };
      debug_info.sType = OBERON_VK_STRUCT(DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
      debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
      debug_info.pfnUserCallback = vkDebugLog;
      validation.pNext = &debug_info;
      const auto extensions = std::array<cstring, 4>{ VK_KHR_XCB_SURFACE_EXTENSION_NAME,
                                                      VK_KHR_SURFACE_EXTENSION_NAME,
                                                      VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
                                                      VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
      create_vulkan_instance(vulkan_layers, vulkan_layer_count, std::data(extensions), std::size(extensions),
                             &validation);
      create_vulkan_debug_messenger(debug_info);
    }
    else
    {
      auto extensions = std::array<cstring, 2>{ VK_KHR_XCB_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME };
      create_vulkan_instance(vulkan_layers, vulkan_layer_count, std::data(extensions), std::size(extensions), nullptr);
    }
    {
      const auto extensions = std::array<cstring, 1>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
      select_vulkan_physical_device(vulkan_device_index);
      create_vulkan_device(std::data(extensions), std::size(extensions), nullptr);
    }
  }

  onscreen_context::~onscreen_context() noexcept {
    destroy_vulkan_device();
    destroy_vulkan_debug_messenger();
    destroy_vulkan_instance();
    disconnect_from_x_server();
  }
}
