#include "oberon/internal/linux/x11/render_window_impl.hpp"

#include <cstring>

#include <algorithm>

#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <nng/protocol/pubsub0/sub.h>

#include "oberon/debug.hpp"
#include "oberon/errors.hpp"
#include "oberon/graphics_device.hpp"

#include "oberon/internal/linux/x11/xcb.hpp"
#include "oberon/internal/linux/x11/atoms.hpp"
#include "oberon/internal/linux/x11/keys.hpp"
#include "oberon/internal/linux/x11/wsi_context.hpp"
#include "oberon/internal/linux/x11/wsi_worker.hpp"
#include "oberon/internal/linux/x11/graphics_device_impl.hpp"

#define XCB_SEND_REQUEST_SYNC(reply, request, connection, ...) \
  OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST_SYNC(reply, request, connection __VA_OPT__(, __VA_ARGS__))

#define XCB_SEND_REQUEST(request, connection, ...) \
  OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST(request, connection __VA_OPT__(, __VA_ARGS__))

#define XCB_AWAIT_REPLY(request, connection, cookie, error) \
  OBERON_INTERNAL_LINUX_X11_XCB_AWAIT_REPLY(request, connection, cookie, error)

#define XCB_HANDLE_ERROR(reply, error, msg) \
  OBERON_INTERNAL_LINUX_X11_XCB_HANDLE_ERROR(reply, error, msg)

#define VK_STRUCT(name) \
  OBERON_INTERNAL_BASE_VK_STRUCT(name)

#define VK_DECLARE_PFN(dl, name) \
  OBERON_INTERNAL_BASE_VK_DECLARE_PFN(dl, name)

#define VK_SUCCEEDS(exp) \
  OBERON_INTERNAL_BASE_VK_SUCCEEDS(exp)

namespace oberon::internal::linux::x11 {

