#ifndef OBERON_DETAIL_LINUX_RENDER_WINDOW_IMPL_HPP
#define OBERON_DETAIL_LINUX_RENDER_WINDOW_IMPL_HPP

#include <vector>

#include "../../render_window.hpp"
#include "../../linux/context.hpp"

namespace oberon::detail {

  class render_window_impl final {
  private:
    context& m_ctx;
    bounding_rect m_bounds{ };
    xcb_window_t m_x_window{ };
    xcb_atom_t m_x_protocols_atom{ };
    xcb_atom_t m_x_delete_atom{ };
    VkSurfaceKHR m_vulkan_surface{ };
    VkSurfaceCapabilitiesKHR m_vulkan_surface_capabilities{ };
    std::vector<VkSurfaceFormatKHR> m_vulkan_surface_formats{ };
    std::vector<VkPresentModeKHR> m_vulkan_present_modes{ };
    VkSwapchainCreateInfoKHR m_vulkan_swapchain_info{ };
    VkSwapchainKHR m_vulkan_swapchain{ };
    std::vector<VkImage> m_vulkan_swapchain_images{ };
    std::vector<VkImageView> m_vulkan_swapchain_image_views{ };

    void create_x_window(const bounding_rect& bounds);
    void create_vulkan_surface();
    void create_vulkan_swapchain();

    void destroy_vulkan_surface() noexcept;
    void destroy_vulkan_swapchain() noexcept;
    void destroy_x_window() noexcept;
  public:
    render_window_impl(context& ctx, const bounding_rect& bounds);

    ~render_window_impl() noexcept;

    xcb_window_t window_id();

    void map_window();
    void unmap_window();
  };

}

#endif
