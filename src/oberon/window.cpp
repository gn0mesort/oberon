#include "oberon/detail/window_impl.hpp"

#include <cstring>

#include "oberon/debug.hpp"
#include "oberon/events.hpp"
#include "oberon/errors.hpp"

#include "oberon/detail/context_impl.hpp"

namespace oberon {
namespace detail {

  iresult create_x11_window(const context_impl& ctx, window_impl& window, const bounding_rect& bounds) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    OBERON_PRECONDITION(ctx.x11_screen);
    window.x11_window = xcb_generate_id(ctx.x11_connection); // Can this fail??
    {
      auto depth = XCB_COPY_FROM_PARENT;
      auto parent = ctx.x11_screen->root;
      auto border = u16{ 0 };
      auto visual = XCB_COPY_FROM_PARENT;
      auto window_class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
      auto value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
      auto event_mask = XCB_EVENT_MASK_EXPOSURE |
                        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                        XCB_EVENT_MASK_KEY_PRESS |
                        XCB_EVENT_MASK_KEY_RELEASE |
                        XCB_EVENT_MASK_POINTER_MOTION |
                        XCB_EVENT_MASK_BUTTON_PRESS |
                        XCB_EVENT_MASK_BUTTON_RELEASE |
                        XCB_EVENT_MASK_ENTER_WINDOW |
                        XCB_EVENT_MASK_FOCUS_CHANGE |
                        XCB_EVENT_MASK_PROPERTY_CHANGE;
      auto value_array = std::array<u32, 3>{ ctx.x11_screen->black_pixel, false, static_cast<u32>(event_mask) };
      xcb_create_window(
        ctx.x11_connection,
        depth,
        window.x11_window,
        parent,
        bounds.position.x, bounds.position.y, bounds.size.width, bounds.size.height,
        border,
        window_class,
        visual,
        value_mask, std::data(value_array)
      );
    }
    {
      auto protocols_cookie = xcb_intern_atom(ctx.x11_connection, false, 12, "WM_PROTOCOLS");
      auto delete_cookie = xcb_intern_atom(ctx.x11_connection, false, 16, "WM_DELETE_WINDOW");
      auto* protocols_reply = xcb_intern_atom_reply(ctx.x11_connection, protocols_cookie, nullptr);
      auto* delete_reply = xcb_intern_atom_reply(ctx.x11_connection, delete_cookie, nullptr);
      window.x11_wm_protocols_atom = protocols_reply->atom;
      window.x11_delete_atom = delete_reply->atom;
      std::free(protocols_reply);
      std::free(delete_reply);
      xcb_change_property(
        ctx.x11_connection,
        XCB_PROP_MODE_REPLACE,
        window.x11_window,
        window.x11_wm_protocols_atom,
        XCB_ATOM_ATOM,
        32,
        1,
        &window.x11_delete_atom
      );
      xcb_change_property(
        ctx.x11_connection,
        XCB_PROP_MODE_REPLACE,
        window.x11_window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        std::size(ctx.application_name),
        std::data(ctx.application_name)
      );
    }
    window.bounds = bounds;
    OBERON_POSTCONDITION(window.x11_window);
    return 0;
  }

  iresult create_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.instance);
    OBERON_PRECONDITION(ctx.vkft.vkCreateXcbSurfaceKHR);
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    OBERON_PRECONDITION(window.x11_window);
    auto vkCreateXcbSurfaceKHR = ctx.vkft.vkCreateXcbSurfaceKHR;
    auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
    OBERON_INIT_VK_STRUCT(surface_info, XCB_SURFACE_CREATE_INFO_KHR);
    surface_info.connection = ctx.x11_connection;
    surface_info.window = window.x11_window;
    auto result = vkCreateXcbSurfaceKHR(ctx.instance, &surface_info, nullptr, &window.surface);
    if (result != VK_SUCCESS)
    {
      return result;
    }
    OBERON_POSTCONDITION(window.surface);
    return 0;
  }

  iresult retrieve_vulkan_surface_info(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.physical_device);
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfaceFormatsKHR);
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfacePresentModesKHR);
    OBERON_PRECONDITION(window.surface);
    auto vkGetPhysicalDeviceSurfaceCapabilitiesKHR = ctx.vkft.vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    auto vkGetPhysicalDeviceSurfaceFormatsKHR = ctx.vkft.vkGetPhysicalDeviceSurfaceFormatsKHR;
    auto vkGetPhysicalDeviceSurfacePresentModesKHR = ctx.vkft.vkGetPhysicalDeviceSurfacePresentModesKHR;

    auto result =
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, window.surface, &window.surface_capabilities);
    OBERON_ASSERT(result == VK_SUCCESS);
    auto sz = u32{ 0 };
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, window.surface, &sz, nullptr);
    OBERON_ASSERT(result == VK_SUCCESS);
    window.surface_formats.resize(sz);
    result =
      vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, window.surface, &sz, std::data(window.surface_formats));
    OBERON_ASSERT(result == VK_SUCCESS);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physical_device, window.surface, &sz, nullptr);
    OBERON_ASSERT(result == VK_SUCCESS);
    window.presentation_modes.resize(sz);
    auto data = std::data(window.presentation_modes);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physical_device, window.surface, &sz, data);
    OBERON_ASSERT(result == VK_SUCCESS);

    return 0;
  }

