#include "oberon/detail/graphics_subsystem.hpp"

#include <vector>

#include "oberon/debug.hpp"

#include "oberon/detail/x11.hpp"
#include "oberon/detail/io_subsystem.hpp"

namespace oberon::detail {

  // Pre: no instance, instance unloaded
  // Post: instance, debugger (ifndef NDEBUG), instance loaded
  void graphics_subsystem::open_vk_instance() {
    OBERON_PRECONDITION(m_vkdl.loaded_instance() == VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_instance == VK_NULL_HANDLE);
    auto app_info = VkApplicationInfo{ };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_3;
    auto instance_info = VkInstanceCreateInfo{ };
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
#ifndef NDEBUG
  #define LAYER_COUNT 1
  #define LAYER_STRINGS "VK_LAYER_KHRONOS_validation"
  #define LAYERS \
    auto layers = std::array<cstring, LAYER_COUNT>{ LAYER_STRINGS }; \
    instance_info.ppEnabledLayerNames = std::data(layers); \
    instance_info.enabledLayerCount = std::size(layers)
  #define EXTENSION_COUNT 4
  #define EXTENSION_STRINGS \
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME, \
    VK_KHR_XCB_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME
  #define EXTENSIONS \
    auto extensions = std::array<cstring, EXTENSION_COUNT>{ EXTENSION_STRINGS }; \
    instance_info.ppEnabledExtensionNames = std::data(extensions); \
    instance_info.enabledExtensionCount = std::size(extensions)
  #define PNEXT \
    auto debug_messenger_info = VkDebugUtilsMessengerCreateInfoEXT{ }; \
    debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; \
    debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | \
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | \
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT; \
    debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | \
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | \
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | \
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; \
    debug_messenger_info.pfnUserCallback = vkDebugLog; \
    auto validation_features = VkValidationFeaturesEXT{ }; \
    validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT; \
    validation_features.pNext = &debug_messenger_info; \
    auto validation_feature_enables = std::array<VkValidationFeatureEnableEXT, 4>{ \
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, \
    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT, \
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT }; \
    validation_features.pEnabledValidationFeatures = std::data(validation_feature_enables); \
    validation_features.enabledValidationFeatureCount = std::size(validation_feature_enables); \
    instance_info.pNext = &validation_features
  #define CREATE_DEBUGGER \
    OBERON_DECLARE_VK_PFN(m_vkdl, CreateDebugUtilsMessengerEXT); \
    OBERON_VK_SUCCEEDS(vkCreateDebugUtilsMessengerEXT(m_instance, &debug_messenger_info, nullptr, \
    &m_debug_messenger), oberon::errors::vk_create_debug_messenger_failed_error{ })
  #define DEBUGGER_VALID (m_debug_messenger != VK_NULL_HANDLE)
#else
  #define LAYERS ((void) 0)
  #define EXTENSION_COUNT 2
  #define EXTENSION_STRINGS \
    VK_KHR_XCB_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME
  #define EXTENSIONS \
    auto extensions = std::array<cstring, EXTENSION_COUNT>{ EXTENSION_STRINGS }; \
    instance_info.ppEnabledExtensionNames = std::data(extensions); \
    instance_info.enabledExtensionCount = std::size(extensions)
  #define PNEXT ((void) 0)
  #define CREATE_DEBUGGER ((void) 0)
  #define DEBUGGER_VALID true
#endif
    LAYERS;
    EXTENSIONS;
    PNEXT;
    OBERON_DECLARE_VK_PFN(m_vkdl, CreateInstance);
    OBERON_VK_SUCCEEDS(vkCreateInstance(&instance_info, nullptr, &m_instance),
                       oberon::errors::vk_create_instance_failed_error{ });
    m_vkdl.load(m_instance);
    CREATE_DEBUGGER;
    OBERON_POSTCONDITION(m_instance != VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_vkdl.loaded_instance() == m_instance);
    OBERON_POSTCONDITION(DEBUGGER_VALID);
#undef LAYER_COUNT
#undef LAYER_STRINGS
#undef LAYERS
#undef EXTENSION_COUNT
#undef EXTENSION_STRINGS
#undef EXTENSIONS
#undef PNEXT
#undef CREATE_DEBUGGER
#undef DEBUGGER_VALID
  }

