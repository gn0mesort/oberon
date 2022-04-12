#ifndef OBERON_LINUX_DETAIL_RENDER_WINDOW_IMPL_HPP
#define OBERON_LINUX_DETAIL_RENDER_WINDOW_IMPL_HPP

#include <tuple>
#include <vector>


#include "../render_window.hpp"

#include "x11.hpp"
#include "vulkan.hpp"

namespace oberon::linux::detail {

  OBERON_NON_OWNING_PIMPL_FWD(window_system);
  OBERON_NON_OWNING_PIMPL_FWD(render_system);

  struct vk_swapchain_configuration final {
    std::vector<VkSurfaceFormatKHR> available_surface_formats{ };
    std::vector<VkPresentModeKHR> available_present_modes{ };
    VkPresentModeKHR current_present_mode{ VK_PRESENT_MODE_FIFO_KHR };
    VkSurfaceFormatKHR current_surface_format{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    u32 image_count{ };
    VkSurfaceCapabilitiesKHR capabilities{ };
    VkExtent2D extent{ };
  };

  struct render_window_impl final {
    ptr<window_system_impl> parent_window_system_backend{ };
    ptr<render_system_impl> parent_render_system_backend{ };
    bounding_rect bounds{ };
    xcb_window_t window_handle{ };
    xcb_atom_t wm_delete_atom{ };
    VkSurfaceKHR surface{ };
    vk_swapchain_configuration swapchain_config{ };
    VkSwapchainKHR swapchain{ };
    std::vector<VkImage> swapchain_images{ };
    std::vector<VkImageView> swapchain_image_views{ };
  };

  std::pair<xcb_window_t, xcb_atom_t> x11_create_window(const xcb_ewmh_connection_t& connection,
                                                        const ptr<xcb_screen_t> screen,
                                                        const std::string_view title,
                                                        const bounding_rect& bounds);
  bounding_rect x11_get_current_geometry(const ptr<xcb_connection_t> connection, const xcb_window_t window);
  VkSurfaceKHR vk_create_surface(const VkInstance instance, const ptr<xcb_connection_t> connection,
                                 const xcb_window_t window, const vkfl::loader& dl);
  void vk_configure_swapchain(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface,
                              const bounding_rect& bounds, const u32 image_count, vk_swapchain_configuration& config,
                              const vkfl::loader& dl);
  VkSwapchainKHR vk_create_swapchain(const VkPhysicalDevice physical_device, const VkDevice device,
                                     const VkSurfaceKHR surface, const VkSwapchainKHR old, const u32 work_queue_family,
                                     const vk_swapchain_configuration& config, const vkfl::loader& dl);
  std::pair<std::vector<VkImage>, std::vector<VkImageView>>
    vk_create_swapchain_images(const VkDevice device, const VkSwapchainKHR swapchain,
                               const vk_swapchain_configuration& config, const vkfl::loader& dl);

  void vk_destroy_swapchain_images(const VkDevice device, const std::vector<VkImage>&,
                                   const std::vector<VkImageView>& image_views, const vkfl::loader& dl) noexcept;
  void vk_destroy_swapchain(const VkDevice device, const VkSwapchainKHR swapchain, const vkfl::loader& dl) noexcept;
  void vk_destroy_surface(const VkInstance instance, const VkSurfaceKHR surface, const vkfl::loader& dl) noexcept;
  void x11_destroy_window(const ptr<xcb_connection_t> connection, const xcb_window_t window) noexcept;

}

#endif
