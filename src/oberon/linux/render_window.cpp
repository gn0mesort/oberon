#include "oberon/detail/linux/render_window_impl.hpp"

#include <cstdlib>
#include <cstdio>

#include <array>
#include <algorithm>

namespace oberon::detail {

  void render_window_impl_dtor::operator()(const ptr<render_window_impl> p) const noexcept {
    delete p;
  }

  void render_window_impl::create_x_window(const bounding_rect& bounds) {
    auto id = xcb_generate_id(m_ctx.x_connection());
    auto depth = XCB_COPY_FROM_PARENT;
    auto parent = m_ctx.x_screen()->root;
    auto border = u16{ 0 };
    auto visual = XCB_COPY_FROM_PARENT;
    auto window_class = XCB_WINDOW_CLASS_INPUT_OUTPUT;
    auto value_mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    auto event_mask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
                      XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS |
                      XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
                      XCB_EVENT_MASK_PROPERTY_CHANGE;
    auto value_array = std::array<u32, 3>{ m_ctx.x_screen()->black_pixel, false, static_cast<u32>(event_mask) };
    xcb_create_window(m_ctx.x_connection(), depth, id, parent, bounds.position.x, bounds.position.y, bounds.size.width,
                      bounds.size.height, border, window_class, visual, value_mask, std::data(value_array));
    m_x_window = id;
    auto protocols_cookie = xcb_intern_atom(m_ctx.x_connection(), false, 12, "WM_PROTOCOLS");
    auto delete_cookie = xcb_intern_atom(m_ctx.x_connection(), false, 16, "WM_DELETE_WINDOW");
    auto protocols_reply = xcb_intern_atom_reply(m_ctx.x_connection(), protocols_cookie, nullptr);
    m_x_protocols_atom = protocols_reply->atom;
    std::free(protocols_reply);
    auto delete_reply = xcb_intern_atom_reply(m_ctx.x_connection(), delete_cookie, nullptr);
    m_x_delete_atom = delete_reply->atom;
    std::free(delete_reply);
    xcb_change_property(m_ctx.x_connection(), XCB_PROP_MODE_REPLACE, m_x_window, m_x_protocols_atom, XCB_ATOM_ATOM,
                        32, 1, &m_x_delete_atom);
  }

