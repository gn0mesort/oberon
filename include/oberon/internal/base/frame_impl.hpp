/**
 * @file frame_impl.hpp
 * @brief Internal frame object API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
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

  /**
   * @class frame_impl
   * @brief The base frame implementation for Vulkan.
   */
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

    /**
     * @brief Create a `frame_impl`.
     * @param device The `graphics_device` on which the resulting `frame_impl` is based.
     * @param color_format The color format to use for color images.
     * @param depth_stencil_format The depth/stencil format to use for depth/stencil images.
     * @param extent The 3D extent of the images in the frame.
     * @param samples The number of samples for the multisampled color attachment.
     * @param layouts An array of `VkPipelineLayout`s corresponding to the `VkPipeline`s used by the frame's
     *               `renderer`.
     * @param pipelines An array of `VkPipeline`s used by the frame's `renderer`.
     */
    frame_impl(graphics_device_impl& device, const VkFormat color_format, const VkFormat depth_stencil_format,
               const VkExtent3D& extent, const VkSampleCountFlagBits samples,
               std::array<VkPipelineLayout, PIPELINE_COUNT>& layouts,
               std::array<VkPipeline, PIPELINE_COUNT>& pipelines);
    /// @cond
    frame_impl(const frame_impl& other) = delete;
    frame_impl(frame_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `frame_impl`.
     */
    ~frame_impl() noexcept;

    /// @cond
    frame_impl& operator=(const frame_impl& rhs) = delete;
    frame_impl& operator=(frame_impl&& rhs) = delete;
    /// @endcond

    /**
     * @brief Block until the frame becomes available.
     * @details A frame becomes unavailable when it is submited to a `graphics_device` for processing. When processing
     *          is complete the frame becomes available again.
     */
    void wait_for_availability();

    /**
     * @brief Begin rendering a frame.
     * @details This records all steps required for the frame to begin accepting rendering commands.
     */
    void begin_rendering();

    /**
     * @brief Finish rendering a frame.
     * @details This executes the steps to finalize rendering and copy/blit the resulting image onto the target.
     * @param target The target `VkImage` to render to.
     * @param format The `VkFormat` of the target.
     * @param extent The 3D extent of the target.
     * @param layout The desired final layout of the target.
     * @param target_must_be_acquired This indicates whether or not to use the "target acquired" semaphore.
     *                                For images that belong to a swapchain, it is crucial to wait for this semaphore
     *                                to be signaled by a cooperating `window`.
     */
    void end_rendering(const VkImage target, const VkFormat format, const VkExtent3D& extent,
                       const VkImageLayout layout, const bool target_must_be_acquired);

    /**
     * @brief Retrieve the frame's "target acquired" semaphore.
     * @return A `VkSemaphore` that indicates when a target image has been acquired.
     */
    VkSemaphore target_acquired_semaphore() const;

    /**
     * @brief Retrieve the frame's "copy/blit finished" semaphore.
     * @return A `VkSemaphore` that indicates when the final copy/blit step is finished.
     */
    VkSemaphore copy_blit_finished_semaphore() const;

    /**
     * @brief Draw a test image.
     */
    void draw_test_image();

    /**
     * @brief Draw a `mesh` using the given `camera`.
     * @param c The `camera` to use when drawing.
     * @param m The `mesh` to draw.
     */
    void draw(camera& c, mesh& m);
  };

}

#endif
