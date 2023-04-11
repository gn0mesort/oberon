#include "oberon/internal/graphics_context.hpp"

#include <array>

#define VK_STRUCT(name) OBERON_INTERNAL_VK_STRUCT(name)
#define VK_DECLARE_PFN(dl, pfn) OBERON_INTERNAL_VK_DECLARE_PFN(dl, pfn)
#define VK_SUCCEEDS(exp) OBERON_INTERNAL_VK_SUCCEEDS(exp)

namespace oberon::internal {

  void graphics_context::check_instance_version(const vkfl::loader& dl) {
    auto ver = u32{ };
    VK_DECLARE_PFN(dl, vkEnumerateInstanceVersion);
    VK_SUCCEEDS(vkEnumerateInstanceVersion(&ver));
    auto major = VK_API_VERSION_MAJOR(ver);
    auto minor = VK_API_VERSION_MINOR(ver);
    OBERON_CHECK_ERROR_MSG(major == 1 && minor >= 3, 1, "The minimum supported Vulkan version is 1.3. However, the "
                           "maximum supported Vulkan instance version is %u.%u", major, minor);
  }

  VkApplicationInfo graphics_context::pack_application_info(const cstring application_name,
                                                                   const u32 application_version,
                                                                   const cstring engine_name,
                                                                   const u32 engine_version) {
    auto result = VkApplicationInfo{ };
    result.sType = VK_STRUCT(APPLICATION_INFO);
    result.apiVersion = VK_API_VERSION_1_3;
    result.pEngineName = engine_name;
    result.engineVersion = engine_version;
    result.pApplicationName = application_name;
    result.applicationVersion = application_version;
    return result;
  }

  std::unordered_set<std::string> graphics_context::available_layers(const vkfl::loader& dl) {
    auto sz = u32{ };
    VK_DECLARE_PFN(dl, vkEnumerateInstanceLayerProperties);
    VK_SUCCEEDS(vkEnumerateInstanceLayerProperties(&sz, nullptr));
    auto layers = std::vector<VkLayerProperties>(sz);
    VK_SUCCEEDS(vkEnumerateInstanceLayerProperties(&sz, layers.data()));
    auto result = std::unordered_set<std::string>{ };
    for (const auto& layer : layers)
    {
      result.insert(layer.layerName);
    }
    return result;
  }

  std::vector<cstring> graphics_context::select_layers(std::unordered_set<std::string>& available_layers,
                                                              const std::unordered_set<std::string>& requested_layers) {
    auto result = std::vector<cstring>{ };
    for (const auto& layer : available_layers)
    {
      if (requested_layers.contains(layer))
      {
        result.emplace_back(layer.c_str());
      }
    }
    return result;
  }