namespace {

  VkSurfaceFormatKHR select_surface_format(const std::vector<VkSurfaceFormatKHR>& surface_formats) {
    auto criteria = [](const VkSurfaceFormatKHR& surface_format) {
      if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
          surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
      {
        return true;
      }
      return false;
    };
    auto pos = std::find_if(std::begin(surface_formats), std::end(surface_formats), criteria);
    if (pos == std::end(surface_formats))
    {
      return surface_formats.front();
    }
    return *pos;
  }

}

  iresult create_vulkan_swapchain(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.physical_device);
    OBERON_PRECONDITION(ctx.device);
    OBERON_PRECONDITION(window.surface);
    OBERON_PRECONDITION(std::size(window.presentation_modes) > 0);
    OBERON_PRECONDITION(std::size(window.surface_formats) > 0);
    OBERON_PRECONDITION(ctx.vkft.vkGetPhysicalDeviceSurfaceSupportKHR);
    OBERON_PRECONDITION(ctx.vkft.vkCreateSwapchainKHR);
    OBERON_PRECONDITION(ctx.vkft.vkGetSwapchainImagesKHR);
    OBERON_PRECONDITION(ctx.vkft.vkCreateImageView);

    auto vkGetPhysicalDeviceSurfaceSupportKHR = ctx.vkft.vkGetPhysicalDeviceSurfaceSupportKHR;
    auto vkCreateSwapchainKHR = ctx.vkft.vkCreateSwapchainKHR;
    auto vkGetSwapchainImagesKHR = ctx.vkft.vkGetSwapchainImagesKHR;
    auto vkCreateImageView = ctx.vkft.vkCreateImageView;

    // Recheck surface support. This is dumb but required by Vulkan.
    {
      auto support = VkBool32{ };
      auto presentation_queue_family = ctx.presentation_queue_family;
      auto result =
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx.physical_device, presentation_queue_family, window.surface, &support);
      OBERON_ASSERT(result == VK_SUCCESS);
      if (!support)
      {
        return -1;
      }
    }
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    OBERON_INIT_VK_STRUCT(swapchain_info, SWAPCHAIN_CREATE_INFO_KHR);
    swapchain_info.surface = window.surface;
    swapchain_info.minImageCount = window.surface_capabilities.minImageCount + 1;
    if (
      window.surface_capabilities.maxImageCount &&
      swapchain_info.minImageCount > window.surface_capabilities.maxImageCount
    )
    {
      swapchain_info.minImageCount = window.surface_capabilities.maxImageCount;
    }
    // FIFO mode support is required by standard.
    // This should be selected by user input instead of the library.
    // Personally I think offer these as FIFO, FIFO Relaxed, Immediate, and Mailbox are better than
    // Offering them as Vsync, Double/triple buffering, etc.
    swapchain_info.presentMode = window.current_presentation_mode;
    // This will probably be finicky.
    if (window.current_surface_format.format == VK_FORMAT_UNDEFINED)
    {
      window.current_surface_format = select_surface_format(window.surface_formats);
    }
    swapchain_info.imageFormat = window.current_surface_format.format;
    swapchain_info.imageColorSpace = window.current_surface_format.colorSpace;
    if (window.surface_capabilities.currentExtent.width != std::numeric_limits<u32>::max())
    {
      swapchain_info.imageExtent = window.surface_capabilities.currentExtent;
    }
    else
    {
      auto& capabilities = window.surface_capabilities;
      auto actual_extent = VkExtent2D{
        static_cast<u32>(window.bounds.size.width), static_cast<u32>(window.bounds.size.height)
      };
      swapchain_info.imageExtent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      swapchain_info.imageExtent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    // More is for head mounted displays?
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (ctx.graphics_transfer_queue_family == ctx.presentation_queue_family)
    {
      swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
      auto queue_family_indices =
        std::array<u32, 2>{ ctx.graphics_transfer_queue_family, ctx.presentation_queue_family };
      swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapchain_info.pQueueFamilyIndices = std::data(queue_family_indices);
      swapchain_info.queueFamilyIndexCount = std::size(queue_family_indices);
    }
    swapchain_info.preTransform = window.surface_capabilities.currentTransform;
    // Should this ever be any other value?
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.clipped = true;
    if (auto result = vkCreateSwapchainKHR(ctx.device, &swapchain_info, nullptr, &window.swapchain);
        result != VK_SUCCESS)
    {
      return result;
    }
    {
      auto sz = u32{ 0 };
      auto result = vkGetSwapchainImagesKHR(ctx.device, window.swapchain, &sz, nullptr);
      OBERON_ASSERT(result == VK_SUCCESS);
      window.swapchain_images.resize(sz);
      result = vkGetSwapchainImagesKHR(ctx.device, window.swapchain, &sz, std::data(window.swapchain_images));
      OBERON_ASSERT(result == VK_SUCCESS);
    }
    {
      window.swapchain_image_views.resize(std::size(window.swapchain_images));
      auto image_view_info = VkImageViewCreateInfo{ };
      OBERON_INIT_VK_STRUCT(image_view_info, IMAGE_VIEW_CREATE_INFO);
      image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      image_view_info.format = window.current_surface_format.format;
      image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_view_info.subresourceRange.baseArrayLayer = 0;
      image_view_info.subresourceRange.layerCount = 1;
      image_view_info.subresourceRange.baseMipLevel = 0;
      image_view_info.subresourceRange.levelCount = 1;
      for (auto cur = std::begin(window.swapchain_image_views); const auto& swapchain_image : window.swapchain_images)
      {
        image_view_info.image = swapchain_image;
        auto& image_view = *(cur++);
        if (auto result = vkCreateImageView(ctx.device, &image_view_info, nullptr, &image_view); result == VK_SUCCESS)
        {
          return result;
        }
      }
    }
    OBERON_POSTCONDITION(window.swapchain);
    OBERON_POSTCONDITION(std::size(window.swapchain_images) > 0);
    OBERON_POSTCONDITION(std::size(window.swapchain_image_views) > 0);
    OBERON_POSTCONDITION(std::size(window.swapchain_images) == std::size(window.swapchain_image_views));
    return 0;
  }

  iresult display_x11_window(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    xcb_map_window(ctx.x11_connection, window.x11_window);
    xcb_flush(ctx.x11_connection);
    return 0;
  }

  iresult handle_x11_expose(window_impl& window, const events::window_expose_data& expose) noexcept {
    return 0;
  }

  iresult handle_x11_message(window_impl& window, const events::window_message_data& message) noexcept {
    OBERON_PRECONDITION(window.x11_window);
    OBERON_PRECONDITION(window.x11_delete_atom != XCB_ATOM_NONE);
    if (window.x11_delete_atom == reinterpret_cast<readonly_ptr<xcb_atom_t>>(std::data(message.content))[0])
    {
      window.was_close_requested = true;
    }
    return 0;
  }

  iresult handle_x11_configure(window_impl& window, const events::window_configure_data& configure) noexcept {
    OBERON_PRECONDITION(window.x11_window);
    if (configure.bounds.size.width != window.bounds.size.width ||
        configure.bounds.size.height != window.bounds.size.height)
    {
      //TODO resized: recreate swapchain etc.
    }
    if (configure.bounds.position.x != window.bounds.position.x ||
        configure.bounds.position.y != window.bounds.position.y)
    {
      //TODO repositioned: probably just ignore. Positions don't seem super meaningful.
      // The reported positions are influenced by the window manager theme for instance. Themes with chunkier window
      // decorations have larger offsets from 0 on either axis.
    }
    window.bounds = configure.bounds;
    return 0;
  }

  iresult hide_x11_window(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    xcb_unmap_window(ctx.x11_connection, window.x11_window);
    xcb_flush(ctx.x11_connection);
    return 0;
  }

  iresult destroy_vulkan_swapchain(const context_impl& ctx, window_impl& window) noexcept {
    if (!window.swapchain)
    {
      return 0;
    }
    OBERON_ASSERT(ctx.device);
    OBERON_ASSERT(ctx.vkft.vkDestroySwapchainKHR);
    OBERON_ASSERT(ctx.vkft.vkDestroyImageView);
    auto vkDestroyImageView = ctx.vkft.vkDestroyImageView;
    auto vkDestroySwapchainKHR = ctx.vkft.vkDestroySwapchainKHR;

    for (const auto& swapchain_image_view : window.swapchain_image_views)
    {
      vkDestroyImageView(ctx.device, swapchain_image_view, nullptr);
    }
    vkDestroySwapchainKHR(ctx.device, window.swapchain, nullptr);
    window.swapchain_images.resize(0);
    window.swapchain_image_views.resize(0);
    window.swapchain = nullptr;
    OBERON_POSTCONDITION(!window.swapchain);
    OBERON_POSTCONDITION(std::size(window.swapchain_images) == 0);
    OBERON_POSTCONDITION(std::size(window.swapchain_image_views) == 0);
    return 0;
  }

  iresult destroy_vulkan_surface(const context_impl& ctx, window_impl& window) noexcept {
    if (!window.surface)
    {
      return 0;
    }
    OBERON_ASSERT(ctx.instance);
    OBERON_ASSERT(ctx.vkft.vkDestroySurfaceKHR);
    auto vkDestroySurfaceKHR = ctx.vkft.vkDestroySurfaceKHR;
    vkDestroySurfaceKHR(ctx.instance, window.surface, nullptr);
    window.surface = nullptr;
    OBERON_POSTCONDITION(!window.surface);
    return 0;
  }

  iresult destroy_x11_window(const context_impl& ctx, window_impl& window) noexcept {
    OBERON_PRECONDITION(ctx.x11_connection);
    OBERON_PRECONDITION(!xcb_connection_has_error(ctx.x11_connection));
    xcb_destroy_window(ctx.x11_connection, window.x11_window);
    window.x11_window = 0;
    OBERON_POSTCONDITION(!window.x11_window);
    return 0;
  }

}

  void window::v_dispose() noexcept {
    auto q = q_ptr<detail::window_impl>();
    detail::hide_x11_window(*m_parent, *q);
    detail::destroy_vulkan_swapchain(*m_parent, *q);
    detail::destroy_vulkan_surface(*m_parent, *q);
    detail::destroy_x11_window(*m_parent, *q);
  }

  window::window(const ptr<detail::window_impl> child_impl) : object{ child_impl } { }

  window::window(context& ctx) : object{ new detail::window_impl{ } } {
  }

  window::window(context& ctx, const bounding_rect& bounds) : object{ new detail::window_impl{ } } {
    auto q = q_ptr<detail::window_impl>();
    m_parent = &reinterpret_cast<detail::context_impl&>(ctx.implementation());
    detail::create_x11_window(*m_parent, *q, bounds);
    if (OBERON_IS_IERROR(detail::create_vulkan_surface(*m_parent, *q)))
    {
      throw fatal_error{ "Failed to create Vulkan window surface." };
    }
    detail::retrieve_vulkan_surface_info(*m_parent, *q);
    if (OBERON_IS_IERROR(detail::create_vulkan_swapchain(*m_parent, *q)))
    {
      throw fatal_error{ "Failed to create Vulkan window swapchain." };
    }
    detail::display_x11_window(*m_parent, *q);
  }

  window::~window() noexcept {
    dispose();
  }

  imax window::id() const {
    auto q = q_ptr<detail::window_impl>();
    return q->x11_window;
  }

  bool window::should_close() const {
    auto q = q_ptr<detail::window_impl>();
    return q->was_close_requested;
  }

  const extent_2d& window::size() const {
    auto q = q_ptr<detail::window_impl>();
    return q->bounds.size;
  }

  usize window::width() const {
    return this->size().width;
  }

  usize window::height() const {
    return this->size().height;
  }

  window& window::notify(const event& ev) {
    if (ev.window_id != this->id())
    {
      return *this;
    }
    switch (ev.type)
    {
    case event_type::window_expose:
      return this->notify(ev.data.window_expose);
    case event_type::window_message:
      return this->notify(ev.data.window_message);
    case event_type::window_configure:
      return this->notify(ev.data.window_configure);
    default:
      return *this;
    }
  }

  window& window::notify(const events::window_expose_data& expose) {
    auto q = q_ptr<detail::window_impl>();
    detail::handle_x11_expose(*q, expose);
    return *this;
  }

  window& window::notify(const events::window_message_data& message) {
    auto q = q_ptr<detail::window_impl>();
    detail::handle_x11_message(*q, message);
    return *this;
  }

  window& window::notify(const events::window_configure_data& configure) {
    auto q = q_ptr<detail::window_impl>();
    detail::handle_x11_configure(*q, configure);
    return *this;
  }
/*
  window_message window::translate_message(const events::window_message_data& message) const {
    auto q = q_ptr<detail::window_impl>();
    auto result = window_message{ };
    translate_x11_message(*q, message.content, result);
    return result;
  }
*/
}
