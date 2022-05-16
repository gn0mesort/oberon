#include "oberon/detail/window_impl.hpp"

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <bit>

#include "oberon/bounds.hpp"
#include "oberon/debug.hpp"
#include "oberon/application.hpp"

#include "oberon/detail/io_subsystem.hpp"
#include "oberon/detail/graphics_subsystem.hpp"

namespace oberon::detail {

  void window_impl_dtor::operator()(const ptr<window_impl> p) const noexcept {
    delete p;
  }

  // Pre: no io, no graphics
  // Post: io, graphics
  void window_impl::open_parent_systems(io_subsystem& io, graphics_subsystem& graphics) {
    OBERON_PRECONDITION(!m_io);
    OBERON_PRECONDITION(!m_graphics);
    m_io = &io;
    m_graphics = &graphics;
    OBERON_POSTCONDITION(m_io);
    OBERON_POSTCONDITION(m_graphics);
  }

  // Post: no io, no graphics
  void window_impl::close_parent_systems() noexcept {
    m_io = nullptr;
    m_graphics = nullptr;
    OBERON_POSTCONDITION(!m_io);
    OBERON_POSTCONDITION(!m_graphics);
  }

  // Pre: no window
  // Post: window
  void window_impl::open_x_window(const std::string_view title, const bounding_rect& bounds) {
    OBERON_PRECONDITION(m_window_id == XCB_NONE);
    const auto conn = m_io->x_connection();
    const auto parent = m_io->x_screen()->root;
    const auto window_id = xcb_generate_id(conn);
    const auto valmask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    const auto eventmask = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS |
                           XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE |
                           XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    const auto vallist = std::array<u32, 3>{ m_io->x_screen()->black_pixel, false, eventmask };
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, window_id, parent, bounds.position.x, bounds.position.y,
                      bounds.size.width, bounds.size.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT,
                      valmask, std::data(vallist));
    m_window_id = window_id;
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_WM_NAME), XCB_ATOM_STRING, 8,
                        std::size(title), std::data(title));
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_NET_WM_NAME),
                        m_io->x_atom(X_ATOM_UTF8_STRING), 8, std::size(title), std::data(title));
    const auto hostname = m_io->hostname();
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_WM_CLIENT_MACHINE),
                        XCB_ATOM_STRING, 8, std::size(hostname), std::data(hostname));
    const auto wm_protocols = m_io->x_atom(X_ATOM_WM_PROTOCOLS);
    const auto wm_delete_window = m_io->x_atom(X_ATOM_WM_DELETE_WINDOW);
    xcb_change_property(conn, XCB_PROP_MODE_APPEND, m_window_id, wm_protocols, XCB_ATOM_ATOM, 32, 1,
                        &wm_delete_window);
    const auto net_wm_ping = m_io->x_atom(X_ATOM_NET_WM_PING);
    xcb_change_property(conn, XCB_PROP_MODE_APPEND, m_window_id, wm_protocols, XCB_ATOM_ATOM, 32, 1, &net_wm_ping);
    const auto pid = m_io->process_id();
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_NET_WM_PID), XCB_ATOM_CARDINAL,
                        32, 1, &pid);
    auto hints = x_size_hints{ };
    hints.flags = x_size_hint_flag_bits::program_min_size_bit | x_size_hint_flag_bits::program_max_size_bit;
    hints.min_width = bounds.size.width;
    hints.min_height = bounds.size.height;
    hints.max_width = bounds.size.width;
    hints.max_height = bounds.size.height;
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, m_window_id, m_io->x_atom(X_ATOM_WM_NORMAL_HINTS),
                        XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(x_size_hints) >> 2, &hints);
    OBERON_POSTCONDITION(m_window_id != XCB_NONE);
  }

  // Post: no window
  void window_impl::close_x_window() noexcept {
    if (m_window_id != XCB_NONE)
    {
      hide();
      xcb_destroy_window(m_io->x_connection(), m_window_id);
      m_window_id = XCB_NONE;
    }
    OBERON_POSTCONDITION(m_window_id == XCB_NONE);
  }


  // Pre: no surface, graphics->instance, instance loaded
  // Post: surface
  void window_impl::open_vk_surface() {
    OBERON_PRECONDITION(m_graphics->vk_instance());
    OBERON_PRECONDITION(m_graphics->vk_loader().loaded_instance() == m_graphics->vk_instance());
    OBERON_PRECONDITION(m_surface == VK_NULL_HANDLE);
    const auto& vkdl = m_graphics->vk_loader();
    auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
    surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_info.window = m_window_id;
    surface_info.connection = m_io->x_connection();
    OBERON_DECLARE_VK_PFN(vkdl, CreateXcbSurfaceKHR);
    OBERON_VK_SUCCEEDS(vkCreateXcbSurfaceKHR(m_graphics->vk_instance(), &surface_info, nullptr, &m_surface));
    const auto& physical_device = m_graphics->vk_physical_device();
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_surface, &m_surface_capabilities));
    OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceSurfacePresentModesKHR);
    auto sz = u32{ 0 };
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &sz, nullptr));
    auto present_modes = std::vector<VkPresentModeKHR>(sz);
    OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &sz,
                                                                 std::data(present_modes)));
    m_surface_present_modes ^= m_surface_present_modes;
    for (const auto present_mode : present_modes)
    {
      switch (present_mode)
      {
      case VK_PRESENT_MODE_IMMEDIATE_KHR:
        m_surface_present_modes |= window_present_mode_bits::immediate_bit;
        break;
      case VK_PRESENT_MODE_MAILBOX_KHR:
        m_surface_present_modes |= window_present_mode_bits::mailbox_bit;
        break;
      case VK_PRESENT_MODE_FIFO_KHR:
        m_surface_present_modes |= window_present_mode_bits::fifo_bit;
        break;
      case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        m_surface_present_modes |= window_present_mode_bits::fifo_relaxed_bit;
        break;
      default:
        break;
      }
    }
    OBERON_POSTCONDITION(m_surface != VK_NULL_HANDLE);
  }

  // Pre: graphics->instance, instance loaded
  // Post: no surface
  void window_impl::close_vk_surface() noexcept {
    OBERON_PRECONDITION(m_graphics->vk_instance() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_graphics->vk_loader().loaded_instance() == m_graphics->vk_instance());
    if (m_surface != VK_NULL_HANDLE)
    {
      const auto& vkdl = m_graphics->vk_loader();
      OBERON_DECLARE_VK_PFN(vkdl, DestroySurfaceKHR);
      vkDestroySurfaceKHR(m_graphics->vk_instance(), m_surface, nullptr);
      m_surface = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(m_surface == VK_NULL_HANDLE);
  }

  // Pre: no swapchain, no swapchain images, no swapchain image views, device, device loaded, surface
  // Post: swapchain, swapchain images, swapchain image views, size(swapchain images) == size(swapchain image views)
  void window_impl::open_vk_swapchain(const u32 buffer_count, const bitmask present_mode) {
    OBERON_PRECONDITION(m_graphics->vk_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_graphics->vk_loader().loaded_device() == m_graphics->vk_device());
    OBERON_PRECONDITION(m_surface != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_swapchain == VK_NULL_HANDLE);
    OBERON_PRECONDITION(std::size(m_swapchain_images) == 0);
    OBERON_PRECONDITION(std::size(m_swapchain_image_views) == 0);
    auto bounds_update = start_bounds_update();
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = m_surface;
    auto primary_queue_family = m_graphics->vk_primary_queue_family();
    swapchain_info.pQueueFamilyIndices = &primary_queue_family;
    swapchain_info.queueFamilyIndexCount = 1;
    swapchain_info.clipped = true;
    if (!m_surface_capabilities.maxImageCount)
    {
      swapchain_info.minImageCount = std::max(buffer_count, m_surface_capabilities.minImageCount);
    }
    else
    {
      swapchain_info.minImageCount = std::clamp(buffer_count, m_surface_capabilities.minImageCount,
                                                m_surface_capabilities.minImageCount);
    }
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.preTransform = m_surface_capabilities.currentTransform;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    OBERON_ASSERT(std::has_single_bit(present_mode));
    switch (present_mode & m_surface_present_modes)
    {
    case window_present_mode_bits::immediate_bit:
      swapchain_info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      break;
    case window_present_mode_bits::mailbox_bit:
      swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    case window_present_mode_bits::fifo_relaxed_bit:
      swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
      break;
    default:
      break;
    }
    const auto& vkdl = m_graphics->vk_loader();
    auto sz = u32{ 0 };
    {
      auto physical_device = m_graphics->vk_physical_device();
      OBERON_DECLARE_VK_PFN(vkdl, GetPhysicalDeviceSurfaceFormatsKHR);
      OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &sz, nullptr));
      auto surface_formats = std::vector<VkSurfaceFormatKHR>(sz);
      OBERON_VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &sz,
                                                              std::data(surface_formats)));
      auto found = false;
      for (const auto& surface_format : surface_formats)
      {
        if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
          swapchain_info.imageColorSpace = surface_format.colorSpace;
          swapchain_info.imageFormat = surface_format.format;
          found = true;
        }
      }
      if (!found)
      {
        swapchain_info.imageColorSpace = surface_formats[0].colorSpace;
        swapchain_info.imageFormat = surface_formats[0].format;
      }
    }
    finish_bounds_update(bounds_update);
    if (m_surface_capabilities.currentExtent.width != std::numeric_limits<u32>::max())
    {
      swapchain_info.imageExtent = m_surface_capabilities.currentExtent;
    }
    else
    {
      swapchain_info.imageExtent = VkExtent2D{ m_bounds.size.width, m_bounds.size.height };
      swapchain_info.imageExtent.width = std::clamp(swapchain_info.imageExtent.width,
                                                    m_surface_capabilities.minImageExtent.width,
                                                    m_surface_capabilities.maxImageExtent.width);
      swapchain_info.imageExtent.height = std::clamp(swapchain_info.imageExtent.height,
                                                     m_surface_capabilities.minImageExtent.height,
                                                     m_surface_capabilities.maxImageExtent.height);
    }
    OBERON_DECLARE_VK_PFN(vkdl, CreateSwapchainKHR);
    auto device = m_graphics->vk_device();
    OBERON_VK_SUCCEEDS(vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &m_swapchain));
    OBERON_DECLARE_VK_PFN(vkdl, GetSwapchainImagesKHR);
    OBERON_VK_SUCCEEDS(vkGetSwapchainImagesKHR(device, m_swapchain, &sz, nullptr));
    m_swapchain_images.resize(sz);
    m_swapchain_image_views.resize(sz);
    OBERON_VK_SUCCEEDS(vkGetSwapchainImagesKHR(device, m_swapchain, &sz, std::data(m_swapchain_images)));
    {
      auto image_view_info = VkImageViewCreateInfo{ };
      image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      image_view_info.format = swapchain_info.imageFormat;
      image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_view_info.subresourceRange.baseArrayLayer = 0;
      image_view_info.subresourceRange.layerCount = 1;
      image_view_info.subresourceRange.baseMipLevel = 0;
      image_view_info.subresourceRange.levelCount = 1;
      auto cur = std::begin(m_swapchain_image_views);
      OBERON_DECLARE_VK_PFN(vkdl, CreateImageView);
      for (const auto& image : m_swapchain_images)
      {
        image_view_info.image = image;
        OBERON_VK_SUCCEEDS(vkCreateImageView(device, &image_view_info, nullptr, &(*cur)));
        ++cur;
      }
    }
    OBERON_POSTCONDITION(m_swapchain != VK_NULL_HANDLE);
    OBERON_POSTCONDITION(std::size(m_swapchain_images) != 0);
    OBERON_POSTCONDITION(std::size(m_swapchain_image_views) == std::size(m_swapchain_images));
  }

  // Pre: device, device loaded, swapchain, swapchain images
  // Post: command pool, command buffers == swapchain images
  void window_impl::open_vk_command_buffers() {
    OBERON_PRECONDITION(m_graphics->vk_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_graphics->vk_loader().loaded_device() == m_graphics->vk_device());
    OBERON_PRECONDITION(m_swapchain != VK_NULL_HANDLE);
    OBERON_PRECONDITION(std::size(m_swapchain_images) != 0);
    const auto& vkdl = m_graphics->vk_loader();
    const auto device = m_graphics->vk_device();
    auto command_pool_info = VkCommandPoolCreateInfo{ };
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = m_graphics->vk_primary_queue_family();
    OBERON_DECLARE_VK_PFN(vkdl, CreateCommandPool);
    OBERON_VK_SUCCEEDS(vkCreateCommandPool(device, &command_pool_info, nullptr, &m_command_pool));
    auto command_buffer_info = VkCommandBufferAllocateInfo{ };
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.commandPool = m_command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    m_command_buffers.resize(std::size(m_swapchain_images));
    command_buffer_info.commandBufferCount = std::size(m_command_buffers);
    OBERON_DECLARE_VK_PFN(vkdl, AllocateCommandBuffers);
    OBERON_VK_SUCCEEDS(vkAllocateCommandBuffers(device, &command_buffer_info, std::data(m_command_buffers)));
    OBERON_POSTCONDITION(m_command_pool != VK_NULL_HANDLE);
    OBERON_POSTCONDITION(std::size(m_command_buffers) == std::size(m_swapchain_images));
  }

  // Pre: device, device loaded
  // Post: no command buffers, no command pool
  void window_impl::close_vk_command_buffers() noexcept {
    OBERON_PRECONDITION(m_graphics->vk_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_graphics->vk_loader().loaded_device() == m_graphics->vk_device());
    if (m_command_pool != VK_NULL_HANDLE)
    {
      const auto& vkdl = m_graphics->vk_loader();
      const auto device = m_graphics->vk_device();
      OBERON_DECLARE_VK_PFN(vkdl, FreeCommandBuffers);
      vkFreeCommandBuffers(device, m_command_pool, std::size(m_command_buffers), std::data(m_command_buffers));
      m_command_buffers.resize(0);
      OBERON_DECLARE_VK_PFN(vkdl, DestroyCommandPool);
      vkDestroyCommandPool(device, m_command_pool, nullptr);
      m_command_pool = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(m_command_pool == VK_NULL_HANDLE);
    OBERON_POSTCONDITION(std::size(m_command_buffers) == 0);
  }

  // Pre: io, window
  xcb_get_geometry_cookie_t window_impl::start_bounds_update() {
    OBERON_PRECONDITION(m_io);
    OBERON_PRECONDITION(m_window_id != XCB_NONE);
    return xcb_get_geometry(m_io->x_connection(), m_window_id);
  }


  // Pre: io, window
  void window_impl::finish_bounds_update(const xcb_get_geometry_cookie_t req) {
    OBERON_PRECONDITION(m_io);
    OBERON_PRECONDITION(m_window_id != XCB_NONE);
    auto rep = ptr<xcb_get_geometry_reply_t>{ };
    OBERON_X_SUCCEEDS(rep, xcb_get_geometry_reply(m_io->x_connection(), req, err));
    m_bounds.position = { rep->x, rep->y };
    m_bounds.size = { rep->width, rep->height };
    std::free(rep);
  }

  // Pre: device, device loaded
  // Post: no swapchain, no swapchain images, no swapchain image views
  void window_impl::close_vk_swapchain() noexcept {
    OBERON_PRECONDITION(m_graphics->vk_device() != VK_NULL_HANDLE);
    OBERON_PRECONDITION(m_graphics->vk_loader().loaded_device() == m_graphics->vk_device());
    const auto& vkdl = m_graphics->vk_loader();
    const auto& device = m_graphics->vk_device();
    OBERON_DECLARE_VK_PFN(vkdl, DestroyImageView);
    for (auto& image_view : m_swapchain_image_views)
    {
      vkDestroyImageView(device, image_view, nullptr);
    }
    m_swapchain_image_views.resize(0);
    m_swapchain_images.resize(0);
    if (m_swapchain != VK_NULL_HANDLE)
    {
      OBERON_DECLARE_VK_PFN(vkdl, DestroySwapchainKHR);
      vkDestroySwapchainKHR(device, m_swapchain, nullptr);
      m_swapchain = VK_NULL_HANDLE;
    }
    OBERON_POSTCONDITION(m_swapchain == VK_NULL_HANDLE);
    OBERON_POSTCONDITION(std::size(m_swapchain_images) == 0);
    OBERON_POSTCONDITION(std::size(m_swapchain_image_views) == 0);
  }

  window_impl::window_impl(context& ctx, const window::config& conf) {
    open_parent_systems(ctx.io(), ctx.graphics());
    open_x_window(conf.title, conf.bounds);
    open_vk_surface();
    open_vk_swapchain(conf.preferred_buffer_count, static_cast<bitmask>(conf.preferred_present_mode));
    open_vk_command_buffers();
  }

  window_impl::~window_impl() noexcept {
    close_vk_command_buffers();
    close_vk_swapchain();
    close_vk_surface();
    close_x_window();
    close_parent_systems();
  }

  bitmask window_impl::get_signals() const {
    return  m_window_signals;
  }

  void window_impl::clear_signals(const bitmask sigs) {
    m_window_signals = m_window_signals & ~sigs;
  }

  bitmask window_impl::get_flags() const {
    return m_window_state;
  }

  void window_impl::show() {
    xcb_map_window(m_io->x_connection(), m_window_id);
    xcb_flush(m_io->x_connection());
    m_window_state = m_window_state | window_flag_bits::shown_bit;
  }

  void window_impl::hide() {
    xcb_unmap_window(m_io->x_connection(), m_window_id);
    xcb_flush(m_io->x_connection());
    m_window_state = m_window_state & ~window_flag_bits::shown_bit;
  }

  void window_impl::accept_message(const ptr<void> message) {
    auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(message);
    OBERON_ASSERT(client_message->window == m_window_id);
    if (client_message->type == m_io->x_atom(X_ATOM_WM_PROTOCOLS))
    {
      if (client_message->data.data32[0] == m_io->x_atom(X_ATOM_NET_WM_PING))
      {
        auto root = m_io->x_screen()->root;
        client_message->window = root;
        auto buffer = xcb_generic_event_t{ };
        std::memcpy(reinterpret_cast<ptr<char>>(&buffer), client_message, sizeof(xcb_client_message_event_t));
        m_io->x_send_event(root, buffer, 0, false);
        m_last_ping = client_message->data.data32[1];
      }
      if (client_message->data.data32[0] == m_io->x_atom(X_ATOM_WM_DELETE_WINDOW))
      {
        m_window_signals = m_window_signals | window_signal_bits::destroy_bit;
        hide();
      }
    }
  }

}
