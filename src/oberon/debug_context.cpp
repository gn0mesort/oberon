#include "oberon/detail/debug_context_impl.hpp"

#include <cstdio>

#include <vector>
#include <algorithm>
#include <iterator>
#include <optional>
#include <memory>

#include "oberon/errors.hpp"

namespace {
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
  std::vector<cstring> debug_context_select_layers(
    debug_context_impl& ctx,
    const std::unordered_set<std::string_view>& requested_layers
  ) {
    auto vkEnumerateInstanceLayerProperties = ctx.dbgft.vkEnumerateInstanceLayerProperties;
    auto sz = u32{ 0 };
    if (auto result = vkEnumerateInstanceLayerProperties(&sz, nullptr); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to enumerate Vulkan instance layers." };
    }
    auto layer_props = std::vector<VkLayerProperties>(sz);
    if (auto result = vkEnumerateInstanceLayerProperties(&sz, layer_props.data()); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to enumerate Vulkan instance layers." };
    }
    for (const auto& requested_layer : requested_layers)
    {
      auto found = false;
      for (const auto& layer_prop : layer_props)
      {
        if (requested_layer == layer_prop.layerName)
        {
          found = true;
        }
      }
      if (!found)
      {
        throw fatal_error{ "Requested Vulkan layer: \"" + std::string{ requested_layer } + "\" was not found." };
      }
    }
    auto layers = std::vector<cstring>(requested_layers.size());
    for (auto cur = std::begin(layers); const auto& requested_layer : requested_layers)
    {
      *cur = requested_layer.data();
      ++cur;
    }
    return layers;
  }

  std::unordered_set<std::string> debug_context_fetch_extensions(debug_context_impl& ctx, const cstring layer) {
    auto vkEnumerateInstanceExtensionProperties = ctx.dbgft.vkEnumerateInstanceExtensionProperties;
    auto sz = u32{ 0 };
    if (auto result = vkEnumerateInstanceExtensionProperties(layer, &sz, nullptr); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to enumerate Vulkan extensions." };
    }
    auto extension_props = std::vector<VkExtensionProperties>(sz);
    if (auto result = vkEnumerateInstanceExtensionProperties(layer, &sz, extension_props.data()); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to enumerate Vulkan extensions." };
    }
    auto extensions = std::unordered_set<std::string>{ };
    for (const auto& extension_prop : extension_props)
    {
      extensions.insert(extension_prop.extensionName);
    }
    return extensions;
  }

