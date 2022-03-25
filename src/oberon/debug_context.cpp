#include "oberon/detail/debug_context_impl.hpp"

#include <cstdio>
#include <cstring>

#include <vector>
#include <algorithm>
#include <iterator>

#include "oberon/errors.hpp"

namespace {

  static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugLog(
    VkDebugUtilsMessageSeverityFlagBitsEXT /* messageSeverity */,
    VkDebugUtilsMessageTypeFlagsEXT /* messageTypes */,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /* pUserData */
  ) {
    std::printf("[%s]: %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    return VK_FALSE;
  }

}

namespace oberon {
namespace detail {

  iresult validate_requested_layers(
    const debug_context_impl& ctx,
    const std::unordered_set<std::string>& requested_layers
  ) noexcept {
    OBERON_DECLARE_VK_PFN(ctx.dl, EnumerateInstanceLayerProperties);
    auto available_layer_props = std::vector<VkLayerProperties>{ };
    {
      auto sz = u32{ 0 };
      OBERON_ASSERT(vkEnumerateInstanceLayerProperties(&sz, nullptr) == VK_SUCCESS);
      available_layer_props.resize(sz);
      OBERON_ASSERT(vkEnumerateInstanceLayerProperties(&sz, std::data(available_layer_props)) == VK_SUCCESS);
    }
    auto available_layers = std::unordered_set<std::string>{ };
    for (const auto& available_layer_prop : available_layer_props)
    {
      available_layers.insert(available_layer_prop.layerName);
    }
    for (const auto& requested_layer : requested_layers)
    {
      if (!available_layers.contains(requested_layer))
      {
        return -1;
      }
    }
    return 0;
  }

namespace {

  const auto enabled_validation_features = std::array<VkValidationFeatureEnableEXT, 3>{
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
  };

}

  iresult preload_debugging_context(
    const debug_context_impl& ctx,
    VkDebugUtilsMessengerCreateInfoEXT& debug_info,
    VkValidationFeaturesEXT& validation_features
  ) noexcept {
    OBERON_INIT_VK_STRUCT(debug_info, DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.pfnUserCallback = vkDebugLog;
    if (ctx.instance_extensions.contains(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
    {
      OBERON_INIT_VK_STRUCT(validation_features, VALIDATION_FEATURES_EXT);
      validation_features.pEnabledValidationFeatures = std::data(enabled_validation_features);
      validation_features.enabledValidationFeatureCount = std::size(enabled_validation_features);
      validation_features.pNext = &debug_info;
    }
    return 0;
  }

  iresult create_debug_messenger(
    debug_context_impl& ctx,
    const VkDebugUtilsMessengerCreateInfoEXT& debug_info
  ) noexcept {
    OBERON_PRECONDITION(ctx.instance);
    OBERON_PRECONDITION(!ctx.debug_messenger);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateDebugUtilsMessengerEXT);
    auto result = vkCreateDebugUtilsMessengerEXT(ctx.instance, &debug_info, nullptr, &ctx.debug_messenger);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(ctx.debug_messenger);
    return 0;
  }

  iresult send_debug_message(
    const debug_context_impl& ctx,
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const std::string& message
  ) noexcept {
    OBERON_DECLARE_VK_PFN(ctx.dl, SubmitDebugUtilsMessageEXT);
    auto callback_data = VkDebugUtilsMessengerCallbackDataEXT{ };
    OBERON_INIT_VK_STRUCT(callback_data, DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT);
    callback_data.pMessageIdName = "Library message";
    callback_data.pMessage = std::data(message);
    vkSubmitDebugUtilsMessageEXT(ctx.instance, severity, VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, &callback_data);
    return 0;
  }

  iresult destroy_debug_messenger(debug_context_impl& ctx) noexcept {
    OBERON_PRECONDITION(ctx.instance);
    if (ctx.debug_messenger)
    {
      OBERON_DECLARE_VK_PFN(ctx.dl, DestroyDebugUtilsMessengerEXT);
      vkDestroyDebugUtilsMessengerEXT(ctx.instance, ctx.debug_messenger, nullptr);
      ctx.debug_messenger = nullptr;
    }
    OBERON_POSTCONDITION(!ctx.debug_messenger);
    return 0;
  }

}
  debug_context::debug_context(
    const std::string& application_name,
    const u16 application_version_major,
    const u16 application_version_minor,
    const u16 application_version_patch,
    const std::unordered_set<std::string>& requested_layers
  ) : context{ new detail::debug_context_impl{ } } {
    auto& q = reference_cast<detail::debug_context_impl>(implementation());
    detail::store_application_info(
      q,
      application_name,
      application_version_major, application_version_minor, application_version_patch
    );
    detail::connect_to_x11(q, nullptr);
    if (OBERON_IS_IERROR(detail::validate_requested_layers(q, requested_layers)))
    {
      throw fatal_error{ "One or more requested Vulkan instance layers are not available." };
    }
    {
      auto required_extensions = std::unordered_set<std::string>{
          VK_KHR_SURFACE_EXTENSION_NAME,
          VK_KHR_XCB_SURFACE_EXTENSION_NAME,
          VK_EXT_DEBUG_UTILS_EXTENSION_NAME
      };
      auto optional_extensions = std::unordered_set<std::string>{
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
      };
      auto result = detail::get_instance_extensions(q, requested_layers, required_extensions, optional_extensions);
      if (OBERON_IS_IERROR(result))
      {
        throw fatal_error{ "One or more required Vulkan instance are not available." };
      }
    }

    auto debug_info = VkDebugUtilsMessengerCreateInfoEXT{ };
    auto validation_features = VkValidationFeaturesEXT{ };
    preload_debugging_context(q, debug_info, validation_features);
    {
      auto next = readonly_ptr<void>{ nullptr };
      if (validation_features.pNext)
      {
        next = &validation_features;
      }
      else
      {
        next = &debug_info;
      }
      if (OBERON_IS_IERROR(detail::create_vulkan_instance(q, requested_layers, next)))
      {
        throw fatal_error{ "Failed to create Vulkan instance." };
      }
    }
    q.dl.load(q.instance);
    if (OBERON_IS_IERROR(detail::create_debug_messenger(q, debug_info)))
    {
      throw fatal_error{ "Failed to create Vulkan debug messenger." };
    }
    {
      auto required_extensions = std::unordered_set<std::string>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
      if (OBERON_IS_IERROR(detail::select_physical_device(q, required_extensions, { })))
      {
        throw fatal_error{ "None of the Vulkan physical devices available can be used." };
      }
      detail::select_physical_device_queue_families(q);
      if (OBERON_IS_IERROR(detail::create_vulkan_device(q, nullptr)))
      {
        throw fatal_error{ "Failed to create Vulkan device." };
      }
    }
    q.dl.load(q.device);
    detail::get_device_queues(q);
  }

  void debug_context::v_dispose() noexcept {
    auto& q = reference_cast<detail::debug_context_impl>(implementation());
    detail::destroy_vulkan_device(q);
    detail::destroy_debug_messenger(q);
    detail::destroy_vulkan_instance(q);
    detail::disconnect_from_x11(q);
  }

  debug_context::~debug_context() noexcept {
    dispose();
  }

}