  std::unordered_set<std::string> graphics_context::available_extensions(const vkfl::loader& dl,
                                                                                std::vector<cstring>& selected_layers) {
    auto sz = u32{ };
    VK_DECLARE_PFN(dl, vkEnumerateInstanceExtensionProperties);
    VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(nullptr, &sz, nullptr));
    auto extensions = std::vector<VkExtensionProperties>(sz);
    VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(nullptr, &sz, extensions.data()));
    auto result = std::unordered_set<std::string>{ };
    for (const auto& extension : extensions)
    {
      result.insert(extension.extensionName);
    }
    for (const auto& layer : selected_layers)
    {
      VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(layer, &sz, nullptr));
      extensions.resize(sz);
      VK_SUCCEEDS(vkEnumerateInstanceExtensionProperties(layer, &sz, extensions.data()));
      for (const auto& extension : extensions)
      {
        result.insert(extension.extensionName);
      }
    }
    return result;
  }

  std::vector<cstring>
  graphics_context::select_extensions(std::unordered_set<std::string>& available_extensions,
                                             const std::unordered_set<std::string>& required_extensions,
                                             const std::unordered_set<std::string>& requested_extensions) {
    auto result = std::vector<cstring>{ };
    for (const auto& extension : required_extensions)
    {
      auto itr = available_extensions.find(extension);
      OBERON_CHECK_ERROR_MSG(itr != available_extensions.end(), 1, "The \"%s\" extension is required but not "
                             "supported by the Vulkan instance.", extension.c_str());
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

  VkInstanceCreateInfo
  graphics_context::pack_instance_info(const readonly_ptr<VkApplicationInfo> application_info,
                                              const ptr<void> next, const readonly_ptr<cstring> layers,
                                              const u32 layer_count, const readonly_ptr<cstring> extensions,
                                              const u32 extension_count) {
    auto result = VkInstanceCreateInfo{ };
    result.sType = VK_STRUCT(INSTANCE_CREATE_INFO);
    result.pNext = next;
    result.pApplicationInfo = application_info;
    result.ppEnabledLayerNames = layers;
    result.enabledLayerCount = layer_count;
    result.ppEnabledExtensionNames = extensions;
    result.enabledExtensionCount = extension_count;
    return result;
  }

  void graphics_context::intern_instance_in_loader(vkfl::loader& dl, const VkInstance instance) {
    dl.load(instance);
  }

  graphics_context::graphics_context(const defer_construction&) { }

  graphics_context::graphics_context(const std::unordered_set<std::string>& requested_layers,
                                                   const std::unordered_set<std::string>& required_extensions,
                                                   const std::unordered_set<std::string>& requested_extensions) {
    check_instance_version(m_dl);
    auto app_info = pack_application_info(nullptr, 0, "oberon", 0);
    auto layers = available_layers(m_dl);
    auto selected_layers = select_layers(layers, requested_layers);
    auto extensions = available_extensions(m_dl, selected_layers);
    auto selected_extensions = select_extensions(extensions, required_extensions, requested_extensions);
    auto instance_info = pack_instance_info(&app_info, nullptr, selected_layers.data(), selected_layers.size(),
                                            selected_extensions.data(), selected_extensions.size());
    VK_DECLARE_PFN(m_dl, vkCreateInstance);
    auto instance = VkInstance{ };
    VK_SUCCEEDS(vkCreateInstance(&instance_info, nullptr, &instance));
    intern_instance_in_loader(m_dl, instance);
  }

  graphics_context::~graphics_context() noexcept {
    VK_DECLARE_PFN(m_dl, vkDestroyInstance);
    vkDestroyInstance(m_dl.loaded_instance(), nullptr);
  }

  VkInstance graphics_context::instance() {
    return m_dl.loaded_instance();
  }

  const vkfl::loader& graphics_context::dispatch_loader() {
    return m_dl;
  }

  debug_graphics_context::debug_graphics_context(const std::unordered_set<std::string>& requested_layers,
                                                               const std::unordered_set<std::string>& required_extensions,
                                                               const std::unordered_set<std::string>& requested_extensions) :
  graphics_context{ defer_construction{ } } {
    check_instance_version(m_dl);
    auto debug_required_extensions = required_extensions;
    debug_required_extensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    auto debug_requested_extensions = requested_extensions;
    debug_requested_extensions.insert(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    auto app_info = pack_application_info(nullptr, 0, "oberon", 0);
    auto layers = available_layers(m_dl);
    auto selected_layers = select_layers(layers, requested_layers);
    auto extensions = available_extensions(m_dl, selected_layers);
    auto selected_extensions = select_extensions(extensions, debug_required_extensions, debug_requested_extensions);
    auto debug_info = VkDebugUtilsMessengerCreateInfoEXT{ };
    debug_info.sType = VK_STRUCT(DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    debug_info.pfnUserCallback = vkDebugUtilsMessengerCallbackEXT;
    auto next = reinterpret_cast<ptr<void>>(&debug_info);
    auto validation_features = VkValidationFeaturesEXT{ };
    validation_features.sType = VK_STRUCT(VALIDATION_FEATURES_EXT);
    validation_features.pNext = &debug_info;
    auto enabled_validation_features = std::array<VkValidationFeatureEnableEXT, 4>{
      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
      VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
      VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };
    validation_features.pEnabledValidationFeatures = enabled_validation_features.data();
    validation_features.enabledValidationFeatureCount = enabled_validation_features.size();
    if (extensions.contains(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
    {
      next = reinterpret_cast<ptr<void>>(&validation_features);
    }
    auto instance_info = pack_instance_info(&app_info, next, selected_layers.data(), selected_layers.size(),
                                            selected_extensions.data(), selected_extensions.size());
    VK_DECLARE_PFN(m_dl, vkCreateInstance);
    auto instance = VkInstance{ };
    VK_SUCCEEDS(vkCreateInstance(&instance_info, nullptr, &instance));
    intern_instance_in_loader(m_dl, instance);
    VK_DECLARE_PFN(m_dl, vkCreateDebugUtilsMessengerEXT);
    VK_SUCCEEDS(vkCreateDebugUtilsMessengerEXT(instance, &debug_info, nullptr, &m_debug_messenger));
  }

  debug_graphics_context::~debug_graphics_context() noexcept {
    VK_DECLARE_PFN(m_dl, vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(m_dl.loaded_instance(), m_debug_messenger, nullptr);
  }
}
