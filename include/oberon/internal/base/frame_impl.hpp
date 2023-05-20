#ifndef OBERON_INTERNAL_BASE_FRAME_IMPL_HPP
#define OBERON_INTERNAL_BASE_FRAME_IMPL_HPP

#include <array>

#include "../../glm.hpp"

#include "vulkan.hpp"
#include "pipelines.hpp"

namespace oberon {

  class camera;
  class mesh;

}

namespace oberon::internal::base {

  class graphics_device_impl;

  class frame_impl final {
  private:
    enum {
      FENCE_COUNT = 1,
      SUBMISSION_COMPLETE_FENCE_INDEX = 0,
      COMMAND_BUFFER_COUNT = 2,
      RENDER_COMMAND_BUFFER_INDEX = 0,
      COPY_BLIT_COMMAND_BUFFER_INDEX = 1,
      SEMAPHORE_COUNT = 3,
      TARGET_ACQUIRED_SEMAPHORE_INDEX = 0,
      RENDER_FINISHED_SEMAPHORE_INDEX = 1,
      COPY_BLIT_FINISHED_SEMAPHORE_INDEX = 2
    };

    ptr<graphics_device_impl> m_parent{ };
    ptr<std::array<VkPipelineLayout, PIPELINE_COUNT>> m_layouts{ };
    ptr<std::array<VkPipeline, PIPELINE_COUNT>> m_pipelines{ };
    std::array<VkFence, FENCE_COUNT> m_fences{ };
    std::array<VkCommandBuffer, COMMAND_BUFFER_COUNT> m_command_buffers{ };
    std::array<VkSemaphore, SEMAPHORE_COUNT> m_semaphores{ };
    VkCommandPool m_command_pool{ };
    VkImage m_color_attachment{ };
    VkImage m_resolve_attachment{ };
    VkImage m_depth_stencil_attachment{ };
    VmaAllocation m_color_allocation{ };
    VmaAllocation m_resolve_allocation{ };
    VmaAllocation m_depth_stencil_allocation{ };
    VkImageView m_color_view{ };
    VkImageView m_resolve_view{ };
    VkImageView m_depth_stencil_view{ };
    VkRenderingAttachmentInfo m_color_attachment_info{ };
    VkRenderingAttachmentInfo m_depth_stencil_attachment_info{ };
    VkRenderingInfo m_current_render{ };
    VkFormat m_current_color_format{ };
    VkFormat m_current_depth_stencil_format{ };
    VkExtent3D m_current_extent{ };

    void create_images(const VkFormat color_format, const VkFormat depth_stencil_format, const VkExtent3D& extent,
                       const VkSampleCountFlagBits samples);
    void destroy_images();
  public:
    frame_impl(graphics_device_impl& device, const VkFormat color_format, const VkFormat depth_stencil_format,
               const VkExtent3D& extent, const VkSampleCountFlagBits samples,
               std::array<VkPipelineLayout, PIPELINE_COUNT>& layouts,
               std::array<VkPipeline, PIPELINE_COUNT>& pipelines);
    frame_impl(const frame_impl& other) = delete;
    frame_impl(frame_impl&& other) = delete;

    ~frame_impl() noexcept;

    frame_impl& operator=(const frame_impl& rhs) = delete;
    frame_impl& operator=(frame_impl&& rhs) = delete;

    void wait_for_availability();
    void make_unavailable();
    void begin_rendering();
    void end_rendering(const VkImage target, const VkFormat format, const VkExtent3D& extent,
                       const VkImageLayout layout, const VkSemaphore acquired);
    VkSemaphore copy_blit_finished_semaphore() const;
    void draw_test_image();
    void draw(camera& c, mesh& m);
  };

}

#endif