  void render_window_impl::create_vulkan_surface() {
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), CreateXcbSurfaceKHR);
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), GetPhysicalDeviceSurfaceSupportKHR);
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), GetPhysicalDeviceSurfaceCapabilitiesKHR);
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), GetPhysicalDeviceSurfaceFormatsKHR);
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), GetPhysicalDeviceSurfacePresentModesKHR);
    auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
    surface_info.sType = OBERON_VK_STRUCT(XCB_SURFACE_CREATE_INFO_KHR);
    surface_info.connection = m_ctx.x_connection();
    surface_info.window = m_x_window;
    if (auto res = vkCreateXcbSurfaceKHR(m_ctx.vulkan_instance(), &surface_info, nullptr, &m_vulkan_surface);
        res != VK_SUCCESS)
    {
      // TODO throw
    }
    const auto& [ graphics, transfer, present ] = m_ctx.vulkan_queue_families();
    auto support = VkBool32{ };
    vkGetPhysicalDeviceSurfaceSupportKHR(m_ctx.vulkan_physical_device(), present, m_vulkan_surface, &support);
    if (!support)
    {
      // TODO throw
    }
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_ctx.vulkan_physical_device(), m_vulkan_surface,
                                              &m_vulkan_surface_capabilities);
    auto sz = u32{ 0 };
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.vulkan_physical_device(), m_vulkan_surface, &sz, nullptr);
    m_vulkan_surface_formats.resize(sz);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.vulkan_physical_device(), m_vulkan_surface, &sz,
                                         std::data(m_vulkan_surface_formats));
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.vulkan_physical_device(), m_vulkan_surface, &sz, nullptr);
    m_vulkan_present_modes.resize(sz);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.vulkan_physical_device(), m_vulkan_surface, &sz,
                                              std::data(m_vulkan_present_modes));
  }

  void render_window_impl::create_vulkan_swapchain() {
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), CreateSwapchainKHR);
    m_vulkan_swapchain_info.sType = OBERON_VK_STRUCT(SWAPCHAIN_CREATE_INFO_KHR);
    m_vulkan_swapchain_info.surface = m_vulkan_surface;
    m_vulkan_swapchain_info.minImageCount = m_vulkan_surface_capabilities.minImageCount + 1;
    if (m_vulkan_surface_capabilities.maxImageCount &&
        m_vulkan_swapchain_info.minImageCount > m_vulkan_surface_capabilities.maxImageCount)
    {
      m_vulkan_swapchain_info.minImageCount = m_vulkan_surface_capabilities.maxImageCount;
    }
    // This is supported by default. Users can change it later by querying the set of provided modes and selecting
    // one manually.
    m_vulkan_swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
      auto desired_found = false;
      for (const auto& surface_format : m_vulkan_surface_formats) // On Nvidia runs 2 times max
      {
        if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
          m_vulkan_swapchain_info.imageFormat = surface_format.format;
          m_vulkan_swapchain_info.imageColorSpace = surface_format.colorSpace;
          desired_found = true;
        }
      }
      // This is probably not a good way to resolve this issue.
      if (!desired_found)
      {
        m_vulkan_swapchain_info.imageFormat = m_vulkan_surface_formats[0].format;
        m_vulkan_swapchain_info.imageColorSpace = m_vulkan_surface_formats[0].colorSpace;
      }
    }
    if (m_vulkan_surface_capabilities.currentExtent.width != std::numeric_limits<u32>::max())
    {
      m_vulkan_swapchain_info.imageExtent = m_vulkan_surface_capabilities.currentExtent;
    }
    else
    {
      auto extent = VkExtent2D{ m_bounds.size.width, m_bounds.size.height };
      extent.width = std::clamp(extent.width, m_vulkan_surface_capabilities.minImageExtent.width,
                                m_vulkan_surface_capabilities.maxImageExtent.width);
      extent.height = std::clamp(extent.height, m_vulkan_surface_capabilities.minImageExtent.height,
                                 m_vulkan_surface_capabilities.maxImageExtent.height);
      m_vulkan_swapchain_info.imageExtent = extent;
    }
    m_vulkan_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    m_vulkan_swapchain_info.imageArrayLayers = 1;
    auto unique_queue_families = std::unordered_set<u32>{ std::begin(m_ctx.vulkan_queue_families()),
                                                          std::end(m_ctx.vulkan_queue_families()) };
    auto queue_families = std::vector<u32>{ };
    if (std::size(unique_queue_families) == 1)
    {
      m_vulkan_swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
      for (const auto& queue_family : unique_queue_families)
      {
        queue_families.emplace_back(queue_family);
      }
      m_vulkan_swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      m_vulkan_swapchain_info.pQueueFamilyIndices = std::data(queue_families);
      m_vulkan_swapchain_info.queueFamilyIndexCount = std::size(queue_families);
    }
    m_vulkan_swapchain_info.preTransform = m_vulkan_surface_capabilities.currentTransform;
    m_vulkan_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    m_vulkan_swapchain_info.clipped = true;
    if (auto res = vkCreateSwapchainKHR(m_ctx.vulkan_device(), &m_vulkan_swapchain_info, nullptr, &m_vulkan_swapchain);
        res != VK_SUCCESS)
    {
      // TODO throw
    }
  }

  void render_window_impl::destroy_vulkan_swapchain() noexcept {
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), DestroyImageView);
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), DestroySwapchainKHR);
    for (auto& image_view : m_vulkan_swapchain_image_views)
    {
      vkDestroyImageView(m_ctx.vulkan_device(), image_view, nullptr);
    }
    m_vulkan_swapchain_image_views.resize(0);
    m_vulkan_swapchain_images.resize(0);
    vkDestroySwapchainKHR(m_ctx.vulkan_device(), m_vulkan_swapchain, nullptr);
    m_vulkan_swapchain = VK_NULL_HANDLE;
  }

  void render_window_impl::destroy_vulkan_surface() noexcept {
    OBERON_DECLARE_VK_PFN(m_ctx.vulkan_dl(), DestroySurfaceKHR);
    vkDestroySurfaceKHR(m_ctx.vulkan_instance(), m_vulkan_surface, nullptr);
    m_vulkan_surface = VK_NULL_HANDLE;
  }

  void render_window_impl::destroy_x_window() noexcept {
    xcb_destroy_window(m_ctx.x_connection(), m_x_window);
    m_x_window = 0;
    m_x_delete_atom = XCB_ATOM_NONE;
    m_x_protocols_atom = XCB_ATOM_NONE;
  }

  render_window_impl::render_window_impl(context& ctx, const bounding_rect& bounds) :
  m_ctx{ ctx }, m_bounds{ bounds } {
    create_x_window(bounds);
    create_vulkan_surface();
    create_vulkan_swapchain();
  }

  render_window_impl::~render_window_impl() noexcept {
    destroy_vulkan_swapchain();
    destroy_vulkan_surface();
    destroy_x_window();
  }

  xcb_window_t render_window_impl::window_id() {
    return m_x_window;
  }

  void render_window_impl::map_window() {
    xcb_map_window(m_ctx.x_connection(), m_x_window);
    xcb_flush(m_ctx.x_connection());
  }

  void render_window_impl::unmap_window() {
    xcb_unmap_window(m_ctx.x_connection(), m_x_window);
    xcb_flush(m_ctx.x_connection());
  }

}

namespace oberon {

  render_window::render_window(context& ctx, const bounding_rect& bounds) :
  m_impl{ new detail::render_window_impl{ ctx, bounds } } { }

  render_window::~render_window() noexcept {
    hide();
  }

  void render_window::show() {
    m_impl->map_window();
  }

  void render_window::hide() {
    m_impl->unmap_window();
  }
}