  // Post: no instance, no debugger, instance unloaded
  void graphics_subsystem::close_vk_instance() noexcept {
    if (m_instance != VK_NULL_HANDLE)
    {
      OBERON_ASSERT(m_instance == m_vkdl.loaded_instance());
      if (m_debug_messenger != VK_NULL_HANDLE)
      {
        OBERON_DECLARE_VK_PFN(m_vkdl, DestroyDebugUtilsMessengerEXT);
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
        m_debug_messenger = VK_NULL_HANDLE;
      }
      OBERON_DECLARE_VK_PFN(m_vkdl, DestroyInstance);
      vkDestroyInstance(m_instance, nullptr);
      m_instance = VK_NULL_HANDLE;
      m_vkdl.unload_instance();
    }
    OBERON_POSTCONDITION(m_debug_messenger == VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_instance == VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_vkdl.loaded_instance() == VK_NULL_HANDLE);
  }

  bool is_acceptable_device(const VkPhysicalDevice physical_device, io_subsystem& io, const vkfl::loader& vkdl) {
    OBERON_PRECONDITION(physical_device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(vkdl.loaded_instance() != VK_NULL_HANDLE);
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceQueueFamilyProperties);
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceXcbPresentationSupportKHR);
    auto sz = u32{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, nullptr);
    auto queue_families = std::vector<VkQueueFamilyProperties>(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &sz, std::data(queue_families));
    auto has_graphics = false;
    auto has_compute = false;
    auto has_transfer = false;
    auto has_present = false;
    auto index = u32{ 0 };
    for (const auto& queue_family : queue_families)
    {
      has_graphics = has_graphics || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
      has_compute = has_compute || (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT);
      has_transfer = has_transfer || (queue_family.queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT |
                                                                 VK_QUEUE_TRANSFER_BIT));
      has_present = has_present || vkGetPhysicalDeviceXcbPresentationSupportKHR(physical_device, index++,
                                                                                io.x_connection(),
                                                                                io.x_screen()->root_visual);
    }
    return has_graphics && has_compute && has_transfer && has_present;
  }

  // Pre: no device, no physical_device, no primary_queue, device unloaded, instance, instance loaded
  // Post: device, physical_device, primary_queue, device loaded
  void graphics_subsystem::open_vk_device(io_subsystem& io, const u32 device_index) {
    OBERON_PRECONDITION(m_device == VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_physical_device == VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_primary_queue == VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_vkdl.loaded_device() == VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_instance != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_vkdl.loaded_instance() == m_instance);
    // Select physical device and retrieve properties.
    OBERON_DECLARE_VK_PFN(m_vkdl, EnumeratePhysicalDevices);
    {
      auto sz = u32{ 0 };
      OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_instance, &sz, nullptr),
                         oberon::errors::vk_create_device_failed_error{ });
      // Sanity, must have at least one device.
      OBERON_ASSERT(sz > 0);
      auto physical_devices = std::vector<VkPhysicalDevice>(sz);
      OBERON_VK_SUCCEEDS(vkEnumeratePhysicalDevices(m_instance, &sz, std::data(physical_devices)),
                         oberon::errors::vk_create_device_failed_error{ });
      std::erase_if(physical_devices, [&](const VkPhysicalDevice p){ return !is_acceptable_device(p, io, m_vkdl); });
      if (device_index > std::size(physical_devices))
      {
        throw oberon::errors::no_device_found_error{ };
      }
      m_physical_device = physical_devices[device_index];
      OBERON_DECLARE_VK_PFN(m_vkdl, GetPhysicalDeviceProperties);
      vkGetPhysicalDeviceProperties(m_physical_device, &m_physical_device_properties);
    }
    // Create a new Vulkan device.
    {
      auto device_info = VkDeviceCreateInfo{ };
      device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      auto extensions = std::array<cstring, 1>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
      device_info.ppEnabledExtensionNames = std::data(extensions);
      device_info.enabledExtensionCount = std::size(extensions);
      OBERON_DECLARE_VK_PFN(m_vkdl, GetPhysicalDeviceFeatures);
      auto features = VkPhysicalDeviceFeatures{ };
      vkGetPhysicalDeviceFeatures(m_physical_device, &features);
      device_info.pEnabledFeatures = &features;
      switch (m_physical_device_properties.vendorID)
      {
      // AMD, Intel, and Nvidia all implement a graphics/compute/transfer/present queue family as queue family 0.
      // In the future these cases may not all be the same but this is the current reality.
      case 0x10de: // Nvidia
      case 0x1002: // AMD
      case 0x8086: // Intel
        m_primary_queue_family = 0;
        break;
      default:
        throw oberon::errors::not_implemented_error{ };
      }
      auto queue_info = VkDeviceQueueCreateInfo{ };
      queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_info.queueFamilyIndex = m_primary_queue_family;
      queue_info.queueCount = 1;
      auto priority = 1.0f;
      queue_info.pQueuePriorities = &priority;
      device_info.pQueueCreateInfos = &queue_info;
      device_info.queueCreateInfoCount = 1;
      OBERON_DECLARE_VK_PFN(m_vkdl, CreateDevice);
      OBERON_VK_SUCCEEDS(vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device),
                         oberon::errors::vk_create_device_failed_error{ });
      m_vkdl.load(m_device);
      OBERON_DECLARE_VK_PFN(m_vkdl, GetDeviceQueue);
      vkGetDeviceQueue(m_device, m_primary_queue_family, 0, &m_primary_queue);
    }
    OBERON_POSTCONDITION(m_device != VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_physical_device != VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_primary_queue != VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_vkdl.loaded_device() == m_device);
  }

  // Post: no device, no physical device, device unloaded, no primary queue and primary queue family undefined
  void graphics_subsystem::close_vk_device() noexcept {
    if (m_device != VK_NULL_HANDLE)
    {
      OBERON_ASSERT(m_device == m_vkdl.loaded_device());
      OBERON_DECLARE_VK_PFN(m_vkdl, DestroyDevice);
      vkDestroyDevice(m_device, nullptr);
      m_device = VK_NULL_HANDLE;
      m_vkdl.unload_device();
      m_primary_queue = VK_NULL_HANDLE;
      m_physical_device = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(m_device == VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_physical_device == VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_primary_queue == VK_NULL_HANDLE);
    OBERON_POSTCONDITION(m_vkdl.loaded_device() == VK_NULL_HANDLE);
  }

  graphics_subsystem::graphics_subsystem(io_subsystem& io, const u32 device_index) {
    open_vk_instance();
    open_vk_device(io, device_index);
  }

  graphics_subsystem::~graphics_subsystem() noexcept {
    close_vk_device();
    close_vk_instance();
  }

  const vkfl::loader& graphics_subsystem::vk_loader() const {
    return m_vkdl;
  }

  VkInstance graphics_subsystem::vk_instance() {
    return m_instance;
  }

  VkPhysicalDevice graphics_subsystem::vk_physical_device() {
    return m_physical_device;
  }

  const VkPhysicalDeviceProperties& graphics_subsystem::vk_physical_device_properties() const {
    return m_physical_device_properties;
  }

  u32 graphics_subsystem::vk_primary_queue_family() const {
    return m_primary_queue_family;
  }

  VkDevice graphics_subsystem::vk_device() {
    return m_device;
  }

  VkQueue graphics_subsystem::vk_primary_queue() {
    return m_primary_queue;
  }

}
