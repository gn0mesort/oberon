/**
 * @file renderer.cpp
 * @brief 3D renderer object implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/renderer.hpp"

#include "oberon/graphics_device.hpp"
#include "oberon/window.hpp"
#include "oberon/frame.hpp"

#include "oberon/internal/base/renderer_impl.hpp"
#include "oberon/internal/base/window_impl.hpp"
#include "oberon/internal/base/frame_impl.hpp"

namespace oberon {

  renderer::renderer(graphics_device& device, const extent_2d& resolution, const u32 samples) :
  m_impl{ new internal::base::renderer_impl{ device.implementation(), resolution, samples } } { }

  renderer::renderer(graphics_device& device, window& win, const u32 samples) :
  m_impl{ new internal::base::renderer_impl{ device.implementation(), win.implementation(), samples } } { }

  renderer::implementation_type& renderer::implementation() {
    return *m_impl;
  }

  frame renderer::begin_frame() {
    return m_impl->next_frame();
  }

  void renderer::end_frame(window& win, frame&& fr) {
    // Retire the external frame.
    auto submission = frame{ std::move(fr) };
    auto& impl = submission.implementation();
    auto& win_impl = win.implementation();
    const auto image_index = win_impl.acquire_next_image(impl.target_acquired_semaphore());
    const auto swap_extent = win_impl.swapchain_extent();
    impl.end_rendering(win_impl.swapchain_images()[image_index], win_impl.surface_format(),
                       { swap_extent.width, swap_extent.height, 1 }, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, true);
    win_impl.present_image(image_index, submission.implementation().copy_blit_finished_semaphore());
  }

}