  std::vector<cstring> debug_context_select_extensions(
    debug_context_impl& ctx,
    const std::unordered_set<std::string>& available_extensions
  ) {
    auto required_extensions = std::unordered_set<std::string_view>{
      VK_KHR_XCB_SURFACE_EXTENSION_NAME,
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    for (const auto& required_extension : required_extensions)
    {
      if (!available_extensions.contains(required_extension.data()))
      {
        throw fatal_error{ "Required Vulkan extension: \"" + std::string{ required_extension } + "\" was not found." };
      }
    }

    auto enabled_extensions = std::vector<cstring>(required_extensions.size());
    for (auto cur = std::begin(enabled_extensions); const auto& required_extension : required_extensions)
    {
      *cur = required_extension.data();
      ++cur;
    }

    auto optional_extensions = std::unordered_set<std::string_view>{
      VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
    };

    for (const auto& optional_extension : optional_extensions)
    {
      if (available_extensions.contains(optional_extension.data()))
      {
        ctx.optional_extensions[optional_extension] = true;
        enabled_extensions.push_back(optional_extension.data());
      }
      else
      {
        ctx.optional_extensions[optional_extension] = false;
      }
    }
    return enabled_extensions;
  }

  void debug_context_load_instance_extension_pfns(debug_context_impl& ctx) {
    ctx.dbgft.vkCreateDebugUtilsMessengerEXT = OBERON_INSTANCE_FN(ctx, vkCreateDebugUtilsMessengerEXT);
    ctx.dbgft.vkGetPhysicalDeviceXcbPresentationSupportKHR = 
      OBERON_INSTANCE_FN(ctx, vkGetPhysicalDeviceXcbPresentationSupportKHR);
    ctx.dbgft.vkDestroyDebugUtilsMessengerEXT = OBERON_INSTANCE_FN(ctx, vkDestroyDebugUtilsMessengerEXT);
  }

  std::vector<VkPhysicalDevice> debug_context_fetch_physical_devices(
    debug_context_impl& ctx,
    const std::unordered_set<std::string_view>& required_extensions
  ) {
    auto vkEnumeratePhysicalDevices = ctx.dbgft.vkEnumeratePhysicalDevices;
    auto sz = u32{ 0 };
    if (auto result = vkEnumeratePhysicalDevices(ctx.instance, &sz, nullptr); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to enumerate available Vulkan devices." };
    }
    auto physical_devices = std::vector<VkPhysicalDevice>(sz);
    if (auto result = vkEnumeratePhysicalDevices(ctx.instance, &sz, physical_devices.data()); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to enumerate available Vulkan devices." };
    }
    // Filtering only against required extensions because swapchain is required and, in my opinion, a device
    // that supports VK_KHR_swapchain should always have a valid presentation queue family.
    // I'm not certain that the extension or standard guarantees this property but it would be supremely bizarre
    // for it not to be guaranteed.
    // Ditto for physical devices that support VK_KHR_swapchain and presentation but not graphics.
    // Does a magical unicorn compute and presentation only device exist?
    auto filter_point = std::remove_if(
      std::begin(physical_devices), std::end(physical_devices),
      [&ctx, &required_extensions](const auto& physical_device) {
        auto vkEnumerateDeviceExtensionProperties = ctx.dbgft.vkEnumerateDeviceExtensionProperties;
        auto available_extensions = std::unordered_set<std::string_view>{ };
        auto sz = u32{ 0 };
        if (
          auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &sz, nullptr);
          result != VK_SUCCESS
        )
        {
          return true;
        }
        auto extensions = std::vector<VkExtensionProperties>(sz);
        if (
          auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &sz, extensions.data());
          result != VK_SUCCESS
        )
        {
          return true;
        }
        for (const auto& extension : extensions)
        {
          available_extensions.insert(extension.extensionName);
        }
        for (const auto& required_extension : required_extensions)
        {
          if (!available_extensions.contains(required_extension))
          {
            return true;
          }
        }
        return false;
      }
    );
    physical_devices.erase(filter_point, std::end(physical_devices));
    std::sort(
      std::begin(physical_devices),
      std::end(physical_devices),
      [&ctx](const auto& a, const auto& b) {
        auto vkGetPhysicalDeviceProperties = ctx.dbgft.vkGetPhysicalDeviceProperties;
        auto props = std::array<VkPhysicalDeviceProperties, 2>{ };
        auto scores = std::array<u32, 2>{ };
        auto& [a_props, b_props] = props;
        auto& [a_score, b_score] = scores;
        vkGetPhysicalDeviceProperties(a, &a_props);
        vkGetPhysicalDeviceProperties(b, &b_props);
        for (auto score = std::begin(scores); auto& prop : props)
        {
          // Preference discrete cards.
          switch(prop.deviceType)
          {
          case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            *score = 10;
            break;
          case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            *score = 9;
            break;
          case VK_PHYSICAL_DEVICE_TYPE_CPU:
            *score = 8;
            break;
          case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            *score = 7;
            break;
          default:
            *score = 0;
          }
          ++score;
        }
        return a_score < b_score;
      }
    );
    return physical_devices;
  }

  void debug_context_select_physical_device_queue_families(debug_context_impl& ctx) {
    auto vkGetPhysicalDeviceQueueFamilyProperties = ctx.dbgft.vkGetPhysicalDeviceQueueFamilyProperties;
    auto vkGetPhysicalDeviceXcbPresentationSupportKHR = ctx.dbgft.vkGetPhysicalDeviceXcbPresentationSupportKHR;
    auto sz = u32{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &sz, nullptr);
    auto queue_families = std::vector<VkQueueFamilyProperties>(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &sz, queue_families.data());
    // Based on my previous reasoning (about the swapchain extension) all available devices should support graphics
    // and present. If this isn't the case this will need to change.
    auto has_gfx = false; // Graphics implies transfer.
    auto has_pres = false;
    for (auto i = u32{ 0 }; i < queue_families.size(); ++i)
    {
      auto supports_present =
        vkGetPhysicalDeviceXcbPresentationSupportKHR(
          ctx.physical_device, i,
          ctx.x_connection, ctx.x_screen->root_visual
        );
      auto supports_graphics = queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
      if (supports_present && supports_graphics)
      {
        ctx.graphics_transfer_queue_family = i;
        ctx.presentation_queue_family = i;
        has_gfx = true;
        has_pres = true;
        break;
      }
      if (supports_present && !has_pres)
      {
        ctx.presentation_queue_family = i;
        has_pres = true;
      }
      if (supports_graphics && !has_gfx)
      {
        ctx.graphics_transfer_queue_family = i;
        has_gfx = true;
      }
    }
  }

  void debug_context_fill_queue_create_info(
    u32 queue_family_index,
    VkDeviceQueueCreateInfo& queue_info, float& priority
  ) {
    std::memset(&queue_info, 0, sizeof(VkDeviceQueueCreateInfo));
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pQueuePriorities = &priority;
    queue_info.queueFamilyIndex = queue_family_index;
    queue_info.queueCount = 1;
  }
}
  debug_context::debug_context(
    const std::string_view& application_name,
    const u16 application_version_major,
    const u16 application_version_minor,
    const u16 application_version_patch,
    const std::unordered_set<std::string_view>& requested_layers
  ) : context{ new detail::debug_context_impl{ } } {
    auto q = q_ptr<detail::debug_context_impl>();
    detail::context_initialize_x(*q, nullptr);
    detail::context_load_vulkan_library(*q);
    q->vkft = &q->dbgft; // Function table allocated automatically
    detail::context_load_global_pfns(*q);
    auto layers = detail::debug_context_select_layers(*q, requested_layers);
    auto available_extensions = detail::debug_context_fetch_extensions(*q, nullptr);
    for (const auto& layer : layers)
    {
      available_extensions.merge(detail::debug_context_fetch_extensions(*q, layer));
    }
    auto extensions = detail::debug_context_select_extensions(*q, available_extensions);
    
    auto debug_info = VkDebugUtilsMessengerCreateInfoEXT{ };
    std::memset(&debug_info, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.pfnUserCallback = vkDebugLog;

    auto validation_features = VkValidationFeaturesEXT{ };

    auto enabled_features = std::array<VkValidationFeatureEnableEXT, 3>{
      VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
      VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
      VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };

    if (q->optional_extensions[VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME])
    {
      std::memset(&validation_features, 0, sizeof(VkValidationFeaturesEXT));
      validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
      validation_features.pEnabledValidationFeatures = enabled_features.data();
      validation_features.enabledValidationFeatureCount = enabled_features.size();
      debug_info.pNext = &validation_features;
    }

    detail::context_initialize_vulkan_instance(
      *q,
      application_name.data(),
      VK_MAKE_VERSION(application_version_major, application_version_minor, application_version_patch),
      layers.data(), layers.size(),
      extensions.data(), extensions.size(),
      &debug_info
    );
    detail::context_load_instance_pfns(*q);
    detail::debug_context_load_instance_extension_pfns(*q);
    {
      auto vkCreateDebugUtilsMessengerEXT = q->dbgft.vkCreateDebugUtilsMessengerEXT;
      if (
          auto result = vkCreateDebugUtilsMessengerEXT(q->instance, &debug_info, nullptr, &q->debug_messenger);
          result != VK_SUCCESS
      )
      {
        throw fatal_error{ "Failed to create Vulkan debug messenger." };
      }
    }

    auto required_device_extensions = std::unordered_set<std::string_view>{ 
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    auto available_physical_devices = detail::debug_context_fetch_physical_devices(*q, required_device_extensions);
    // VkQuake does this and therefore it is fine.
    // This should be the first discrete graphics device that has the required extensions.
    // There's really no good way to determine which device should be used in a multi-gpu scenario.
    // Certain factors (like which cards are connected to a monitor for presentation) aren't known.
    // Luckily, multi-gpu scenarios are not the common case for this kind of application.
    // Ideally a user configuration can select a device and that device can be selected on successive runs.
    // TODO user configured device selection.
    q->physical_device = available_physical_devices.front();

    auto enabled_device_features = VkPhysicalDeviceFeatures{ };
    {
      auto vkGetPhysicalDeviceFeatures = q->dbgft.vkGetPhysicalDeviceFeatures;
      vkGetPhysicalDeviceFeatures(q->physical_device, &enabled_device_features);
    }
    debug_context_select_physical_device_queue_families(*q);
    auto device_extensions = std::vector<cstring>(required_device_extensions.size());
    for (auto i = u32{ 0 }; const auto& required_device_extension : required_device_extensions)
    {
      device_extensions[i++] = required_device_extension.data();
    }
    if (q->graphics_transfer_queue_family == q->presentation_queue_family)
    {
      auto queue_info = VkDeviceQueueCreateInfo{ };
      auto priority = 1.0f;
      detail::debug_context_fill_queue_create_info(q->graphics_transfer_queue_family, queue_info, priority);
      detail::context_initialize_vulkan_device(
        *q,
        device_extensions.data(), device_extensions.size(),
        &enabled_device_features,
        &queue_info, 1,
        nullptr
      );
    }
    else
    {
      auto queue_infos = std::array<VkDeviceQueueCreateInfo, 2>{ };
      auto priorites = std::array<float, 2>{ };
      auto& [gfx_queue_info, pres_queue_info] = queue_infos;
      auto& [gfx_priority, pres_priority] = priorites;
      detail::debug_context_fill_queue_create_info(q->graphics_transfer_queue_family, gfx_queue_info, gfx_priority);
      detail::debug_context_fill_queue_create_info(q->presentation_queue_family, pres_queue_info, pres_priority);
      detail::context_initialize_vulkan_device(
        *q,
        device_extensions.data(), device_extensions.size(),
        &enabled_device_features,
        queue_infos.data(), queue_infos.size(),
        nullptr
      );
    }
    detail::context_load_device_pfns(*q);
  }

  debug_context::~debug_context() noexcept {
    auto q = q_ptr<detail::debug_context_impl>();
    detail::context_deinitialize_vulkan_device(*q);
    {
      auto vkDestroyDebugUtilsMessengerEXT = q->dbgft.vkDestroyDebugUtilsMessengerEXT;
      vkDestroyDebugUtilsMessengerEXT(q->instance, q->debug_messenger, nullptr);
    }
    detail::context_deinitialize_vulkan_instance(*q);
    detail::context_deinitialize_x(*q);
  }
}
