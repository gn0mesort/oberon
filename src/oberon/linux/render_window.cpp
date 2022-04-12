#include "oberon/linux/detail/render_window_impl.hpp"

#include <cstdlib>

#include <limits>
#include <algorithm>

#include "oberon/debug.hpp"

#include "oberon/linux/window_system.hpp"
#include "oberon/linux/render_system.hpp"

#include "oberon/linux/detail/window_system_impl.hpp"
#include "oberon/linux/detail/render_system_impl.hpp"


namespace oberon::linux::detail {

  void render_window_impl_dtor::operator()(const ptr<render_window_impl> p) const noexcept {
    delete p;
  }

  std::pair<xcb_window_t, xcb_atom_t> x11_create_window(const xcb_ewmh_connection_t& ewmh,
                                                        const ptr<xcb_screen_t> screen,
                                                        const std::string_view title,
                                                        const bounding_rect& bounds) {
    auto id = xcb_generate_id(ewmh.connection);
    auto depth = XCB_COPY_FROM_PARENT;
    auto parent = screen->root;
    auto border = u16{ 0 };
    auto visual = XCB_COPY_FROM_PARENT;
    auto window_class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
    auto value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    auto event_mask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
                      XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS |
                      XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
                      XCB_EVENT_MASK_PROPERTY_CHANGE;
    auto value_array = std::array<u32, 3>{ screen->black_pixel, false, static_cast<u32>(event_mask) };
    xcb_create_window(ewmh.connection, depth, id, parent, bounds.position.x, bounds.position.y, bounds.size.width,
                      bounds.size.height, border, window_class, visual, value_mask, std::data(value_array));
    xcb_icccm_set_wm_name(ewmh.connection, id, ewmh.UTF8_STRING, 8, std::size(title), std::data(title));
    auto size_hints = xcb_size_hints_t{ };
    xcb_icccm_size_hints_set_max_size(&size_hints, bounds.size.width, bounds.size.height);
    xcb_icccm_size_hints_set_min_size(&size_hints, bounds.size.width, bounds.size.height);
    xcb_icccm_set_wm_normal_hints(ewmh.connection, id, &size_hints);
    auto wm_delete = x11_atom(ewmh.connection, "WM_DELETE_WINDOW");
    xcb_icccm_set_wm_protocols(ewmh.connection, id, ewmh.WM_PROTOCOLS, 1, &wm_delete);
    return { id, wm_delete };
  }

  bounding_rect x11_get_current_geometry(const ptr<xcb_connection_t> connection, const xcb_window_t window) {
    OBERON_PRECONDITION(connection != nullptr);
    OBERON_PRECONDITION(window != XCB_NONE);
    auto req = xcb_get_geometry(connection, window);
    auto res = bounding_rect{ };
    auto reply = xcb_get_geometry_reply(connection, req, nullptr);
    res.position.x = reply->x;
    res.position.y = reply->y;
    res.size.width = reply->width;
    res.size.height = reply->height;
    std::free(reply);
    return res;
  }

  VkSurfaceKHR vk_create_surface(const VkInstance instance, const ptr<xcb_connection_t> connection,
                                 const xcb_window_t window, const vkfl::loader& dl) {
    OBERON_PRECONDITION(instance != VK_NULL_HANDLE);
    OBERON_PRECONDITION(connection != nullptr);
    OBERON_PRECONDITION(window != XCB_NONE);
    OBERON_PRECONDITION(dl.loaded_instance() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() == instance);
    OBERON_DECLARE_VK_PFN(dl, CreateXcbSurfaceKHR);
    auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
    surface_info.sType = OBERON_VK_STRUCT(XCB_SURFACE_CREATE_INFO_KHR);
    surface_info.connection = connection;
    surface_info.window = window;
    auto surface = VkSurfaceKHR{ };
    OBERON_VK_SUCCEEDS(vkCreateXcbSurfaceKHR(instance, &surface_info, nullptr, &surface),
                       vk_surface_create_failed_error{ });
    return surface;
  }

