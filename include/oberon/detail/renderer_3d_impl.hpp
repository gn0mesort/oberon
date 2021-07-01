#ifndef OBERON_DETAIL_RENDERER_3D_IMPL_HPP
#define OBERON_DETAIL_RENDERER_3D_IMPL_HPP

#include <vector>

#include "../renderer_3d.hpp"
#include "../types.hpp"

#include "object_impl.hpp"
#include "vulkan.hpp"

namespace oberon {
namespace detail {

  struct context_impl;
  struct window_impl;

  struct renderer_3d_impl : public object_impl {
    virtual ~renderer_3d_impl() noexcept = default;

    VkSurfaceCapabilitiesKHR surface_capabilities{ };
    std::vector<VkSurfaceFormatKHR> surface_formats{ };
    std::vector<VkPresentModeKHR> presentation_modes{ };
    // FIFO is always available if presentation is available.
    VkPresentModeKHR current_presentation_mode{ VK_PRESENT_MODE_FIFO_KHR };
    // Treating VK_FORMAT_UNDEFINED as meaning "unset".
    VkSurfaceFormatKHR current_surface_format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
    VkSwapchainKHR swapchain{ };
    std::vector<VkImage> swapchain_images{ };
    std::vector<VkImageView> swapchain_image_views{ };
    //TODO std::vector<VkImage> depth_stencil_images{ };
    //TODO std::vector<VkImageView depth_stencil_image_views{ };
    VkRenderPass main_renderpass{ };
    std::vector<VkFramebuffer> framebuffers{ };
  };


  iresult retrieve_vulkan_surface_info(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_swapchain(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept;
  //TODO implmentation
  iresult create_vulkan_depth_stencil(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_renderpasses(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_framebuffers(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept;

  iresult destroy_vulkan_framebuffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_renderpasses(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  //TODO implmentation
  iresult destroy_vulkan_depth_stencil(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_swapchain(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
}
}

#endif
