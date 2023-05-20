#include "oberon/renderer.hpp"

#include "oberon/graphics_device.hpp"
#include "oberon/window.hpp"
#include "oberon/frame.hpp"

#include "oberon/internal/base/renderer_impl.hpp"
#include "oberon/internal/base/window_impl.hpp"
#include "oberon/internal/base/frame_impl.hpp"

namespace oberon {

  renderer::renderer(graphics_device& device, const extent_2d& resolution) :
  m_impl{ new internal::base::renderer_impl{ device.implementation(), resolution } } { }

  renderer::renderer(graphics_device& device, window& win) :
  m_impl{ new internal::base::renderer_impl{ device.implementation(), win.implementation() } } { }

  renderer::implementation_type& renderer::implementation() {
    return *m_impl;
  }

  frame renderer::begin_frame(window& win) {
    return m_impl->next_frame(win.implementation());
  }

  void renderer::end_frame(frame&& fr) {
    auto submission = frame{ std::move(fr) };
    auto& info = submission.information();
    if (info.window)
    {
      const auto image_index = info.window->acquire_next_image(info.target_acquired);
      const auto swap_extent = info.window->swapchain_extent();
      submission.implementation().end_rendering(info.window->swapchain_images()[image_index],
                                                info.window->surface_format(),
                                                { swap_extent.width, swap_extent.height, 1 },
                                                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, info.target_acquired);
      info.window->present_image(image_index, submission.implementation().copy_blit_finished_semaphore());
    }
  }

}
