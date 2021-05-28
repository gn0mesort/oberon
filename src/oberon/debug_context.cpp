#include "oberon/detail/debug_context_impl.hpp"

#include <cstdio>

#include <vector>
#include <algorithm>
#include <iterator>
#include <optional>

namespace {
  static std::unordered_set<std::string> sg_required_extensions{
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
  };

  static std::unordered_set<std::string> sg_optional_extensions{
    VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
  };

  static std::unordered_set<std::string> sg_required_device_extensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  static std::unordered_set<std::string> sg_optional_device_extensions{
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

  template <typename Dispatch>
  std::optional<vk::PhysicalDevice> select_device(const std::vector<vk::PhysicalDevice>& pdevs, const Dispatch& dl) {
    auto org_devs = oberon::detail::organize_physical_devices(std::begin(pdevs), std::end(pdevs), dl);
    if (!org_devs[vk::PhysicalDeviceType::eDiscreteGpu].empty())
    {
      return org_devs[vk::PhysicalDeviceType::eDiscreteGpu].front();
    }
    if (!org_devs[vk::PhysicalDeviceType::eIntegratedGpu].empty())
    {
      return org_devs[vk::PhysicalDeviceType::eIntegratedGpu].front();
    }
    if (!org_devs[vk::PhysicalDeviceType::eCpu].empty())
    {
      return org_devs[vk::PhysicalDeviceType::eCpu].front();
    }
    return std::nullopt;
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

  const std::unordered_set<std::string>& debug_context::required_device_extensions() {
    return sg_required_device_extensions;
  }

  const std::unordered_set<std::string>& debug_context::optional_device_extensions() {
    return sg_optional_device_extensions;
  }

  debug_context::debug_context(const std::unordered_set<std::string_view>& requested_layers) :
  m_impl{ new detail::debug_context_impl{ } } {
    // X stuff
    {
      auto preferred_screen = int{ };
      m_impl->x_connection = xcb_connect(nullptr, &preferred_screen);
      if (xcb_connection_has_error(m_impl->x_connection))
      {
        // TODO fatal can't connect to X
      }
      m_impl->x_screen = detail::screen_of_display(m_impl->x_connection, preferred_screen);
    }

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

    {
      auto pdevs = m_impl->instance.enumeratePhysicalDevices(m_impl->dl);
      // Filter devices without presentation and graphics support.
      // This is not queue selection.
      auto bad_pdevs = std::remove_if(
        std::begin(pdevs), std::end(pdevs),
        [this](const vk::PhysicalDevice& pdev) {
          {
            auto exts = pdev.enumerateDeviceExtensionProperties(nullptr, m_impl->dl);
            for (const auto& required_extension : sg_required_device_extensions)
            {
              auto pos = std::find_if(
                std::begin(exts), std::end(exts),
                [&required_extension](const auto& prop) {
                  return std::string{ prop.extensionName } == required_extension;
                }
              );
              if (pos == std::end(exts))
              {
                return true;
              }
            }
          }

          auto queues = pdev.getQueueFamilyProperties(m_impl->dl);
          auto has_gfx = false;
          auto has_pre = false;
          {
            auto ctr = u32{ 0 };
            for (const auto& queue : queues)
            {
              // Check graphics support. This implies transfer operation support.
              if (queue.queueFlags & vk::QueueFlagBits::eGraphics)
              {
                has_gfx = true;
              }
              // Check presentation support with XCB.
              if (
                pdev.getXcbPresentationSupportKHR(ctr, m_impl->x_connection, m_impl->x_screen->root_visual, m_impl->dl)
              )
              {
                has_pre = true;
              }
              ++ctr;
            }
          }
          return !(has_gfx && has_pre);
        }
      );
      pdevs.erase(bad_pdevs, std::end(pdevs));
      
      switch (pdevs.size())
      {
      case 0:
        // TODO fatal error no suitable device
      case 1:
        // Exactly one device so select it.
        m_impl->physical_device = pdevs.front();
        break;
      default:
        {
          auto best = select_device(pdevs, m_impl->dl);
          if (!best.has_value())
          {
            // TODO fatal error no suitable device
          }
          m_impl->physical_device = *best;
        }
      }
      
      auto device_info = vk::DeviceCreateInfo{ };
      auto device_exts = std::vector<cstring>(sg_required_device_extensions.size());
      std::transform(
        std::begin(sg_required_device_extensions), std::end(sg_required_device_extensions),
        std::begin(device_exts),
        [](const auto& ext) {
          return ext.c_str();
        }
      );
      device_info.ppEnabledExtensionNames = device_exts.data();
      device_info.enabledExtensionCount = device_exts.size();
      auto features = m_impl->physical_device.getFeatures(m_impl->dl);
      device_info.pEnabledFeatures = &features;
      
      // Select queues
      {
        auto queue_families = m_impl->physical_device.getQueueFamilyProperties(m_impl->dl);
        m_impl->graphics_transfer_queue_family = queue_families.size();
        m_impl->presentation_queue_family = queue_families.size();
        {
          auto ctr = u32{ 0 };
          for (const auto& queue_family : queue_families)
          {
            if (m_impl->graphics_transfer_queue_family >= queue_families.size())
            {
              if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
              {
                m_impl->graphics_transfer_queue_family = ctr;
              }
            }
            if (m_impl->presentation_queue_family >= queue_families.size())
            {
              auto pres = m_impl->physical_device.getXcbPresentationSupportKHR(
                ctr,
                m_impl->x_connection, m_impl->x_screen->root_visual,
                m_impl->dl
              );
              if (pres)
              {
                m_impl->presentation_queue_family = ctr;
              }
            }
            ++ctr;
          }
        }
      }
      auto queue_infos = std::array<vk::DeviceQueueCreateInfo, 2>{ };
      auto& [gfx_info, pres_info] = queue_infos;
      auto priority = f32{ 1.0 };
      gfx_info.pQueuePriorities = &priority;
      gfx_info.queueCount = 1;
      gfx_info.queueFamilyIndex = m_impl->graphics_transfer_queue_family;
      pres_info.pQueuePriorities = &priority;
      pres_info.queueCount = 1;
      pres_info.queueFamilyIndex = m_impl->presentation_queue_family;

      device_info.pQueueCreateInfos = queue_infos.data();
      if (m_impl->graphics_transfer_queue_family == m_impl->presentation_queue_family)
      {
        device_info.queueCreateInfoCount = 1;
      }
      else
      {
        device_info.queueCreateInfoCount = queue_infos.size();
      }

      m_impl->device = m_impl->physical_device.createDevice(device_info, nullptr, m_impl->dl);
      m_impl->graphics_transfer_queue = m_impl->device.getQueue(m_impl->graphics_transfer_queue_family, 0, m_impl->dl);
      m_impl->presentation_queue = m_impl->device.getQueue(m_impl->presentation_queue_family, 0, m_impl->dl);
    }
  }

  debug_context::~debug_context() noexcept {
    m_impl->device.destroy(nullptr, m_impl->dl);
    m_impl->instance.destroyDebugUtilsMessengerEXT(m_impl->debug_messenger, nullptr, m_impl->dl);
    m_impl->instance.destroy(nullptr, m_impl->dl);
    xcb_disconnect(m_impl->x_connection);
  }
}
