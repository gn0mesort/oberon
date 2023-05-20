#ifndef OBERON_INTERNAL_BASE_RENDERER_IMPL_HPP
#define OBERON_INTERNAL_BASE_RENDERER_IMPL_HPP

#include <array>

#include "../../types.hpp"
#include "../../rects.hpp"

#include "vulkan.hpp"
#include "pipelines.hpp"

namespace oberon {

  class frame;

}

namespace oberon::internal::base {

  class graphics_device_impl;
  class window_impl;
  class frame_impl;


  struct frame_info {
    VkSemaphore target_acquired{ };
    ptr<window_impl> window{ };
  };

  class renderer_impl final {
  private:
    static constexpr usize FRAME_COUNT{ 2 };

    ptr<graphics_device_impl> m_parent{ };
    usize m_current_frame{ 0 };
    VkFormat m_color_format{ };
    VkFormat m_depth_stencil_format{ };
    VkExtent3D m_extent{ };
    std::array<ptr<frame_impl>, FRAME_COUNT> m_frames{ };
    std::array<frame_info, FRAME_COUNT> m_infos{ };
    std::array<VkSemaphore, FRAME_COUNT> m_target_acquired_semaphores{ };
    std::array<VkPipelineLayout, PIPELINE_COUNT> m_pipeline_layouts{ };
    std::array<VkPipeline, PIPELINE_COUNT> m_pipelines{ };

    void create_test_image_pipeline(const VkPipelineRenderingCreateInfo& rendering_info);
    void create_unlit_pc_pipeline(const VkPipelineRenderingCreateInfo& rendering_info);
  public:
    renderer_impl(graphics_device_impl& device, const extent_2d& resolution);
    renderer_impl(graphics_device_impl& device, window_impl& win);
    renderer_impl(const renderer_impl& other) = delete;
    renderer_impl(renderer_impl&& other) = delete;

    ~renderer_impl() noexcept;

    renderer_impl& operator=(const renderer_impl& rhs) = delete;
    renderer_impl& operator=(renderer_impl&& rhs) = delete;

    frame next_frame(window_impl& window);
  };

}

#endif
