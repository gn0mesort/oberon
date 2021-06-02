#include "oberon/detail/context_impl.hpp"

#include <cstring>

#include <string>

namespace oberon {
namespace detail {
  void context_initialize_x(context_impl& ctx, const cstring displayname) {
    auto default_screen = int{ 0 };
    ctx.x_connection = xcb_connect(displayname, &default_screen);
    if (xcb_connection_has_error(ctx.x_connection))
    {
      throw fatal_error{ "Failed to connect to the X server." };
    }
    ctx.x_screen = screen_of_display(ctx.x_connection, default_screen);
  }

  void context_load_vulkan_library(context_impl& ctx) {
    ctx.loader = vkGetInstanceProcAddr;
  }

  void context_load_global_pfns(context_impl& ctx) {
    ctx.vkft->vkGetInstanceProcAddr = ctx.loader;
    ctx.vkft->vkEnumerateInstanceVersion = OBERON_GLOBAL_FN(ctx, vkEnumerateInstanceVersion);
    ctx.vkft->vkEnumerateInstanceLayerProperties = OBERON_GLOBAL_FN(ctx, vkEnumerateInstanceLayerProperties);
    ctx.vkft->vkEnumerateInstanceExtensionProperties = OBERON_GLOBAL_FN(ctx, vkEnumerateInstanceExtensionProperties);
    ctx.vkft->vkCreateInstance = OBERON_GLOBAL_FN(ctx, vkCreateInstance);
  }

  void context_initialize_vulkan_instance(
    context_impl& ctx,
    const cstring application_name,
    const u32 application_version,
    const readonly_ptr<cstring> layers,
    const u32 layer_count,
    const readonly_ptr<cstring> extensions,
    const u32 extension_count,
    const readonly_ptr<void> next
  ) {
    auto app_info = VkApplicationInfo{ };
    std::memset(&app_info, 0, sizeof(VkApplicationInfo));
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = application_version;
    app_info.pEngineName = "oberon";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    auto instance_info = VkInstanceCreateInfo{ };
    std::memset(&instance_info, 0, sizeof(VkInstanceCreateInfo));
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = next;
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledLayerNames = layers;
    instance_info.enabledLayerCount = layer_count;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount = extension_count;

    auto vkCreateInstance = ctx.vkft->vkCreateInstance;
    if (auto result = vkCreateInstance(&instance_info, nullptr, &ctx.instance); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to create Vulkan instance." };
    }
  }


  void context_load_instance_pfns(context_impl& ctx) {
    ctx.vkft->vkEnumeratePhysicalDevices = OBERON_INSTANCE_FN(ctx, vkEnumeratePhysicalDevices);
    ctx.vkft->vkEnumerateDeviceExtensionProperties = OBERON_INSTANCE_FN(ctx, vkEnumerateDeviceExtensionProperties);
    ctx.vkft->vkGetPhysicalDeviceProperties = OBERON_INSTANCE_FN(ctx, vkGetPhysicalDeviceProperties);
    ctx.vkft->vkGetPhysicalDeviceFeatures = OBERON_INSTANCE_FN(ctx, vkGetPhysicalDeviceFeatures);
    ctx.vkft->vkGetPhysicalDeviceQueueFamilyProperties =
      OBERON_INSTANCE_FN(ctx, vkGetPhysicalDeviceQueueFamilyProperties);
    ctx.vkft->vkCreateDevice = OBERON_INSTANCE_FN(ctx, vkCreateDevice);
    ctx.vkft->vkGetDeviceProcAddr = OBERON_INSTANCE_FN(ctx, vkGetDeviceProcAddr);
    ctx.vkft->vkDestroyInstance = OBERON_INSTANCE_FN(ctx, vkDestroyInstance);
  }

  void context_initialize_vulkan_device(
    context_impl& ctx,
    const readonly_ptr<cstring> extensions,
    const u32 extension_count,
    const readonly_ptr<VkPhysicalDeviceFeatures> features,
    const readonly_ptr<VkDeviceQueueCreateInfo> queue_infos,
    const u32 queue_info_count,
    const readonly_ptr<void> next
  ) {
    auto device_info = VkDeviceCreateInfo{ };
    std::memset(&device_info, 0, sizeof(VkDeviceQueueCreateInfo));
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = next;
    device_info.pEnabledFeatures = features;
    device_info.ppEnabledExtensionNames = extensions;
    device_info.enabledExtensionCount = extension_count;
    device_info.pQueueCreateInfos = queue_infos;
    device_info.queueCreateInfoCount = queue_info_count;

    auto vkCreateDevice = ctx.vkft->vkCreateDevice;
    if (auto result = vkCreateDevice(ctx.physical_device, &device_info, nullptr, &ctx.device); result != VK_SUCCESS)
    {
      throw fatal_error{ "Failed to create Vulkan device." };
    }
  }

  void context_load_device_pfns(context_impl& ctx) {
    ctx.vkft->vkDestroyDevice = OBERON_DEVICE_FN(ctx, vkDestroyDevice);
  }

  void context_deinitialize_vulkan_device(context_impl& ctx) {
    auto vkDestroyDevice = ctx.vkft->vkDestroyDevice;
    vkDestroyDevice(ctx.device, nullptr);
    ctx.device = VK_NULL_HANDLE;
  }

  void context_deinitialize_vulkan_instance(context_impl& ctx) {
    auto vkDestroyInstance = ctx.vkft->vkDestroyInstance;
    vkDestroyInstance(ctx.instance, nullptr);
    ctx.instance = VK_NULL_HANDLE;
  }

  void context_deinitialize_x(context_impl& ctx) {
    xcb_disconnect(ctx.x_connection);
    ctx.x_connection = nullptr;
    ctx.x_screen = nullptr;
  }
}

  context::context(const ptr<detail::context_impl> child_impl) : object{ child_impl } { }
}
