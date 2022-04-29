#include "oberon/linux/window.hpp"

#include <cstdlib>

#include <limits>
#include <algorithm>

#include "oberon/debug.hpp"
#include "oberon/context.hpp"
#include "oberon/io_subsystem.hpp"
#include "oberon/graphics_subsystem.hpp"
#include "oberon/linux/io_subsystem.hpp"
#include "oberon/linux/graphics_subsystem.hpp"

namespace oberon::linux {
  void window::open_x_window(const std::string_view title, const bounding_rect& bounds) {
    OBERON_PRECONDITION(m_parent_io);
    OBERON_PRECONDITION(m_window_id == XCB_NONE);
    OBERON_PRECONDITION(m_wm_delete_window == XCB_NONE);
    auto& x_ewmh = m_parent_io->x_ewmh();
    m_window_id = xcb_generate_id(x_ewmh.connection);
    auto x_screen = m_parent_io->x_screen();
    auto depth = XCB_COPY_FROM_PARENT;
    auto parent = x_screen->root;
    auto border = u16{ 0 };
    auto visual = XCB_COPY_FROM_PARENT;
    auto window_class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
    auto value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    auto event_mask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
                      XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS |
                      XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
                      XCB_EVENT_MASK_PROPERTY_CHANGE;
    auto value_array = std::array<u32, 3>{ x_screen->black_pixel, false, static_cast<u32>(event_mask) };
    xcb_create_window(x_ewmh.connection, depth, m_window_id, parent, bounds.position.x, bounds.position.y,
                      bounds.size.width, bounds.size.height, border, window_class, visual, value_mask,
                      std::data(value_array));
    xcb_icccm_set_wm_name(x_ewmh.connection, m_window_id, x_ewmh.UTF8_STRING, 8, std::size(title), std::data(title));
    auto size_hints = xcb_size_hints_t{ };
    xcb_icccm_size_hints_set_max_size(&size_hints, bounds.size.width, bounds.size.height);
    xcb_icccm_size_hints_set_min_size(&size_hints, bounds.size.width, bounds.size.height);
    xcb_icccm_set_wm_normal_hints(x_ewmh.connection, m_window_id, &size_hints);
    m_wm_delete_window = m_parent_io->x_atom("WM_DELETE_WINDOW");
    xcb_icccm_set_wm_protocols(x_ewmh.connection, m_window_id, x_ewmh.WM_PROTOCOLS, 1, &m_wm_delete_window);
    OBERON_POSTCONDITION(m_window_id != XCB_NONE);
    OBERON_POSTCONDITION(m_wm_delete_window != XCB_NONE);
  }

  void window::discover_x_geometry() const {
    OBERON_PRECONDITION(m_parent_io);
    OBERON_PRECONDITION(m_window_id != XCB_NONE);
    auto req = xcb_get_geometry(m_parent_io->x_connection(), m_window_id);
    auto reply = xcb_get_geometry_reply(m_parent_io->x_connection(), req, nullptr);
    m_window_bounds.position = { reply->x, reply->y };
    m_window_bounds.size = { reply->width, reply->height };
    std::free(reply);
    m_window_bounds_dirty = false;
  }

  void window::open_vk_surface() {
    OBERON_PRECONDITION(m_window_id != XCB_NONE);
    const auto& vkdl = m_parent_gfx->vk_loader();
    OBERON_DECLARE_VK_PFN(vkdl, CreateXcbSurfaceKHR);
    auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
    surface_info.sType = OBERON_VK_STRUCT(XCB_SURFACE_CREATE_INFO_KHR);
    surface_info.connection = m_parent_io->x_connection();
    surface_info.window = m_window_id;
    OBERON_VK_SUCCEEDS(vkCreateXcbSurfaceKHR(vkdl.loaded_instance(), &surface_info, nullptr, &m_surface),
                       vk_create_surface_failed_error{ });
    OBERON_POSTCONDITION(m_surface != VK_NULL_HANDLE);
  }

