#include "oberon/detail/debug_context_impl.hpp"

#include <cstdio>

#include <vector>
#include <algorithm>
#include <iterator>

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.hpp>

namespace {
  static std::unordered_set<std::string> sg_required_extensions{
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
  };

  static std::unordered_set<std::string> sg_optional_extensions{
    VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
    VK_KHR_DISPLAY_EXTENSION_NAME
  };

  static std::array<vk::ValidationFeatureEnableEXT, 3> sg_validation_enable{
    vk::ValidationFeatureEnableEXT::eBestPractices,
    vk::ValidationFeatureEnableEXT::eGpuAssisted,
    vk::ValidationFeatureEnableEXT::eSynchronizationValidation
  };

  static std::array<vk::ValidationFeatureDisableEXT, 0> sg_validation_disable{ };

  static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(
    VkDebugUtilsMessageSeverityFlagBitsEXT /* messageSeverity */,
    VkDebugUtilsMessageTypeFlagsEXT /* messageTypes */,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /* pUserData */
  ) {
    std::printf("[VK]: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
  }
}

namespace oberon {
namespace detail {
  void debug_context_dtor::operator()(const ptr<debug_context_impl> impl) const noexcept {
    delete impl;
  }
}
  std::pair<bool, std::string_view> debug_context::validate_layers(const std::unordered_set<std::string_view>& layers) {
    for (const auto& layer : layers)
    {
      if (!context::has_layer(layer))
      {
        return { false, layer };
      }
    }
    return { true, "" };
  }

  std::pair<bool, std::string_view> debug_context::validate_required_extensions(
    const std::unordered_set<std::string_view>& layers
  ) {
    for (const auto& extension : debug_context::required_extensions())
    {
      if (!debug_context::has_extension(layers, extension))
      {
        return { false, extension };
      }
    }
    return { true, "" };
  }

  bool debug_context::has_extension(
    const std::unordered_set<std::string_view>& layers,
    const std::string_view& extension_name
  ) {
    if (context::has_extension(extension_name))
    {
      return true;
    }
    for (const auto& layer : layers)
    {
      if (context::layer_has_extension(layer, extension_name))
      {
        return true;
      }
    }
    return false;
  }

  const std::unordered_set<std::string>& debug_context::required_extensions() {
    return sg_required_extensions;
  }

  const std::unordered_set<std::string>& debug_context::optional_extensions() {
    return sg_optional_extensions;
  }

  debug_context::debug_context(const std::unordered_set<std::string_view>& requested_layers) :
  m_impl{ new detail::debug_context_impl{ } } {
    m_impl->dl.init(vkGetInstanceProcAddr);
    if (auto [result, missing] = debug_context::validate_layers(requested_layers); !result)
    {
      // TODO fatal error missing layer
    }
    if (auto [result, missing] = debug_context::validate_required_extensions(requested_layers); !result)
    {
      // TODO fatal error missing extension
    }
    auto lyrs = std::vector<cstring>(requested_layers.size());
    std::transform(
      std::begin(requested_layers), std::end(requested_layers),
      std::begin(lyrs),
      [](const auto str) {
        return str.data();
      }
    );
    auto exts = std::vector<cstring>{ };
    std::transform(
      std::begin(debug_context::required_extensions()), std::end(debug_context::required_extensions()),
      std::back_inserter(exts),
      [](const auto& str) {
        return str.c_str();
      }
    );
    for (const auto& optional_extension : debug_context::optional_extensions())
    {
      if (debug_context::has_extension(requested_layers, optional_extension))
      {
        exts.push_back(optional_extension.c_str());
        if (optional_extension == VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)
        {
          m_impl->has_validation_features = true;
        }
      }
    }
    
    auto app_info = vk::ApplicationInfo{ };
    // This probably doesn't matter but shouldn't really be hard coded.
    app_info.pApplicationName = "";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // Maybe allow user selection?
    app_info.apiVersion = VK_API_VERSION_1_2;

    auto instance_chain =
      vk::StructureChain<vk::InstanceCreateInfo, vk::ValidationFeaturesEXT, vk::DebugUtilsMessengerCreateInfoEXT>{ };
    auto& debug_info = instance_chain.get<vk::DebugUtilsMessengerCreateInfoEXT>();
    debug_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                 vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                 vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                 vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debug_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                             vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                             vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    debug_info.pfnUserCallback = vkDebugLog;

    // Always do the initialization and simply unlink if not needed?
    auto& validation_features = instance_chain.get<vk::ValidationFeaturesEXT>();
    if (m_impl->has_validation_features)
    {
      validation_features.pEnabledValidationFeatures = sg_validation_enable.data();
      validation_features.enabledValidationFeatureCount = sg_validation_enable.size();
      validation_features.pDisabledValidationFeatures = sg_validation_disable.data();
      validation_features.disabledValidationFeatureCount = sg_validation_disable.size();
    }
    else
    {
      instance_chain.unlink<vk::ValidationFeaturesEXT>();
    }

    auto& instance_info = instance_chain.get<vk::InstanceCreateInfo>();
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledLayerNames = lyrs.data();
    instance_info.enabledLayerCount = lyrs.size();
    instance_info.ppEnabledExtensionNames = exts.data();
    instance_info.enabledExtensionCount = exts.size();

    m_impl->instance = vk::createInstance(instance_info, nullptr, m_impl->dl);
    m_impl->dl.init(m_impl->instance);
    m_impl->debug_messenger = m_impl->instance.createDebugUtilsMessengerEXT(debug_info, nullptr, m_impl->dl);
  }

  debug_context::~debug_context() noexcept {
    m_impl->instance.destroyDebugUtilsMessengerEXT(m_impl->debug_messenger, nullptr, m_impl->dl);
    m_impl->instance.destroy(nullptr, m_impl->dl);
  }
}
