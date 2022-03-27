#include "oberon/linux/context.hpp"

#include <cstdio>

#include <vector>

extern "C" VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData) {
  auto file = stdout;
  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
      messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    file = stderr;
  }
  std::fprintf(file, "[VK] %s\n", pCallbackData->pMessage);
  return VK_FALSE;
}

namespace oberon {
  void context::connect_to_x_server(const cstring displayname) {
    auto screen_pref = int{ 0 };
    m_x_connection = xcb_connect(displayname, &screen_pref);
    if (xcb_connection_has_error(m_x_connection))
    {
      // TODO throw error
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
        // TODO throw error
      }
    }
  }

  void context::create_vulkan_instance(const readonly_ptr<cstring> layers, const u32 layer_count,
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

    if (auto res = vkCreateInstance(&instance_info, nullptr, &m_vulkan_instance); res != VK_SUCCESS)
    {
      // TODO throw error
    }
    m_vulkan_dl.load(m_vulkan_instance);
  }

  void context::create_vulkan_debug_messenger(const VkDebugUtilsMessengerCreateInfoEXT& debug_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDebugUtilsMessengerEXT);
    if (auto res = vkCreateDebugUtilsMessengerEXT(m_vulkan_instance, &debug_info, nullptr, &m_vulkan_debug_messenger);
        res != VK_SUCCESS)
    {
      // TODO throw
    }
  }

  void context::select_vulkan_physical_device(const u32 device_index) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, EnumeratePhysicalDevices);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetPhysicalDeviceQueueFamilyProperties);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetPhysicalDeviceXcbPresentationSupportKHR);
    auto sz = u32{ 0 };
    if (auto res = vkEnumeratePhysicalDevices(m_vulkan_instance, &sz, nullptr); res != VK_SUCCESS)
    {
      // TODO throw
    }
    auto physical_devices = std::vector<VkPhysicalDevice>(sz);
    if (auto res = vkEnumeratePhysicalDevices(m_vulkan_instance, &sz, std::data(physical_devices)); res != VK_SUCCESS)
    {
      // TODO throw
    }
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

  void context::create_vulkan_device(const readonly_ptr<cstring> extensions, const u32 extension_count,
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
  void context::create_vulkan_device_nvidia(VkDeviceCreateInfo& device_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = m_vulkan_unique_queues = 1;
    if (auto res = vkCreateDevice(m_vulkan_physical_device, &device_info, nullptr, &m_vulkan_device);
        res != VK_SUCCESS)
    {
      // TODO throw
    }
    m_vulkan_dl.load(m_vulkan_device);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetDeviceQueue);
    vkGetDeviceQueue(m_vulkan_device, 0, 0, &m_vulkan_graphics_queue);
    m_vulkan_transfer_queue = m_vulkan_present_queue = m_vulkan_graphics_queue;
  }

  // AMD devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  void context::create_vulkan_device_amd(VkDeviceCreateInfo& device_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = m_vulkan_unique_queues = 1;
    if (auto res = vkCreateDevice(m_vulkan_physical_device, &device_info, nullptr, &m_vulkan_device);
        res != VK_SUCCESS)
    {
      // TODO throw
    }
    m_vulkan_dl.load(m_vulkan_device);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetDeviceQueue);
    vkGetDeviceQueue(m_vulkan_device, 0, 0, &m_vulkan_graphics_queue);
    m_vulkan_transfer_queue = m_vulkan_present_queue = m_vulkan_graphics_queue;
  }

  // Intel devices present 1 Graphics/Compute/Transfer/Present queue in queue family 0
  // Might be different with regard to Iris vs UHD vs HD
  // Intel discrete graphics aren't available yet so I have no idea what those will look like.
  void context::create_vulkan_device_intel(VkDeviceCreateInfo& device_info) {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, CreateDevice);
    auto queue_info = VkDeviceQueueCreateInfo{ };
    queue_info.sType = OBERON_VK_STRUCT(DEVICE_QUEUE_CREATE_INFO);
    auto priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = 0;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = m_vulkan_unique_queues = 1;
    if (auto res = vkCreateDevice(m_vulkan_physical_device, &device_info, nullptr, &m_vulkan_device);
        res != VK_SUCCESS)
    {
      // TODO throw
    }
    m_vulkan_dl.load(m_vulkan_device);
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, GetDeviceQueue);
    vkGetDeviceQueue(m_vulkan_device, 0, 0, &m_vulkan_graphics_queue);
    m_vulkan_transfer_queue = m_vulkan_present_queue = m_vulkan_graphics_queue;
  }

  void context::create_vulkan_device_generic(VkDeviceCreateInfo& device_info) {
    // TODO implementation
  }

  void context::destroy_vulkan_device() noexcept {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, DestroyDevice);
    vkDestroyDevice(m_vulkan_device, nullptr);
  }

  void context::destroy_vulkan_debug_messenger() noexcept {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, DestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(m_vulkan_instance, m_vulkan_debug_messenger, nullptr);
    m_vulkan_debug_messenger = VK_NULL_HANDLE;
  }

  void context::destroy_vulkan_instance() noexcept {
    OBERON_DECLARE_VK_PFN(m_vulkan_dl, DestroyInstance);
    vkDestroyInstance(m_vulkan_instance, nullptr);
    m_vulkan_instance = VK_NULL_HANDLE;
  }

  void context::disconnect_from_x_server() noexcept {
    xcb_disconnect(m_x_connection);
    m_x_screen = nullptr;
    m_x_connection = nullptr;
  }

  context::context(const x_configuration& x_conf, const vulkan_configuration& vulkan_conf) {
    connect_to_x_server(x_conf.displayname);
    if (vulkan_conf.require_debug_messenger)
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
      // idea: multiply &validation by vulkan_conf.require_debug_messenger instead of the condition lol
      create_vulkan_instance(vulkan_conf.layers, vulkan_conf.layer_count, std::data(extensions), std::size(extensions),
                             &validation);
      create_vulkan_debug_messenger(debug_info);
    }
    else
    {
      const auto extensions = std::array<cstring, 2>{ VK_KHR_XCB_SURFACE_EXTENSION_NAME,
                                                      VK_KHR_SURFACE_EXTENSION_NAME };
      create_vulkan_instance(vulkan_conf.layers, vulkan_conf.layer_count, std::data(extensions), std::size(extensions),
                             nullptr);
    }
    {
      const auto extensions = std::array<cstring, 1>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
      select_vulkan_physical_device(vulkan_conf.device_index);
      create_vulkan_device(std::data(extensions), std::size(extensions), nullptr);
    }
  }

  context::~context() noexcept {
    destroy_vulkan_device();
    if (m_vulkan_debug_messenger != VK_NULL_HANDLE)
    {
      destroy_vulkan_debug_messenger();
    }
    destroy_vulkan_instance();
    disconnect_from_x_server();
  }

}
