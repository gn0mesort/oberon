#ifndef OBERON_DETAIL_RENDERER_3D_IMPL_HPP
#define OBERON_DETAIL_RENDERER_3D_IMPL_HPP

#include <vector>
#include <unordered_map>

#include "../renderer_3d.hpp"
#include "../types.hpp"

#include "object_impl.hpp"
#include "vulkan.hpp"
#include "builtin_shaders.hpp"

namespace oberon {
namespace detail {

  constexpr usize MAX_FRAMES_IN_FLIGHT{ 2 }; // should be power of 2.

  struct context_impl;
  struct window_impl;

  struct graphics_pipeline_config final {
    VkPipelineLayoutCreateInfo pipeline_layout_info{ };
    VkGraphicsPipelineCreateInfo graphics_pipeline_info{ };
    std::vector<VkPipelineShaderStageCreateInfo> pipeline_stages{ };
    std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions{ };
    std::vector<VkVertexInputBindingDescription> vertex_binding_descriptions{ };
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info{ };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info{ };
    VkPipelineTessellationStateCreateInfo tessellation_state_info{ };
    std::vector<VkViewport> viewports{ };
    std::vector<VkRect2D> scissors{ };
    VkPipelineViewportStateCreateInfo viewport_state_info{ };
    VkPipelineRasterizationStateCreateInfo rasterization_state_info{ };
    VkPipelineMultisampleStateCreateInfo multisample_state_info{ };
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{ };
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments{ };
    VkPipelineColorBlendStateCreateInfo color_blend_state_info{ };
    std::vector<VkDynamicState> dynamic_states{ };
    VkPipelineDynamicStateCreateInfo dynamic_state_info{ };
    std::vector<VkDescriptorSetLayout> descriptor_sets{ };
    std::vector<VkPushConstantRange> push_constant_ranges{ };
  };

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
    VkExtent2D current_swapchain_extent{ };
    std::vector<VkImage> swapchain_images{ };
    std::vector<VkImageView> swapchain_image_views{ };
    //TODO std::vector<VkImage> depth_stencil_images{ };
    //TODO std::vector<VkImageView depth_stencil_image_views{ };
    VkRenderPass main_renderpass{ };
    std::vector<VkFramebuffer> framebuffers{ };
    VkCommandPool graphics_transfer_command_pool{ };
    std::vector<VkCommandBuffer> graphics_transfer_command_buffers{ };
    // Can't initialize these vectors to the correct size inline because of Most Vexing Parse nonsense.
    std::vector<graphics_pipeline_config> graphics_pipeline_configs{ };
    VkPipelineCache pipeline_cache{ };
    std::vector<VkPipeline> graphics_pipelines{ };
    std::vector<VkSemaphore> render_complete_semaphores{ };
    std::vector<VkSemaphore> image_available_semaphores{ };
    std::vector<VkFence> in_flight_fences{ };
    std::vector<VkFence> in_flight_images{ };
    usize frame_index{ };
    u32 acquired_image_index{ -1U };
  };

  iresult retrieve_vulkan_surface_info(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_swapchain(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept;
  //TODO implmentation
  iresult create_vulkan_depth_stencil(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_renderpasses(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_framebuffers(const context_impl& ctx, const window_impl& win, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_command_pools(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_pipeline_cache(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_synchronization_objects(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;

  iresult configure_test_frame_pipeline(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult create_vulkan_graphics_pipelines(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_graphics_pipelines(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult release_graphics_pipeline_configurations(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;

  iresult reset_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult begin_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult begin_main_render_pass(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult end_main_render_pass(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult draw_test_frame(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult acquire_frame(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult submit_frame(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;

  iresult destroy_vulkan_synchronization_objects(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_graphics_pipelines(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_pipeline_cache(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_command_buffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_command_pools(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_framebuffers(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_renderpasses(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  //TODO implmentation
  iresult destroy_vulkan_depth_stencil(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
  iresult destroy_vulkan_swapchain(const context_impl& ctx, renderer_3d_impl& rnd) noexcept;
}
}

#endif