  void vk_configure_swapchain(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface,
                              const bounding_rect& bounds, const u32 image_count, vk_swapchain_configuration& config,
                              const vkfl::loader& dl) {
    OBERON_PRECONDITION(surface != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() != VK_NULL_HANDLE);
    OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceSurfaceFormatsKHR);
    OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceSurfacePresentModesKHR);
    auto sz = u32{ 0 };
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &sz, nullptr),
                       vk_surface_format_enumeration_failed_error{ });
    config.available_surface_formats.resize(sz);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &sz,
                                                            std::data(config.available_surface_formats)),
                       vk_surface_format_enumeration_failed_error{ });
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &sz, nullptr),
                       vk_presentation_mode_enumeration_failed_error{ });
    config.available_present_modes.resize(sz);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &sz,
                                                                 std::data(config.available_present_modes)),
                       vk_presentation_mode_enumeration_failed_error{ });
    // TODO: something more intelligent than this.
    {
      auto found = false;
      for (const auto& present_mode : config.available_present_modes)
      {
        found = found || present_mode == config.current_present_mode;
      }
      if (!found)
      {
        config.current_present_mode = VK_PRESENT_MODE_FIFO_KHR; // Only FIFO *MUST* be supported.
      }
    }
    // TODO: something more intelligent than this.
    {
      auto found = false;
      for (const auto& surface_format : config.available_surface_formats)
      {
        found = found || (surface_format.format == config.current_surface_format.format &&
                          surface_format.colorSpace == config.current_surface_format.colorSpace);
      }
      if (!found)
      {
        config.current_surface_format = config.available_surface_formats.front();
      }
    }
    {
      OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceSurfaceCapabilitiesKHR);
      OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &config.capabilities),
                         vk_retrieve_surface_capabilities_failed_error{ });
      if (config.capabilities.currentExtent.width != std::numeric_limits<u32>::max())
      {
        config.extent = config.capabilities.currentExtent;
      }
      else
      {
        config.extent = VkExtent2D{ bounds.size.width, bounds.size.height };
        config.extent.width = std::clamp(config.extent.width, config.capabilities.minImageExtent.width,
                                         config.capabilities.maxImageExtent.width);
        config.extent.height = std::clamp(config.extent.height, config.capabilities.minImageExtent.height,
                                          config.capabilities.maxImageExtent.height);
      }
    }
    {
      config.image_count = std::clamp(image_count, config.capabilities.minImageCount,
                                      config.capabilities.maxImageCount);
    }
  }

  VkSwapchainKHR vk_create_swapchain(const VkPhysicalDevice physical_device, const VkDevice device,
                                     const VkSurfaceKHR surface, const VkSwapchainKHR old, const u32 work_queue_family,
                                     const vk_swapchain_configuration& config, const vkfl::loader& dl) {
    OBERON_PRECONDITION(physical_device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(surface != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() == device);
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    swapchain_info.sType = OBERON_VK_STRUCT(SWAPCHAIN_CREATE_INFO_KHR);
    swapchain_info.surface = surface;
    swapchain_info.oldSwapchain = old;
    swapchain_info.clipped = true;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchain_info.preTransform = config.capabilities.currentTransform;
    swapchain_info.imageColorSpace = config.current_surface_format.colorSpace;
    swapchain_info.imageFormat = config.current_surface_format.format;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.minImageCount = config.image_count;
    swapchain_info.imageExtent = config.extent;
    swapchain_info.queueFamilyIndexCount = 1;
    swapchain_info.pQueueFamilyIndices = &work_queue_family;
    swapchain_info.presentMode = config.current_present_mode;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    OBERON_DECLARE_VK_PFN(dl, GetPhysicalDeviceSurfaceSupportKHR);
    {
      auto support = VkBool32{ };
      OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, work_queue_family, surface, &support),
                         vk_surface_unsupported_error{ });
      OBERON_INVARIANT(support, vk_surface_unsupported_error{ });
    }
    auto swapchain = VkSwapchainKHR{ };
    OBERON_DECLARE_VK_PFN(dl, CreateSwapchainKHR);
    OBERON_VK_SUCCEEDS(vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain),
                       vk_swapchain_create_failed_error{ });
    OBERON_POSTCONDITION(swapchain != VK_NULL_HANDLE);
    return swapchain;
  }

  std::pair<std::vector<VkImage>, std::vector<VkImageView>>
    vk_create_swapchain_images(const VkDevice device, const VkSwapchainKHR swapchain,
                               const vk_swapchain_configuration& config, const vkfl::loader& dl) {
    OBERON_PRECONDITION(device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() == device);
    OBERON_PRECONDITION(swapchain != VK_NULL_HANDLE);
    OBERON_DECLARE_VK_PFN(dl,GetSwapchainImagesKHR);
    auto sz = u32{ 0 };
    OBERON_VK_SUCCEEDS(vkGetSwapchainImagesKHR(device, swapchain, &sz, nullptr),
                       vk_swapchain_image_retrieve_failed_error{ });
    auto swapchain_images = std::vector<VkImage>(sz);
    OBERON_VK_SUCCEEDS(vkGetSwapchainImagesKHR(device, swapchain, &sz, std::data(swapchain_images)),
                       vk_swapchain_image_retrieve_failed_error{ });
    auto swapchain_image_views = std::vector<VkImageView>(sz);
    auto image_view_info = VkImageViewCreateInfo{ };
    image_view_info.sType = OBERON_VK_STRUCT(IMAGE_VIEW_CREATE_INFO);
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = config.current_surface_format.format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    OBERON_DECLARE_VK_PFN(dl, CreateImageView);
    {
      auto cur = std::begin(swapchain_image_views);
      for (const auto& image : swapchain_images)
      {
        image_view_info.image = image;
        OBERON_VK_SUCCEEDS(vkCreateImageView(device, &image_view_info, nullptr, &(*cur)),
                           vk_image_view_create_failed_error{ });
        ++cur;
      }
    }
    return { swapchain_images, swapchain_image_views };
  }

  void vk_destroy_swapchain_images(const VkDevice device, const std::vector<VkImage>&,
                                   const std::vector<VkImageView>& image_views, const vkfl::loader& dl) noexcept {
    OBERON_PRECONDITION(device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() == device);
    OBERON_DECLARE_VK_PFN(dl, DestroyImageView);
    for (const auto& image_view : image_views)
    {
      vkDestroyImageView(device, image_view, nullptr);
    }
  }

  void vk_destroy_swapchain(const VkDevice device, const VkSwapchainKHR swapchain, const vkfl::loader& dl) noexcept {
    OBERON_PRECONDITION(device != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_device() == device);
    if (swapchain != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(dl, DestroySwapchainKHR);
      vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
  }

  void vk_destroy_surface(const VkInstance instance, const VkSurfaceKHR surface, const vkfl::loader& dl) noexcept {
    OBERON_PRECONDITION(instance != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(dl.loaded_instance() == instance);
    if (surface != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(dl, DestroySurfaceKHR);
      vkDestroySurfaceKHR(instance, surface, nullptr);
    }
  }

  void x11_destroy_window(const ptr<xcb_connection_t> connection, xcb_window_t window) noexcept {
    OBERON_PRECONDITION(connection != nullptr);
    xcb_destroy_window(connection, window);
  }

}

namespace oberon::linux {

  void render_window::regenerate_render_artifacts() {
    auto& win = m_impl->parent_window_system_backend;
    auto& rnd = m_impl->parent_render_system_backend;
    m_impl->bounds = detail::x11_get_current_geometry(win->connection, m_impl->window_handle);
    detail::vk_destroy_swapchain_images(rnd->device, m_impl->swapchain_images, m_impl->swapchain_image_views, *rnd->dl);
    detail::vk_configure_swapchain(rnd->physical_device, m_impl->surface, m_impl->bounds, 3, m_impl->swapchain_config,
                                   *rnd->dl);
    m_impl->swapchain = detail::vk_create_swapchain(rnd->physical_device, rnd->device, m_impl->surface,
                                                    m_impl->swapchain, rnd->work_queue_family,
                                                    m_impl->swapchain_config, *rnd->dl);
    auto [ swapchain_images, swapchain_image_views ] = detail::vk_create_swapchain_images(rnd->device,
                                                                                          m_impl->swapchain,
                                                                                          m_impl->swapchain_config,
                                                                                          *rnd->dl);
    m_impl->swapchain_images = swapchain_images;
    m_impl->swapchain_image_views = swapchain_image_views;
  }

  render_window::render_window(window_system& win, render_system& rnd, const std::string_view title,
                               const bounding_rect& bounds) : m_impl{ new detail::render_window_impl{ } } {
    OBERON_PRECONDITION(m_impl != nullptr);
    m_impl->parent_window_system_backend = win.m_impl;
    m_impl->parent_render_system_backend = rnd.m_impl;
    // Setup X window
    {
      auto& backend = m_impl->parent_window_system_backend;
      auto [ window, wm_delete ] = detail::x11_create_window(backend->ewmh, backend->screen, title, bounds);
      m_impl->window_handle = window;
      m_impl->wm_delete_atom = wm_delete;
      // X11 doesn't guarantee that requested bounds == actual bounds
      m_impl->bounds = detail::x11_get_current_geometry(backend->connection, m_impl->window_handle);
    }
    // Create Vulkan surface
    {
      auto& win = m_impl->parent_window_system_backend;
      auto& rnd = m_impl->parent_render_system_backend;
      m_impl->surface = detail::vk_create_surface(rnd->instance, win->connection, m_impl->window_handle, *rnd->dl);
    }
    // Create Vulkan swapchain
    {
      auto& backend = m_impl->parent_render_system_backend;
      detail::vk_configure_swapchain(backend->physical_device, m_impl->surface, m_impl->bounds, 3,
                                     m_impl->swapchain_config, *backend->dl);
      m_impl->swapchain = detail::vk_create_swapchain(backend->physical_device, backend->device, m_impl->surface,
                                                      VK_NULL_HANDLE, backend->work_queue_family,
                                                      m_impl->swapchain_config, *backend->dl);
      auto [ swapchain_images, swapchain_image_views ] =
        detail::vk_create_swapchain_images(backend->device, m_impl->swapchain, m_impl->swapchain_config, *backend->dl);
      m_impl->swapchain_images = swapchain_images;
      m_impl->swapchain_image_views = swapchain_image_views;
    }
    OBERON_POSTCONDITION(m_impl->wm_delete_atom != XCB_NONE);
    OBERON_POSTCONDITION(m_impl->window_handle != XCB_NONE);
    OBERON_POSTCONDITION(m_impl->parent_window_system_backend != nullptr);
    OBERON_POSTCONDITION(m_impl->parent_render_system_backend != nullptr);
  }

  render_window::~render_window() noexcept {
    OBERON_PRECONDITION(m_impl->parent_window_system_backend != nullptr);
    OBERON_PRECONDITION(m_impl->parent_render_system_backend != nullptr);
    OBERON_PRECONDITION(m_impl->wm_delete_atom != XCB_NONE);
    OBERON_PRECONDITION(m_impl->window_handle != XCB_NONE);
    OBERON_PRECONDITION(m_impl->surface != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_impl->swapchain != VK_NULL_HANDLE);
    hide();
    detail::vk_destroy_swapchain_images(m_impl->parent_render_system_backend->device, m_impl->swapchain_images,
                                        m_impl->swapchain_image_views, *m_impl->parent_render_system_backend->dl);
    detail::vk_destroy_swapchain(m_impl->parent_render_system_backend->device, m_impl->swapchain,
                                 *m_impl->parent_render_system_backend->dl);
    detail::vk_destroy_surface(m_impl->parent_render_system_backend->instance, m_impl->surface,
                               *m_impl->parent_render_system_backend->dl);
    detail::x11_destroy_window(m_impl->parent_window_system_backend->connection, m_impl->window_handle);
  }

  void render_window::show() {
    xcb_map_window(m_impl->parent_window_system_backend->connection, m_impl->window_handle);
    xcb_flush(m_impl->parent_window_system_backend->connection);
  }

  void render_window::hide() {
    xcb_unmap_window(m_impl->parent_window_system_backend->connection, m_impl->window_handle);
    xcb_flush(m_impl->parent_window_system_backend->connection);
  }

}
