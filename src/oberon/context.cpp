#include "oberon/detail/context_impl.hpp"

#include <cstring>

#include <string>
#include <vector>
#include <algorithm>

#include "oberon/debug.hpp"
#include "oberon/errors.hpp"
#include "oberon/events.hpp"

#include "oberon/detail/window_impl.hpp"

namespace oberon {
namespace detail {

  iresult store_application_info(
    context_impl& ctx,
    const std::string& name,
    const u16 major, const u16 minor, const u16 patch
  ) noexcept {
    ctx.application_name = name;
    ctx.application_version_major = major;
    ctx.application_version_minor = minor;
    ctx.application_version_patch = patch;
    return 0;
  }

  iresult connect_to_x11(context_impl& ctx, const cstring displayname) noexcept {
    OBERON_PRECONDITION(!ctx.x11_connection);
    OBERON_PRECONDITION(!ctx.x11_screen);
    auto default_screen = int{ 0 };
    ctx.x11_connection = xcb_connect(displayname, &default_screen);
    if (xcb_connection_has_error(ctx.x11_connection))
    {
      return -1;
    }
    ctx.x11_screen = screen_of_display(ctx.x11_connection, default_screen);
    OBERON_POSTCONDITION(ctx.x11_connection);
    OBERON_POSTCONDITION(ctx.x11_screen);
    return 0;
  }

  iresult get_instance_extensions(
    context_impl& ctx,
    const std::unordered_set<std::string>& layers,
    const std::unordered_set<std::string>& required_extensions,
    const std::unordered_set<std::string>& optional_extensions
  ) noexcept {
    OBERON_DECLARE_VK_PFN(ctx.dl, EnumerateInstanceExtensionProperties);
    auto sz = u32{ 0 };
    OBERON_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &sz, nullptr) == VK_SUCCESS);
    auto extension_props = std::vector<VkExtensionProperties>(sz);
    OBERON_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &sz, std::data(extension_props)) == VK_SUCCESS);
    for (const auto& layer : layers)
    {
      OBERON_ASSERT(vkEnumerateInstanceExtensionProperties(std::data(layer), &sz, nullptr) == VK_SUCCESS);
      auto offset = std::size(extension_props);
      extension_props.resize(std::size(extension_props) + sz);
      auto result = vkEnumerateInstanceExtensionProperties(std::data(layer), &sz, std::data(extension_props) + offset);
      OBERON_ASSERT(result == VK_SUCCESS);
    }
    for (const auto& extension_prop : extension_props)
    {
      auto name = extension_prop.extensionName;
      if (required_extensions.contains(name) || optional_extensions.contains(name))
      {
        ctx.instance_extensions.insert(name);
      }
    }
    for (const auto& required_extension : required_extensions)
    {
      if (!ctx.instance_extensions.contains(required_extension))
      {
        return -1;
      }
    }
    OBERON_POSTCONDITION(std::size(ctx.instance_extensions) >= std::size(required_extensions));
    return 0;
  }

  iresult create_vulkan_instance(
    context_impl& ctx,
    const std::unordered_set<std::string>& layers,
    const readonly_ptr<void> next
  ) noexcept {
    OBERON_PRECONDITION(!ctx.instance);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateInstance);

    auto app_info = VkApplicationInfo{ };
    OBERON_INIT_VK_STRUCT(app_info, APPLICATION_INFO);
    app_info.pApplicationName = std::data(ctx.application_name);
    auto ver =
      VK_MAKE_VERSION(ctx.application_version_major, ctx.application_version_minor, ctx.application_version_patch);
    app_info.applicationVersion = ver;
    app_info.pEngineName = "oberon";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    auto lyrs = std::vector<cstring>(std::size(layers));
    for (auto cur = std::begin(lyrs); const auto& layer : layers)
    {
      *(cur++) = std::data(layer);
    }

    auto exts = std::vector<cstring>(std::size(ctx.instance_extensions));
    for (auto cur = std::begin(exts); const auto& extension : ctx.instance_extensions)
    {
      *(cur++) = std::data(extension);
    }

    auto instance_info = VkInstanceCreateInfo{ };
    OBERON_INIT_VK_STRUCT(instance_info, INSTANCE_CREATE_INFO);
    instance_info.pNext = next;
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledLayerNames = std::data(lyrs);
    instance_info.enabledLayerCount = std::size(lyrs);
    instance_info.ppEnabledExtensionNames = std::data(exts);
    instance_info.enabledExtensionCount = std::size(exts);

    if (auto result = vkCreateInstance(&instance_info, nullptr, &ctx.instance); result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(ctx.instance);
    return 0;
  }

