#include "oberon/linux/context.hpp"

#include <cstdio>

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
      const auto extensions = std::array<cstring, 3>{ VK_KHR_XCB_SURFACE_EXTENSION_NAME,
                                                      VK_KHR_SURFACE_EXTENSION_NAME,
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
  }

  context::~context() noexcept {
    if (m_vulkan_debug_messenger != VK_NULL_HANDLE)
    {
      destroy_vulkan_debug_messenger();
    }
    destroy_vulkan_instance();
    disconnect_from_x_server();
  }

}