  void window::discover_vk_surface_features() {
    OBERON_PRECONDITION(m_surface != VK_NULL_HANDLE);
    const auto& vkdl = m_parent_gfx->vk_loader();
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceSurfaceFormatsKHR);
    auto sz = u32{ 0 };
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(m_parent_gfx->vk_physical_device(), m_surface, &sz,
                                                            nullptr),
                       vk_query_surface_failed_error{ });
    m_available_surface_formats.resize(sz);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(m_parent_gfx->vk_physical_device(), m_surface, &sz,
                                                            std::data(m_available_surface_formats)),
                       vk_query_surface_failed_error{ });
    // This probably isn't very intelligent.
    // However, specific formats aren't guaranteed to exist :(
    {
      auto found = false;
      for (const auto& surface_format : m_available_surface_formats)
      {
        found = found || (m_current_surface_format.format == surface_format.format &&
                          m_current_surface_format.colorSpace == surface_format.colorSpace);
      }
      if (!found)
      {
        m_current_surface_format = m_available_surface_formats.front();
      }
    }
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceSurfacePresentModesKHR);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(m_parent_gfx->vk_physical_device(), m_surface, &sz,
                                                                 nullptr),
                       vk_query_surface_failed_error{ });
    m_available_present_modes.resize(sz);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(m_parent_gfx->vk_physical_device(), m_surface, &sz,
                                                                 std::data(m_available_present_modes)),
                       vk_query_surface_failed_error{ });
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_parent_gfx->vk_physical_device(), m_surface,
                                                                 &m_surface_capabilities),
                       vk_query_surface_failed_error{ });
    m_swapchain_image_count = std::clamp(m_swapchain_image_count, m_surface_capabilities.minImageCount,
                                         m_surface_capabilities.maxImageCount);
    // Nonsensical but required AFAIK
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceSurfaceSupportKHR);
    auto supported = VkBool32{ };
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(m_parent_gfx->vk_physical_device(),
                                                            m_parent_gfx->vk_primary_queue_family(), m_surface,
                                                            &supported),
                       vk_surface_unsupported_error{ });
    OBERON_INVARIANT(supported, vk_surface_unsupported_error{ });
  }

  void window::open_vk_swapchain(const VkSwapchainKHR old) {
    OBERON_PRECONDITION(m_window_id != XCB_NONE);
    OBERON_PRECONDITION(m_surface != VK_NULL_HANDLE);
    const auto& vkdl = m_parent_gfx->vk_loader();
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    swapchain_info.sType = OBERON_VK_STRUCT(SWAPCHAIN_CREATE_INFO_KHR);
    swapchain_info.oldSwapchain = old;
    swapchain_info.surface = m_surface;
    swapchain_info.clipped = true;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_info.preTransform = m_surface_capabilities.currentTransform;
    swapchain_info.imageColorSpace = m_current_surface_format.colorSpace;
    swapchain_info.imageFormat = m_current_surface_format.format;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.presentMode = m_current_present_mode;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (m_surface_capabilities.currentExtent.width != std::numeric_limits<u32>::max())
    {
      swapchain_info.imageExtent = m_surface_capabilities.currentExtent;
    }
    else
    {
      swapchain_info.imageExtent = VkExtent2D{ m_window_bounds.size.width, m_window_bounds.size.height };
      swapchain_info.imageExtent.width = std::clamp(swapchain_info.imageExtent.width,
                                                    m_surface_capabilities.minImageExtent.width,
                                                    m_surface_capabilities.maxImageExtent.width);
      swapchain_info.imageExtent.height = std::clamp(swapchain_info.imageExtent.height,
                                                     m_surface_capabilities.minImageExtent.height,
                                                     m_surface_capabilities.maxImageExtent.height);
    }
    swapchain_info.minImageCount = m_swapchain_image_count + 1;
    OBERON_DECLARE_VK_PFN(vkdl, CreateSwapchainKHR);
    OBERON_VK_SUCCEEDS(vkCreateSwapchainKHR(vkdl.loaded_device(), &swapchain_info, nullptr, &m_swapchain),
                       vk_create_swapchain_failed_error{ });
    OBERON_DECLARE_VK_PFN(vkdl, GetSwapchainImagesKHR);
    // Swapchain image count doesn't necessarily equal the requested value.
    auto sz = u32{ 0 };
    OBERON_VK_SUCCEEDS(vkGetSwapchainImagesKHR(vkdl.loaded_device(), m_swapchain, &sz, nullptr),
                       vk_enumerate_swapchain_images_failed_error{ });
    m_swapchain_images.resize(sz);
    m_swapchain_image_views.resize(sz);
    OBERON_VK_SUCCEEDS(vkGetSwapchainImagesKHR(vkdl.loaded_device(), m_swapchain, &sz,
                                               std::data(m_swapchain_images)),
                       vk_enumerate_swapchain_images_failed_error{ });
    {
      OBERON_DECLARE_VK_PFN(vkdl, CreateImageView);
      auto image_view_info = VkImageViewCreateInfo{ };
      image_view_info.sType = OBERON_VK_STRUCT(IMAGE_VIEW_CREATE_INFO);
      image_view_info.format = m_current_surface_format.format;
      image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.subresourceRange.baseArrayLayer = 0;
      image_view_info.subresourceRange.layerCount = 1;
      image_view_info.subresourceRange.baseMipLevel = 0;
      image_view_info.subresourceRange.levelCount = 1;
      image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      auto cur = std::begin(m_swapchain_image_views);
      for (const auto& image : m_swapchain_images)
      {
        image_view_info.image = image;
        OBERON_VK_SUCCEEDS(vkCreateImageView(vkdl.loaded_device(), &image_view_info, nullptr, &(*cur)),
                           vk_create_image_view_failed_error{ });
        ++cur;
      }
    }
    OBERON_POSTCONDITION(m_swapchain != VK_NULL_HANDLE);
    OBERON_POSTCONDITION(std::size(m_swapchain_images) > 0);
    OBERON_POSTCONDITION(std::size(m_swapchain_image_views) > 0);
    OBERON_POSTCONDITION(std::size(m_swapchain_images) == std::size(m_swapchain_image_views));
  }

  void window::close_vk_swapchain() noexcept {
    OBERON_PRECONDITION(m_parent_gfx);
    const auto& vkdl = m_parent_gfx->vk_loader();
    OBERON_DECLARE_VK_PFN(vkdl, DestroyImageView);
    for (auto& image_view : m_swapchain_image_views)
    {
      vkDestroyImageView(vkdl.loaded_device(), image_view, nullptr);
    }
    m_swapchain_image_views.clear();
    if (std::size(m_swapchain_images))
    {
      m_swapchain_images.clear();
    }
    if (m_swapchain != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(vkdl, DestroySwapchainKHR);
      vkDestroySwapchainKHR(vkdl.loaded_device(), m_swapchain, nullptr);
      m_swapchain = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(std::size(m_swapchain_image_views) == 0);
    OBERON_POSTCONDITION(std::size(m_swapchain_images) == 0);
    OBERON_POSTCONDITION(m_swapchain == VK_NULL_HANDLE);
  }

  void window::close_vk_surface() noexcept {
    if (m_surface != VK_NULL_HANDLE)
    {
      const auto& vkdl = m_parent_gfx->vk_loader();
      OBERON_DECLARE_VK_PFN(vkdl, DestroySurfaceKHR);
      vkDestroySurfaceKHR(vkdl.loaded_instance(), m_surface, nullptr);
      m_surface = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(m_surface == VK_NULL_HANDLE);
  }

  void window::close_x_window() noexcept {
    OBERON_PRECONDITION(m_parent_io);
    if (m_window_id != XCB_NONE)
    {
      xcb_destroy_window(m_parent_io->x_connection(), m_window_id);
      m_window_id = XCB_NONE;
      m_wm_delete_window = XCB_NONE;
    }
    OBERON_POSTCONDITION(m_window_id == XCB_NONE);
    OBERON_POSTCONDITION(m_wm_delete_window == XCB_NONE);
  }

  window::window(context& ctx, const std::string_view title, const bounding_rect& bounds) {
    OBERON_PRECONDITION(ctx.get_io_subsystem().implementation() == subsystem_implementation::linux_xcb);
    OBERON_PRECONDITION(ctx.get_graphics_subsystem().implementation() == subsystem_implementation::vulkan);
    m_parent_io = static_cast<io_subsystem*>(&ctx.get_io_subsystem());
    m_parent_gfx = static_cast<graphics_subsystem*>(&ctx.get_graphics_subsystem());
    open_x_window(title, bounds);
    discover_x_geometry();
    open_vk_surface();
    discover_vk_surface_features();
    open_vk_swapchain(VK_NULL_HANDLE);
  }

  window::~window() noexcept {
    close_vk_swapchain();
    close_vk_surface();
    close_x_window();
  }

  void window::show() {
    xcb_map_window(m_parent_io->x_connection(), m_window_id);
    xcb_flush(m_parent_io->x_connection());
  }

  void window::hide() {
    xcb_unmap_window(m_parent_io->x_connection(), m_window_id);
    xcb_flush(m_parent_io->x_connection());
  }

  const bounding_rect& window::bounds() const {
    if (m_window_bounds_dirty)
    {
      discover_x_geometry();
    }
    return m_window_bounds;
  }
}