namespace {

  struct physical_device_info final {
    VkPhysicalDevice handle{ };
    VkPhysicalDeviceProperties properties{ };
    std::unordered_set<std::string> extensions{ };
    std::vector<VkQueueFamilyProperties> queue_families{ };
  };

  physical_device_info get_physical_device_info(
      const context_impl& ctx,
      const VkPhysicalDevice pdev,
      const std::unordered_set<std::string>& required_extensions,
      const std::unordered_set<std::string>& optional_extensions
    ) {
    OBERON_PRECONDITION(ctx.instance);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceProperties);
    OBERON_DECLARE_VK_PFN(ctx.dl, EnumerateDeviceExtensionProperties);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceQueueFamilyProperties);
    OBERON_PRECONDITION(pdev);
    auto result = physical_device_info{ };
    result.handle = pdev;
    vkGetPhysicalDeviceProperties(result.handle, &result.properties);
    auto sz = u32{ 0 };
    OBERON_ASSERT(vkEnumerateDeviceExtensionProperties(result.handle, nullptr, &sz, nullptr) == VK_SUCCESS);
    auto exts = std::vector<VkExtensionProperties>(sz);
    OBERON_ASSERT(vkEnumerateDeviceExtensionProperties(result.handle, nullptr, &sz, std::data(exts)) == VK_SUCCESS);
    for (const auto& ext : exts)
    {
      if (required_extensions.contains(ext.extensionName) || optional_extensions.contains(ext.extensionName))
      {
        result.extensions.insert(ext.extensionName);
      }
    }
    vkGetPhysicalDeviceQueueFamilyProperties(result.handle, &sz, nullptr);
    result.queue_families.resize(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(result.handle, &sz, std::data(result.queue_families));
    return result;
  }


  u32 score_physical_device(const physical_device_info& info) noexcept {
    switch (info.properties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      return 10;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      return 9;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      return 8;
    default:
      return 0;
    }
  }

  bool physical_device_less(const physical_device_info& a, const physical_device_info& b) {
    return score_physical_device(a) < score_physical_device(b);
  }

}

  iresult select_physical_device(
    context_impl& ctx,
    const std::unordered_set<std::string>& required_extensions,
    const std::unordered_set<std::string>& optional_extensions
  ) noexcept {
    OBERON_PRECONDITION(ctx.instance);
    OBERON_DECLARE_VK_PFN(ctx.dl, EnumeratePhysicalDevices);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceXcbPresentationSupportKHR);
    auto pdev_infos = std::vector<physical_device_info>{ };
    {
      auto sz = u32{ 0 };
      OBERON_ASSERT(vkEnumeratePhysicalDevices(ctx.instance, &sz, nullptr) == VK_SUCCESS);
      auto pdevs = std::vector<VkPhysicalDevice>(sz);
      OBERON_ASSERT(vkEnumeratePhysicalDevices(ctx.instance, &sz, std::data(pdevs)) == VK_SUCCESS);
      pdev_infos.resize(std::size(pdevs));
      // This probably only runs once on most systems.
      for (auto cur = std::begin(pdev_infos); const auto& pdev : pdevs)
      {
        *(cur++) = get_physical_device_info(ctx, pdev, required_extensions, optional_extensions);
      }
    }
    // Ditto this loop too.
    auto filtered_pdev_infos = std::vector<physical_device_info>{ };
    for (const auto& pdev_info : pdev_infos)
    {
      auto has_extensions = true;
      for (const auto& required_extension : required_extensions)
      {
        has_extensions = has_extensions && pdev_info.extensions.contains(required_extension);
      }
      auto has_graphics = false;
      auto has_presentation = false;
      for (auto index = u32{ 0 }; const auto& queue_family : pdev_info.queue_families)
      {
        has_graphics = has_graphics || (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
        auto current_has_presentation =
          vkGetPhysicalDeviceXcbPresentationSupportKHR(
            pdev_info.handle, index,
            ctx.x11_connection, ctx.x11_screen->root_visual
          );
        has_presentation = has_presentation || current_has_presentation;
        ++index;
      }
      if (has_extensions && has_graphics && has_presentation)
      {
        filtered_pdev_infos.push_back(pdev_info);
      }
    }
    if (std::size(filtered_pdev_infos) < 1)
    {
      return -1;
    }
    std::sort(std::begin(filtered_pdev_infos), std::end(filtered_pdev_infos), physical_device_less);
    ctx.physical_device = std::begin(filtered_pdev_infos)->handle;
    ctx.physical_device_properties = std::begin(filtered_pdev_infos)->properties;
    ctx.device_extensions = std::begin(filtered_pdev_infos)->extensions;
    OBERON_POSTCONDITION(ctx.physical_device);
    OBERON_POSTCONDITION(std::size(ctx.device_extensions) >= std::size(required_extensions));
    return 0;
  }

  iresult select_physical_device_queue_families(context_impl& ctx) noexcept {
    OBERON_PRECONDITION(ctx.physical_device);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceQueueFamilyProperties);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceXcbPresentationSupportKHR);

    auto sz = u32{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &sz, nullptr);
    auto queue_families = std::vector<VkQueueFamilyProperties>(sz);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &sz, std::data(queue_families));
    ctx.graphics_transfer_queue_family = std::size(queue_families);
    ctx.presentation_queue_family = std::size(queue_families);

    for (auto index = u32{ 0 }; const auto& queue_family : queue_families)
    {
      auto has_graphics = queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
      auto has_presentation =
        vkGetPhysicalDeviceXcbPresentationSupportKHR(
          ctx.physical_device,
          index,
          ctx.x11_connection,
          ctx.x11_screen->root_visual
        );
      if (has_graphics && has_presentation)
      {
        ctx.graphics_transfer_queue_family = index;
        ctx.presentation_queue_family = index;
        break;
      }
      if (has_graphics && ctx.graphics_transfer_queue_family >= std::size(queue_families))
      {
        ctx.graphics_transfer_queue_family = index;
      }
      if (has_presentation && ctx.presentation_queue_family >= std::size(queue_families))
      {
        ctx.presentation_queue_family = index;
      }
      ++index;
    }
    OBERON_POSTCONDITION(ctx.graphics_transfer_queue_family < std::size(queue_families));
    OBERON_POSTCONDITION(ctx.presentation_queue_family < std::size(queue_families));
    return 0;
  }

  iresult create_vulkan_device(context_impl& ctx, const readonly_ptr<void> next) noexcept {
    OBERON_PRECONDITION(!ctx.device);
    OBERON_PRECONDITION(!ctx.graphics_transfer_queue);
    OBERON_PRECONDITION(!ctx.presentation_queue);
    OBERON_PRECONDITION(ctx.physical_device);

    OBERON_DECLARE_VK_PFN(ctx.dl, GetPhysicalDeviceFeatures);
    OBERON_DECLARE_VK_PFN(ctx.dl, CreateDevice);

    auto device_info = VkDeviceCreateInfo{ };
    OBERON_INIT_VK_STRUCT(device_info, DEVICE_CREATE_INFO);
    device_info.pNext = next;
    auto features = VkPhysicalDeviceFeatures{ };
    vkGetPhysicalDeviceFeatures(ctx.physical_device, &features);
    device_info.pEnabledFeatures = &features;

    auto exts = std::vector<cstring>(std::size(ctx.device_extensions));
    for (auto cur = std::begin(exts); const auto& device_extension : ctx.device_extensions)
    {
      *(cur++) = std::data(device_extension);
    }

    device_info.ppEnabledExtensionNames = std::data(exts);
    device_info.enabledExtensionCount = std::size(exts);

    auto queue_infos = std::array<VkDeviceQueueCreateInfo, 2>{ };
    auto priorities = std::array<float, 2>{ 1.0f, 1.0f };
    device_info.pQueueCreateInfos = std::data(queue_infos);
    if (ctx.graphics_transfer_queue_family == ctx.presentation_queue_family)
    {
      device_info.queueCreateInfoCount = 1;
      auto& main_queue_info = *std::begin(queue_infos);
      OBERON_INIT_VK_STRUCT(main_queue_info, DEVICE_QUEUE_CREATE_INFO);
      main_queue_info.queueFamilyIndex = ctx.graphics_transfer_queue_family;
      main_queue_info.pQueuePriorities = std::data(priorities);
      main_queue_info.queueCount = 1;
    }
    else
    {
      device_info.queueCreateInfoCount = std::size(queue_infos);
      auto& [ graphics_queue_info, presentation_queue_info ] = queue_infos;
      auto& [ graphics_priority, presentation_priority ] = priorities;
      OBERON_INIT_VK_STRUCT(graphics_queue_info, DEVICE_QUEUE_CREATE_INFO);
      graphics_queue_info.queueFamilyIndex = ctx.graphics_transfer_queue_family;
      graphics_queue_info.pQueuePriorities = &graphics_priority;
      graphics_queue_info.queueCount = 1;
      OBERON_INIT_VK_STRUCT(presentation_queue_info, DEVICE_QUEUE_CREATE_INFO);
      presentation_queue_info.queueFamilyIndex = ctx.presentation_queue_family;
      presentation_queue_info.pQueuePriorities = &presentation_priority;
      presentation_queue_info.queueCount = 1;
    }

    if (auto result = vkCreateDevice(ctx.physical_device, &device_info, nullptr, &ctx.device); result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(ctx.device);
    return 0;
  }

  iresult get_device_queues(context_impl& ctx) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, GetDeviceQueue);
    vkGetDeviceQueue(ctx.device, ctx.graphics_transfer_queue_family, 0, &ctx.graphics_transfer_queue);
    vkGetDeviceQueue(ctx.device, ctx.presentation_queue_family, 0, &ctx.presentation_queue);
    OBERON_POSTCONDITION(ctx.graphics_transfer_queue);
    OBERON_POSTCONDITION(ctx.presentation_queue);
    return 0;
  }

  iresult add_window_to_context(const context_impl& ctx, const umax id, const ptr<window> win) noexcept {
    ctx.windows[id] = win;
    return 0;
  }

  iresult remove_window_from_context(const context_impl& ctx, const umax id) noexcept {
    ctx.windows.erase(id);
    return 0;
  }

  iresult poll_x11_event(context_impl& ctx, event& ev) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    auto x11_ev = xcb_poll_for_event(ctx.x11_connection);
    if (!x11_ev)
    {
      ev.type = event_type::empty;
      return 0;
    }
    switch (x11_ev->response_type & 0x7f) // ~0x80
    {
    case XCB_CLIENT_MESSAGE:
      {
        auto client_message_ev = reinterpret_cast<ptr<xcb_client_message_event_t>>(x11_ev);
        ev.window_ptr = ctx.windows.at(client_message_ev->window);
        auto& win_impl = reference_cast<detail::window_impl>(ev.window_ptr->implementation());
        switch (detail::handle_x11_message(win_impl, client_message_ev))
        {
        case WINDOW_MESSAGE_HIDE:
          ev.type = event_type::window_hide;
          break;
        default:
          goto err;
        }
      }
      break;
    case XCB_CONFIGURE_NOTIFY:
      {
        auto configure_ev = reinterpret_cast<ptr<xcb_configure_notify_event_t>>(x11_ev);
        ev.type = event_type::window_configure;
        ev.window_ptr = ctx.windows.at(configure_ev->window);
        auto& win_impl = reference_cast<detail::window_impl>(ev.window_ptr->implementation());
        auto result = detail::handle_x11_configure(win_impl, configure_ev);
        if (OBERON_IS_IERROR(result))
        {
          goto err;
        }
        ev.data.window_configure.bounds = win_impl.bounds;
        ev.data.window_configure.was_repositioned = result & WINDOW_CONFIGURE_REPOSITION_BIT;
        ev.data.window_configure.was_resized = result & WINDOW_CONFIGURE_RESIZE_BIT;
      }
      break;
    default:
      break;
    }
    std::free(x11_ev);
    return 0;
  err:
    std::free(x11_ev);
    return -1;
  }

  iresult destroy_vulkan_device(context_impl& ctx) noexcept {
    if (!ctx.device)
    {
      return 0;
    }
    OBERON_ASSERT(ctx.instance);
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyDevice);
    vkDestroyDevice(ctx.device, nullptr);
    ctx.device = nullptr;
    OBERON_POSTCONDITION(!ctx.device);
    return 0;
  }

  iresult destroy_vulkan_instance(context_impl& ctx) noexcept {
    if (!ctx.instance)
    {
      return 0;
    }
    OBERON_DECLARE_VK_PFN(ctx.dl, DestroyInstance);
    vkDestroyInstance(ctx.instance, nullptr);
    ctx.instance = nullptr;
    OBERON_POSTCONDITION(!ctx.instance);
    return 0;
  }

  iresult disconnect_from_x11(context_impl& ctx) noexcept {
    xcb_disconnect(ctx.x11_connection);
    ctx.x11_connection = nullptr;
    ctx.x11_screen = nullptr;
    OBERON_POSTCONDITION(!ctx.x11_connection);
    OBERON_POSTCONDITION(!ctx.x11_screen);
    return 0;
  }

  iresult is_valid_vulkan_vertex_binding(const context_impl& ctx, const u32 binding) noexcept {
    OBERON_PRECONDITION(ctx.physical_device);
    return binding < ctx.physical_device_properties.limits.maxVertexInputBindings;
  }

  iresult is_valid_vulkan_vertex_location(const context_impl& ctx, const u32 location) noexcept {
    OBERON_PRECONDITION(ctx.physical_device);
    return location < ctx.physical_device_properties.limits.maxVertexInputAttributes;
  }

  iresult wait_for_device_idle(const context_impl& ctx) noexcept {
    OBERON_PRECONDITION(ctx.device);
    OBERON_DECLARE_VK_PFN(ctx.dl, DeviceWaitIdle);
    auto result = vkDeviceWaitIdle(ctx.device);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    return 0;
  }

}

  context::context(const ptr<detail::context_impl> child_impl) : object{ child_impl } { }

  context::context(
    const std::string& application_name,
    const u16 application_version_major,
    const u16 application_version_minor,
    const u16 application_version_patch
  ) : object{ new detail::context_impl{ } } {
    auto& q = reference_cast<detail::context_impl>(implementation());
    detail::store_application_info(
      q,
      application_name,
      application_version_major, application_version_minor, application_version_patch
    );
    if (OBERON_IS_IERROR(detail::connect_to_x11(q, nullptr)))
    {
      throw fatal_error{ "Failed to connect to X11 server." };
    }
    {
      auto required_extensions = std::unordered_set<std::string>{
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
      };
      if (OBERON_IS_IERROR(detail::get_instance_extensions(q, { }, required_extensions, { })))
      {
        throw fatal_error{ "One or more requested Vulkan instance extensions are not available." };
      }
    }
    {
      if (OBERON_IS_IERROR(detail::create_vulkan_instance(q, { }, nullptr)))
      {
        throw fatal_error{ "Failed to create Vulkan instance." };
      }
    }
    q.dl.load(q.instance);
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

  void context::v_dispose() noexcept {
    auto& q = reference_cast<detail::context_impl>(implementation());
    detail::wait_for_device_idle(q);
    detail::destroy_vulkan_device(q);
    detail::destroy_vulkan_instance(q);
    detail::disconnect_from_x11(q);
  }

  context::~context() noexcept {
    dispose();
  }

  bool context::poll_events(event& ev) {
    auto& ctx = reference_cast<detail::context_impl>(implementation());
    poll_x11_event(ctx, ev);
    return ev.type != event_type::empty;
  }

  const std::string& context::application_name() const {
    auto& q = reference_cast<detail::context_impl>(implementation());
    return q.application_name;
  }

}
