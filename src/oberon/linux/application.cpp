#include "oberon/linux/application.hpp"
#include "oberon/linux/detail/window_system_impl.hpp"
#include "oberon/linux/detail/render_system_impl.hpp"

#include <array>
#include <iostream>

namespace oberon::linux {

  void application::set_user_data(const ptr<void> user) {
    m_user_data = user;
  }

  void application::set_vulkan_debug_flag(const bool flag) {
    m_vulkan_create_debug_context = flag;
  }

  void application::set_vulkan_device(const u32 index) {
    m_vulkan_device = index;
  }

  int application::run(const ptr<entry_point> main) const {
    auto status = 0;
    auto win_impl = detail::window_system_impl{ };
    auto vkdl = vkfl::loader{ vkGetInstanceProcAddr };
    auto rnd_impl = detail::render_system_impl{ };
    try
    {
      // Initialize X server connection
      {
        auto [ connection, screen_pref ] = detail::x11_create_connection(nullptr);
        auto screen = detail::x11_select_screen(connection, screen_pref);
        win_impl.connection = connection;
        win_impl.screen = screen;
        detail::x11_init_ewmh(win_impl.connection, win_impl.ewmh);
      }
      // Initialize Vulkan loader
      {
        rnd_impl.dl = &vkdl;
      }
      // Initialize Vulkan instance and debug messenger (if enabled)
      {
        if (m_vulkan_create_debug_context)
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
          rnd_impl.instance = detail::vk_create_instance(std::data(layers), std::size(layers), std::data(extensions),
                                                         std::size(extensions), &validation_features, vkdl);
          rnd_impl.debug_messenger = detail::vk_create_debug_utils_messenger(rnd_impl.instance, debug_info, vkdl);
        }
        else
        {
          auto extensions = std::array<cstring, 2>{ VK_KHR_XCB_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME };
          rnd_impl.instance = detail::vk_create_instance(nullptr, 0, std::data(extensions), std::size(extensions),
                                                         nullptr, vkdl);
        }
      }
      // Select Vulkan physical device
      {
        rnd_impl.physical_device = detail::vk_select_physical_device(m_vulkan_device, rnd_impl.instance,
                                                                     win_impl.connection, win_impl.screen, vkdl);
      }
      // Initialize Vulkan device and retrieve queues
      {
        auto extensions = std::array<cstring, 1>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        auto [ device, work_queue_family, work_queue ] = detail::vk_create_device(rnd_impl.physical_device,
                                                                                  std::data(extensions),
                                                                                  std::size(extensions), nullptr, vkdl);
        rnd_impl.device = device;
        rnd_impl.work_queue_family = work_queue_family;
        rnd_impl.work_queue = work_queue;
      }
      auto win = window_system{ &win_impl };
      auto rnd = render_system{ &rnd_impl };

      status = main(win, rnd, m_user_data);
    }
    catch (const error& err)
    {
      std::cerr << "Error: " << err.message() << std::endl;
      status = err.result();
    }
    catch (const std::exception& err)
    {
      std::cerr << "Error: " << err.what() << std::endl;
      status = 1;
    }
    detail::vk_destroy_device(rnd_impl.device, vkdl);
    detail::vk_destroy_debug_messenger(rnd_impl.instance, rnd_impl.debug_messenger, vkdl);
    detail::vk_destroy_instance(rnd_impl.instance, vkdl);
    detail::x11_destroy_connection(win_impl.connection);
    return status;
  }

}