  render_window_impl::render_window_impl(graphics_device& device, const std::string& title, const rect_2d& bounds) :
  m_parent_device{ &device } {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto screen = parent.wsi().default_screen();
    // Create the X11 window.
    m_window = xcb_generate_id(connection);
    constexpr const auto EVENT_MASK = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                                      XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW |
                                      XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE;
    const auto values = std::array<u32, 3>{ screen->black_pixel, false, EVENT_MASK };
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, m_window, screen->root, bounds.offset.x, bounds.offset.y,
                      bounds.extent.width, bounds.extent.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                      values.data());
    // Configure XInput 2 events.
    {
      auto masks = std::array<xcb_input_xi_event_mask_list_t, 2>{ };
      masks[0].head.deviceid = parent.wsi().keyboard();
      masks[0].head.mask_len = sizeof(xcb_input_xi_event_mask_t) >> 2;
      masks[0].mask = static_cast<xcb_input_xi_event_mask_t>(XCB_INPUT_XI_EVENT_MASK_KEY_PRESS |
                                                             XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE);
      masks[1].head.deviceid = parent.wsi().pointer();
      masks[1].head.mask_len = sizeof(xcb_input_xi_event_mask_t) >> 2;
      masks[1].mask = static_cast<xcb_input_xi_event_mask_t>(XCB_INPUT_XI_EVENT_MASK_MOTION |
                                                             XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS |
                                                             XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE);
      xcb_input_xi_select_events(connection, m_window, masks.size(), &masks[0].head);
    }
    // Connect to the event worker thread via NNG.
    OBERON_CHECK_ERROR_MSG(!nng_sub0_open(&m_socket), 1, "Failed to open an NNG subscriber socket.");
    {
      auto res = nng_socket_set(m_socket, NNG_OPT_SUB_SUBSCRIBE, &m_window, sizeof(xcb_window_t));
      auto leader = parent.wsi().leader();
      res += nng_socket_set(m_socket, NNG_OPT_SUB_SUBSCRIBE, &leader, sizeof(xcb_window_t));
      OBERON_CHECK_ERROR_MSG(!res, 1, "Failed to subscribe to the desired window events.");
    }
    OBERON_CHECK_ERROR_MSG(!nng_dial(m_socket, WSI_WORKER_ENDPOINT, &m_dialer, 0), 1, "Failed to dial the event "
                           "worker thread.");
    // Configure the window.
    {
      const auto instance = parent.wsi().instance_name();
      const auto application = parent.wsi().application_name();
      // Set WM_CLASS
      // WM_CLASS is an odd value in that both strings are explicitly null terminated (i.e., cstrings). Other string
      // atoms don't necessarily care about the null terminator.
      // Additionally, the first value (i.e., the instance name) must be acquired in accordance with ICCCM. This means
      // that the -name argument is checked first, then RESOURCE_NAME, and finally basename(argv[0]) is used.
      // see https://www.x.org/releases/current/doc/xorg-docs/icccm/icccm.html#WM_CLASS_Property
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_CLASS_ATOM),
                          XCB_ATOM_STRING, 8, instance.size() + 1, instance.c_str());
      xcb_change_property(connection, XCB_PROP_MODE_APPEND, m_window, parent.wsi().atom_by_name(WM_CLASS_ATOM),
                          XCB_ATOM_STRING, 8, application.size() + 1, application.c_str());
    }
    {
      const auto pid = getpid();
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(NET_WM_PID_ATOM),
                          XCB_ATOM_CARDINAL, 32, 1, &pid);
      auto buffer = utsname{ };
      OBERON_CHECK_ERROR_MSG(!uname(&buffer), 1, "Failed to retrieve machine hostname.");
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(WM_CLIENT_MACHINE_ATOM), XCB_ATOM_STRING, 8,
                          std::strlen(buffer.nodename), &buffer.nodename[0]);
    }
    {
      const auto protocols_atom = parent.wsi().atom_by_name(WM_PROTOCOLS_ATOM);
      const auto atoms = std::array<xcb_atom_t, 2>{ parent.wsi().atom_by_name(WM_DELETE_WINDOW_ATOM),
                                                    parent.wsi().atom_by_name(NET_WM_PING_ATOM) };
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, protocols_atom, XCB_ATOM_ATOM, 32, 2,
                          atoms.data());
    }
    change_title(title);
    // Set WM_NORMAL_HINTS
    {
      auto hints = xcb_size_hints_t{ };
      // Set max/min size and inform the WM that size/position is a user choice.
      hints.flags = size_hint_flag_bits::program_max_size_bit | size_hint_flag_bits::program_min_size_bit |
                    size_hint_flag_bits::user_size_bit | size_hint_flag_bits::user_position_bit;
      hints.max_width = bounds.extent.width;
      hints.min_width = bounds.extent.width;
      hints.max_height = bounds.extent.height;
      hints.min_height = bounds.extent.height;
      change_size_hints(hints);
    }
    // Set WM_CLIENT_LEADER
    {
      const auto leader = parent.wsi().leader();
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(WM_CLIENT_LEADER_ATOM), XCB_ATOM_WINDOW, 32, 1, &leader);
    }
    // Set WM_HINTS
    {
      auto hints = xcb_hints_t{ };
      hints.flags = hint_flag_bits::window_group_hint_bit | hint_flag_bits::state_hint_bit;
      hints.initial_state = NORMAL_STATE;
      hints.window_group = parent.wsi().leader();
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_HINTS_ATOM),
                          XCB_ATOM_WM_HINTS, 32, sizeof(xcb_hints_t) >> 2, &hints);
    }
    // Set _NET_WM_BYPASS_COMPOSITOR
    {
      const auto mode = u32{ NO_PREFERENCE_COMPOSITOR };
      xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(NET_WM_BYPASS_COMPOSITOR_ATOM), XCB_ATOM_CARDINAL, 32, 1, &mode);
    }
    // Get pointer map.
    // This doesn't seem to be allowed to change in current servers for "security" reasons.
    // I'm only using this to discover the actual number of pointer buttons.
    // My system has 20 but I know mice with more buttons exist and X11 just says, "pointer buttons are always
    // numbered starting at 1."
    // see: https://www.x.org/releases/current/doc/xproto/x11protocol.html#Pointers
    {
      auto reply = ptr<xcb_get_pointer_mapping_reply_t>{ };
      XCB_SEND_REQUEST_SYNC(reply, xcb_get_pointer_mapping, connection);
      m_mouse_button_states.resize(xcb_get_pointer_mapping_map_length(reply));
      std::free(reply);
    }
    initialize_keyboard();
    const auto& dl = parent.dispatch_loader();
    // Create Vulkan window surface.
    {
      auto surface_info = VkXcbSurfaceCreateInfoKHR{ };
      surface_info.sType = VK_STRUCT(XCB_SURFACE_CREATE_INFO_KHR);
      surface_info.connection = connection;
      surface_info.window = m_window;
      VK_DECLARE_PFN(dl, vkCreateXcbSurfaceKHR);
      VK_SUCCEEDS(vkCreateXcbSurfaceKHR(dl.loaded_instance(), &surface_info, nullptr, &m_surface));
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceSupportKHR);
      auto supported = VkBool32{ };
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceSupportKHR(parent.physical_device().handle(), parent.queue_family(),
                                                       m_surface, &supported));
      OBERON_CHECK_ERROR_MSG(supported, 1, "The selected device queue family (%u) does not support the "
                             "render_window.", parent.queue_family());
    }
    // Select Vulkan surface image format.
    {
      VK_DECLARE_PFN(dl, vkGetPhysicalDeviceSurfaceFormatsKHR);
      auto sz = u32{ };
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(parent.physical_device().handle(), m_surface, &sz, nullptr));
      auto surface_formats = std::vector<VkSurfaceFormatKHR>(sz);
      VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceFormatsKHR(parent.physical_device().handle(), m_surface, &sz,
                                                       surface_formats.data()));
      auto found = false;
      for (auto i = u32{ 0 }; i < surface_formats.size() && !found; ++i)
      {
        // According to gpuinfo.org these are the two most commonly supported SRGB surface formats across all
        // platforms. About 63% of reports support R8G8B8A8_SRGB and about 45% support B8G8R8A8_SRGB.
        const auto is_desired = surface_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                                surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB;
        const auto is_srgb = surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        if (is_desired && is_srgb)
        {
          m_swapchain_surface_format = surface_formats[i];
          found = true;
        }
      }
      // If neither of the desired formats are available then the best option is to pick the first format.
      // Oh well!
      if (!found)
      {
        m_swapchain_surface_format = surface_formats.front();
      }
    }
    // Initialize Vulkan swapchain
    initialize_swapchain(VK_NULL_HANDLE);
  }

  render_window_impl::~render_window_impl() noexcept {
    hide();
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    VK_DECLARE_PFN(dl, vkDestroyImageView);
    for (auto& image_view : m_swapchain_image_views)
    {
      vkDestroyImageView(dl.loaded_device(), image_view, nullptr);
    }
    VK_DECLARE_PFN(dl, vkDestroySwapchainKHR);
    vkDestroySwapchainKHR(dl.loaded_device(), m_swapchain, nullptr);
    VK_DECLARE_PFN(dl, vkDestroySurfaceKHR);
    vkDestroySurfaceKHR(dl.loaded_instance(), m_surface, nullptr);
    deinitialize_keyboard();
    xcb_destroy_window(parent.wsi().connection(), m_window);
    nng_dialer_close(m_dialer);
    nng_close(m_socket);
  }

  void render_window_impl::change_size_hints(const xcb_size_hints_t& hints) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_NORMAL_HINTS_ATOM),
                        XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(xcb_size_hints_t) >> 2, &hints);
  }

  void render_window_impl::initialize_swapchain(const VkSwapchainKHR old) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto& dl = parent.dispatch_loader();
    VK_DECLARE_PFN(dl, vkDestroyImageView);
    for (auto& image_view : m_swapchain_image_views)
    {
      vkDestroyImageView(dl.loaded_device(), image_view, nullptr);
    }
    m_swapchain_image_views.clear();
    m_swapchain_images.clear();
    auto swapchain_info = VkSwapchainCreateInfoKHR{ };
    swapchain_info.sType = VK_STRUCT(SWAPCHAIN_CREATE_INFO_KHR);
    swapchain_info.clipped = true;
    swapchain_info.surface = m_surface;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    auto capabilities = VkSurfaceCapabilitiesKHR{ };
    VK_DECLARE_PFN(parent.dispatch_loader(), vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    VK_SUCCEEDS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(parent.physical_device().handle(), m_surface,
                                                          &capabilities));
    // Per the Vulkan specification this must be true.
    // See section 34.2.4 "XCB Platform"
    OBERON_ASSERT(capabilities.currentExtent.width == capabilities.minImageExtent.width);
    OBERON_ASSERT(capabilities.currentExtent.width == capabilities.maxImageExtent.width);
    OBERON_ASSERT(capabilities.currentExtent.height == capabilities.minImageExtent.height);
    OBERON_ASSERT(capabilities.currentExtent.height == capabilities.maxImageExtent.height);
    // TODO: Handle the odd case of a (0, 0) extent which is technically possible according to the specification.
    swapchain_info.imageExtent = m_swapchain_extent = capabilities.currentExtent;
    swapchain_info.imageFormat = m_swapchain_surface_format.format;
    // FIFO is required to be supported.
    // TODO: Fix this so that present mode selection works again.
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.oldSwapchain = old;
    // This is useful for cellphones or other devices where the screen can be rotated (allegedly).
    swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    // The max image count *can* be 0 to indicate any number of images (greater than the minimum) is supported.
    const auto max_images = capabilities.maxImageCount ? capabilities.maxImageCount : std::numeric_limits<u32>::max();
    // TODO: Enable buffer count selection even though the API will *probably* ignore it.
    swapchain_info.minImageCount = std::clamp(u32{ 3 }, capabilities.minImageCount, max_images);
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.imageColorSpace = m_swapchain_surface_format.colorSpace;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    auto queue_family = parent.queue_family();
    swapchain_info.pQueueFamilyIndices = &queue_family;
    swapchain_info.queueFamilyIndexCount = 1;
    VK_DECLARE_PFN(dl, vkCreateSwapchainKHR);
    VK_SUCCEEDS(vkCreateSwapchainKHR(dl.loaded_device(), &swapchain_info, nullptr, &m_swapchain));
    VK_DECLARE_PFN(dl, vkGetSwapchainImagesKHR);
    auto sz = u32{ };
    VK_SUCCEEDS(vkGetSwapchainImagesKHR(dl.loaded_device(), m_swapchain, &sz, nullptr));
    m_swapchain_images.resize(sz);
    VK_SUCCEEDS(vkGetSwapchainImagesKHR(dl.loaded_device(), m_swapchain, &sz, m_swapchain_images.data()));
    m_swapchain_image_views.resize(sz);
    auto image_view_info = VkImageViewCreateInfo{ };
    image_view_info.sType = VK_STRUCT(IMAGE_VIEW_CREATE_INFO);
    image_view_info.format = m_swapchain_surface_format.format;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    VK_DECLARE_PFN(dl, vkCreateImageView);
    auto cur = m_swapchain_image_views.begin();
    for (const auto& image : m_swapchain_images)
    {
      image_view_info.image = image;
      auto& image_view = *cur;
      VK_SUCCEEDS(vkCreateImageView(dl.loaded_device(), &image_view_info, nullptr, &image_view));
      ++cur;
    }
    if (old)
    {
      VK_DECLARE_PFN(dl, vkDestroySwapchainKHR);
      vkDestroySwapchainKHR(dl.loaded_device(), old, nullptr);
    }
  }

  void render_window_impl::initialize_keyboard() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    m_keyboard_map = xkb_x11_keymap_new_from_device(parent.wsi().keyboard_context(), parent.wsi().connection(),
                                                    parent.wsi().keyboard(), XKB_KEYMAP_COMPILE_NO_FLAGS);
    m_keyboard_state = xkb_x11_state_new_from_device(m_keyboard_map, parent.wsi().connection(),
                                                     parent.wsi().keyboard());
    // Map names to keycodes.
    {
#define OBERON_INTERNAL_LINUX_X11_KEYCODE_MAPPING(name, str) (str),
      const auto key_names = std::array<cstring, MAX_KEY>{ OBERON_INTERNAL_LINUX_X11_KEYCODE_MAP };
#undef OBERON_INTERNAL_LINUX_X11_KEYCODE_MAPPING
      auto cur = m_to_keycode.begin();
      for (const auto name : key_names)
      {
        *(cur++) = xkb_keymap_key_by_name(m_keyboard_map, name);
      }
    }
    // Map xcb_keycode_t to oberon::key
    {
      auto i = 1;
      for (const auto keycode : m_to_keycode)
      {
        m_to_external_key[keycode] = static_cast<oberon::key>(i++);
      }
    }
    // Map modifier names to modifier indices
    {
#define OBERON_INTERNAL_LINUX_X11_MODIFIER_KEY_MAPPING(name, str) (str),
      const auto modifier_names = std::array<cstring, MAX_MODIFIER_KEY>{ OBERON_INTERNAL_LINUX_X11_MODIFIER_KEY_MAP };
#undef OBERON_INTERNAL_LINUX_X11_MODIFIER_KEY_MAPPING
      auto cur = m_to_modifier_index.begin();
      for (const auto name : modifier_names)
      {
        *(cur++) = xkb_keymap_mod_get_index(m_keyboard_map, name);
      }
    }
  }

  void render_window_impl::deinitialize_keyboard() {
    xkb_state_unref(m_keyboard_state);
    m_keyboard_state = nullptr;
    xkb_keymap_unref(m_keyboard_map);
    m_keyboard_state = nullptr;
  }

  u32 render_window_impl::id() const {
    return m_window;
  }

  void render_window_impl::show() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    xcb_map_window(parent.wsi().connection(), m_window);
    xcb_flush(parent.wsi().connection());
  }

  void render_window_impl::hide() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    xcb_unmap_window(parent.wsi().connection(), m_window);
    xcb_flush(parent.wsi().connection());
  }

  bitmask render_window_impl::query_visibility() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto window_attrs_req = XCB_SEND_REQUEST(xcb_get_window_attributes, connection, m_window);
    const auto wm_state = parent.wsi().atom_by_name(WM_STATE_ATOM);
    const auto wm_state_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window, wm_state, wm_state, 0,
                                               sizeof(xcb_wm_state_t) >> 2);
    const auto net_wm_state = parent.wsi().atom_by_name(NET_WM_STATE_ATOM);
    const auto net_wm_state_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window, net_wm_state,
                                                   XCB_ATOM_ATOM, 0, (20 * sizeof(xcb_atom_t)) >> 2);
    auto error = ptr<xcb_generic_error_t>{ };
    auto window_attrs_rep = XCB_AWAIT_REPLY(xcb_get_window_attributes, connection, window_attrs_req, &error);
    XCB_HANDLE_ERROR(window_attrs_rep, error, "Failed to retrieve X11 window attributes.");
    auto result = bitmask{ };
    if (window_attrs_rep->map_state == XCB_MAP_STATE_VIEWABLE)
    {
      result |= query_visibility_flag_bits::mapped_bit;
    }
    std::free(window_attrs_rep);
    auto wm_state_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, wm_state_req, &error);
    XCB_HANDLE_ERROR(wm_state_rep, error, "Failed to retrieve ICCCM window state.");
    if (xcb_get_property_value_length(wm_state_rep) == sizeof(xcb_wm_state_t))
    {
      const auto& wm_state_val = *reinterpret_cast<ptr<xcb_wm_state_t>>(xcb_get_property_value(wm_state_rep));
      result |= query_visibility_flag_bits::iconic_bit & -(wm_state_val.state == ICONIC_STATE);
    }
    std::free(wm_state_rep);
    auto net_wm_state_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, net_wm_state_req, &error);
    XCB_HANDLE_ERROR(net_wm_state_rep, error, "Failed to retrieve EWMH window state.");
    if (xcb_get_property_value_length(net_wm_state_rep) / sizeof(xcb_atom_t) > 0)
    {
      const auto net_wm_state_hidden = parent.wsi().atom_by_name(NET_WM_STATE_HIDDEN_ATOM);
      const auto arr = reinterpret_cast<ptr<xcb_atom_t>>(xcb_get_property_value(net_wm_state_rep));
      for (auto i = 0; i < xcb_get_property_value_length(net_wm_state_rep); ++i)
      {
        result ^= query_visibility_flag_bits::hidden_bit & -(arr[i] == net_wm_state_hidden);
      }
    }
    std::free(net_wm_state_rep);
    return result;
  }

  bool render_window_impl::is_shown() const {
    const auto bits = query_visibility();
    return (bits == query_visibility_flag_bits::mapped_bit) ||
           (bits == (query_visibility_flag_bits::iconic_bit | query_visibility_flag_bits::hidden_bit));
  }

  bool render_window_impl::is_minimized() const {
    const auto bits = query_visibility();
    return bits == (query_visibility_flag_bits::iconic_bit | query_visibility_flag_bits::hidden_bit);
  }

  void render_window_impl::change_ewmh_states(const ewmh_state_action action, const xcb_atom_t first,
                                             const xcb_atom_t second) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    auto generic = xcb_generic_event_t{ };
    auto& client_message = *reinterpret_cast<ptr<xcb_client_message_event_t>>(&generic);
    client_message.response_type = XCB_CLIENT_MESSAGE;
    client_message.window = m_window;
    client_message.type = parent.wsi().atom_by_name(NET_WM_STATE_ATOM);
    client_message.format = 32;
    client_message.data.data32[0] = action;
    client_message.data.data32[1] = first;
    client_message.data.data32[2] = second;
    client_message.data.data32[3] = APPLICATION_SOURCE;
    client_message.data.data32[4] = 0;
    send_client_message(parent.wsi().default_screen()->root, generic);
  }

  void render_window_impl::change_compositor_mode(const compositor_mode mode) {
      auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
      xcb_change_property(parent.wsi().connection(), XCB_PROP_MODE_REPLACE, m_window,
                          parent.wsi().atom_by_name(NET_WM_BYPASS_COMPOSITOR_ATOM), XCB_ATOM_CARDINAL, 32, 1, &mode);
  }

  void render_window_impl::change_display_style(const display_style style) {
    if (style == current_display_style())
    {
      return;
    }
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto above = parent.wsi().atom_by_name(NET_WM_STATE_ABOVE_ATOM);
    const auto fullscreen = parent.wsi().atom_by_name(NET_WM_STATE_FULLSCREEN_ATOM);
    switch (style)
    {
    case display_style::windowed:
      {
        change_ewmh_states(REMOVE_WM_STATE_ACTION, above, fullscreen);
        change_compositor_mode(NO_PREFERENCE_COMPOSITOR);
      }
      break;
    case display_style::fullscreen_composited:
      {
        change_ewmh_states(ADD_WM_STATE_ACTION, fullscreen, XCB_NONE);
        change_compositor_mode(NO_PREFERENCE_COMPOSITOR);
      }
      break;
    case display_style::fullscreen_bypass_compositor:
      {
        change_ewmh_states(ADD_WM_STATE_ACTION, above, fullscreen);
        change_compositor_mode(DISABLE_COMPOSITOR);
      }
      break;
    default:
      break;
    }
    xcb_flush(parent.wsi().connection());
  }

  display_style render_window_impl::current_display_style() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    auto net_wm_state_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window,
                                             parent.wsi().atom_by_name(NET_WM_STATE_ATOM), XCB_ATOM_ATOM, 0,
                                             (20 * sizeof(xcb_atom_t)) >> 2);
    auto net_wm_bypass_req = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window,
                                              parent.wsi().atom_by_name(NET_WM_BYPASS_COMPOSITOR_ATOM),
                                              XCB_ATOM_CARDINAL, 0, 1);
    auto error = ptr<xcb_generic_error_t>{ };
    auto net_wm_state_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, net_wm_state_req, &error);
    XCB_HANDLE_ERROR(net_wm_state_rep, error, "Failed to retrieve EWMH window state.");
    const auto atom_count = xcb_get_property_value_length(net_wm_state_rep) / sizeof(xcb_atom_t);
    const auto atoms = reinterpret_cast<readonly_ptr<xcb_atom_t>>(xcb_get_property_value(net_wm_state_rep));
    constexpr auto ABOVE = bitmask{ 0x1 };
    constexpr auto FULLSCREEN = bitmask{ 0x2 };
    constexpr auto BYPASS = bitmask{ 0x4 };
    auto flags = bitmask{ };
    const auto above_atom = parent.wsi().atom_by_name(NET_WM_STATE_ABOVE_ATOM);
    const auto fullscreen_atom = parent.wsi().atom_by_name(NET_WM_STATE_FULLSCREEN_ATOM);
    for (auto i = u32{ 0 }; i < atom_count; ++i)
    {
      flags |= ABOVE & -(atoms[i] == above_atom);
      flags |= FULLSCREEN & -(atoms[i] == fullscreen_atom);
    }
    std::free(net_wm_state_rep);
    const auto net_wm_bypass_rep = XCB_AWAIT_REPLY(xcb_get_property, connection, net_wm_bypass_req, &error);
    XCB_HANDLE_ERROR(net_wm_state_rep, error, "Failed to retrieve EWMH compositor hint.");
    if (xcb_get_property_value_length(net_wm_bypass_rep) == sizeof(u32))
    {
      const auto value = *reinterpret_cast<readonly_ptr<u32>>(xcb_get_property_value(net_wm_bypass_rep));
      flags |= BYPASS & -(value == DISABLE_COMPOSITOR);
    }
    std::free(net_wm_bypass_rep);
    switch (flags)
    {
    case (ABOVE | FULLSCREEN | BYPASS):
      return display_style::fullscreen_bypass_compositor;
    case (FULLSCREEN):
      return display_style::fullscreen_composited;
    default:
      return display_style::windowed;
    }
  }

  rect_2d render_window_impl::current_drawable_rect() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    auto geometry_rep = ptr<xcb_get_geometry_reply_t>{ };
    XCB_SEND_REQUEST_SYNC(geometry_rep, xcb_get_geometry, connection, m_window);
    auto result = rect_2d{ { geometry_rep->x, geometry_rep->y }, { geometry_rep->width, geometry_rep->height } };
    std::free(geometry_rep);
    const auto root = parent.wsi().default_screen()->root;
    auto translate_rep = ptr<xcb_translate_coordinates_reply_t>{ };
    XCB_SEND_REQUEST_SYNC(translate_rep, xcb_translate_coordinates, connection, m_window, root, result.offset.x,
                          result.offset.y);
    result.offset = { translate_rep->dst_x, translate_rep->dst_y };
    std::free(translate_rep);
    return result;
  }

  rect_2d render_window_impl::current_rect() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    // The size of 1 CARD32 value is 1 X11 word (32 bits). Therefore the length is 4.
    auto request = XCB_SEND_REQUEST(xcb_get_property, connection, false, m_window,
                                    parent.wsi().atom_by_name(NET_FRAME_EXTENTS_ATOM), XCB_ATOM_CARDINAL, 0, 4);
    auto result = current_drawable_rect();
    auto error = ptr<xcb_generic_error_t>{ };
    auto reply = XCB_AWAIT_REPLY(xcb_get_property, connection, request, &error);
    XCB_HANDLE_ERROR(reply, error, "Failed to retrieve EWMH frame extents.");
    // It's possible for this function to be issued and reach this point before _NET_FRAME_EXTENTS is set on the
    // corresponding window. As a result it's important to handle this case. If the desired atom is unset then
    // this returns the drawable region unchanged.
    // If an application wishes to retrieve the correct information just after a window is mapped then it should wait
    // for the window to be mapped and updated by the WM.
    if ((xcb_get_property_value_length(reply) >> 2) == 4)
    {
      const auto values = reinterpret_cast<readonly_ptr<u32>>(xcb_get_property_value(reply));
      result.offset.x -= values[0];
      result.offset.y -= values[2];
      result.extent.width += values[0] + values[1];
      result.extent.height += values[2] + values[3];
    }
    std::free(reply);
    return result;
  }

  void render_window_impl::send_client_message(const xcb_window_t destination, const xcb_generic_event_t& message) {
    OBERON_PRECONDITION((message.response_type & ~response_type_bits::synthetic_bit) == XCB_CLIENT_MESSAGE);
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    constexpr const auto MASK = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    xcb_send_event(parent.wsi().connection(), false, destination, MASK, reinterpret_cast<cstring>(&message));
  }

  void render_window_impl::change_title(const std::string& title) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    // The encoding of WM_NAME is ambiguous.
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(WM_NAME_ATOM),
                        XCB_ATOM_STRING, 8, title.size(), title.c_str());
    // _NET_WM_NAME should be preferred and is unambiguously UTF-8 encoded.
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, m_window, parent.wsi().atom_by_name(NET_WM_NAME_ATOM),
                        parent.wsi().atom_by_name(UTF8_STRING_ATOM), 8, title.size(), title.c_str());
    xcb_flush(connection);
  }

  std::string render_window_impl::title() const {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto utf = parent.wsi().atom_by_name(UTF8_STRING_ATOM);
    auto reply = ptr<xcb_get_property_reply_t>{ };
    XCB_SEND_REQUEST_SYNC(reply, xcb_get_property, connection, false, m_window,
                          parent.wsi().atom_by_name(NET_WM_NAME_ATOM), utf, 0, 1024 << 2);
    if (reply->format != 8 || reply->type != utf || reply->bytes_after > 0)
    {
      std::free(reply);
      OBERON_CHECK_ERROR_MSG(false, 1, "Failed to retrieve the window name.");
    }
    auto result = std::string{ reinterpret_cast<cstring>(xcb_get_property_value(reply)) };
    std::free(reply);
    return result;
  }

  void render_window_impl::resize(const extent_2d& extent) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    auto hints = xcb_size_hints_t{ };
    hints.flags = size_hint_flag_bits::program_min_size_bit | size_hint_flag_bits::program_max_size_bit |
                  size_hint_flag_bits::user_position_bit | size_hint_flag_bits::user_size_bit;
    hints.min_width = 0;
    hints.max_width = std::numeric_limits<u16>::max();
    hints.min_height = 0;
    hints.max_height = std::numeric_limits<u16>::max();
    change_size_hints(hints);
    const auto values = std::array<u32, 2>{ extent.width, extent.height };
    xcb_configure_window(connection, m_window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values.data());
    hints.min_width = extent.width;
    hints.max_width = extent.width;
    hints.min_height = extent.height;
    hints.max_height = extent.height;
    change_size_hints(hints);
    xcb_flush(connection);
  }

  void render_window_impl::move_to(const offset_2d& offset) {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    const auto connection = parent.wsi().connection();
    const auto values = std::array<i32, 2>{ offset.x, offset.y };
    xcb_configure_window(connection, m_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values.data());
    xcb_flush(connection);
  }

  event render_window_impl::poll_events() {
    auto& parent = reinterpret_cast<graphics_device_impl&>(m_parent_device->implementation());
    auto protocol_message = ptr<nng_msg>{ };
    auto status = nng_recvmsg(m_socket, &protocol_message, NNG_FLAG_NONBLOCK);
    if (status == NNG_EAGAIN)
    {
      return event{ event_type::none, { } };
    }
    // This is presumptive. The worker must always submit wsi_event_messages.
    auto& message = *reinterpret_cast<ptr<wsi_event_message>>(nng_msg_body(protocol_message));
    auto result = message.data;
    switch (result.type)
    {
    case oberon::event_type::platform:
      {
        const auto generic = reinterpret_cast<ptr<xcb_generic_event_t>>(&result.data.platform.pad[0]);
        const auto type = generic->response_type & ~response_type_bits::synthetic_bit;
        if (parent.wsi().is_keyboard_event(type))
        {
          const auto xkb = reinterpret_cast<ptr<xcb_xkb_generic_event_t>>(generic);
          switch (xkb->xkb_code)
          {
          case XCB_XKB_STATE_NOTIFY:
            {
              const auto state_notify = reinterpret_cast<ptr<xcb_xkb_state_notify_event_t>>(xkb);
              const auto res = xkb_state_update_mask(m_keyboard_state, state_notify->baseMods,
                                                     state_notify->latchedMods, state_notify->lockedMods,
                                                     state_notify->baseGroup, state_notify->latchedGroup,
                                                     state_notify->lockedGroup);
              OBERON_CHECK_ERROR_MSG(res, 1, "No modifiers were altered as the result of a state notify event.");
            }
            break;
          case XCB_XKB_NEW_KEYBOARD_NOTIFY:
          case XCB_XKB_MAP_NOTIFY:
            deinitialize_keyboard();
            initialize_keyboard();
            break;
          default:
            break;
          }
        }
        switch (type)
        {
        case XCB_CLIENT_MESSAGE:
          {
            const auto client_message = reinterpret_cast<ptr<xcb_client_message_event_t>>(generic);
            if (client_message->type == parent.wsi().atom_by_name(WM_PROTOCOLS_ATOM) && client_message->format == 32 &&
                client_message->data.data32[0] == parent.wsi().atom_by_name(NET_WM_PING_ATOM))
            {
              auto response = xcb_generic_event_t{ };
              std::memcpy(&response, generic, sizeof(xcb_generic_event_t));
              auto& message = *reinterpret_cast<ptr<xcb_client_message_event_t>>(&response);
              const auto root = parent.wsi().default_screen()->root;
              message.window = root;
              send_client_message(root, response);
            }
          }
          break;
        }
      }
      break;
    case oberon::event_type::key_press:
      {
        const auto index = static_cast<usize>(translate_keycode(result.data.key_press.key)) - 1;
        m_key_states[index].pressed = true;
        m_key_states[index].echoing = result.data.key_press.echoing;
      }
      break;
    case oberon::event_type::key_release:
      {
        const auto index = static_cast<usize>(translate_keycode(result.data.key_press.key)) - 1;
        m_key_states[index].pressed = false;
        m_key_states[index].echoing = false;
      }
      break;
    case oberon::event_type::button_press:
      {
        const auto index = static_cast<usize>(translate_mouse_buttoncode(result.data.button_press.button)) - 1;
        m_mouse_button_states[index].pressed = true;
      }
      break;
    case oberon::event_type::button_release:
      {
        const auto index = static_cast<usize>(translate_mouse_buttoncode(result.data.button_press.button)) - 1;
        m_mouse_button_states[index].pressed = false;
      }
    default:
      break;
    }
    // Receiving the message is taking ownership of it so we need to release the message here.
    nng_msg_free(protocol_message);
    return result;
  }

  oberon::key render_window_impl::translate_keycode(const u32 code) const {
    return code >= m_to_external_key.size() ? oberon::key::none : m_to_external_key[code];
  }

  oberon::mouse_button render_window_impl::translate_mouse_buttoncode(const u32 code) const {
    return static_cast<oberon::mouse_button>(code);
  }

  bool render_window_impl::is_modifier_pressed(const oberon::modifier_key modifier) const {
    if (modifier == oberon::modifier_key::none)
    {
      return false;
    }
    const auto index = m_to_modifier_index[static_cast<usize>(modifier) - 1];
    return xkb_state_mod_index_is_active(m_keyboard_state, index, XKB_STATE_MODS_EFFECTIVE);
  }

  bool render_window_impl::is_key_pressed(const oberon::key k) const {
    if (k == oberon::key::none)
    {
      return false;
    }
    return m_key_states[static_cast<usize>(k) - 1].pressed;
  }

  bool render_window_impl::is_key_echoing(const oberon::key k) const {
    if (k == oberon::key::none)
    {
      return false;
    }
    return m_key_states[static_cast<usize>(k) - 1].echoing;
  }

  bool render_window_impl::is_mouse_button_pressed(const oberon::mouse_button mb) const {
    if (mb == oberon::mouse_button::none)
    {
      return false;
    }
    return m_mouse_button_states[static_cast<usize>(mb) - 1].pressed;
  }

}
