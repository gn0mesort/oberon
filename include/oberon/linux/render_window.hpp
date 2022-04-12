#ifndef OBERON_LINUX_RENDER_WINDOW_HPP
#define OBERON_LINUX_RENDER_WINDOW_HPP

#include <string_view>

#include "../memory.hpp"
#include "../errors.hpp"
#include "../bounds.hpp"

namespace oberon::linux::detail {

  OBERON_PIMPL_FWD(render_window);

}

namespace oberon::linux {

  class window_system;
  class render_system;

  class render_window final {
  private:
    OBERON_PIMPL_PTR(detail, render_window);

    void regenerate_render_artifacts();
  public:
    render_window(window_system& win, render_system& rnd, const std::string_view title, const bounding_rect& bounds);
    render_window(render_window&& other) = default;
    render_window(const render_window& other) = delete;

    ~render_window() noexcept;

    render_window& operator=(render_window&& rhs) = default;
    render_window& operator=(const render_window& rhs) = delete;

    void show();
    void hide();
  };

  OBERON_EXCEPTION_TYPE(vk_surface_create_failed, "Failed to create Vulkan window surface.", 1);
  OBERON_EXCEPTION_TYPE(vk_surface_format_enumeration_failed, "Failed to enumerate available Vulkan surface formats.",
                        1);
  OBERON_EXCEPTION_TYPE(vk_presentation_mode_enumeration_failed, "Failed to enumerate available Vulkan presentation modes.",
                        1);
  OBERON_EXCEPTION_TYPE(vk_retrieve_surface_capabilities_failed, "Failed to retrieve Vulkan surface capabilities.", 1);
  OBERON_EXCEPTION_TYPE(vk_surface_unsupported, "Presentation is not supported on the current Vulkan surface.", 1);
  OBERON_EXCEPTION_TYPE(vk_swapchain_create_failed, "Failed to create Vulkan swapchain.", 1);
  OBERON_EXCEPTION_TYPE(vk_swapchain_image_retrieve_failed, "Failed to retrieve Vulkan swapchain images.", 1);
  OBERON_EXCEPTION_TYPE(vk_image_view_create_failed, "Failed to create Vulkan image view.", 1);

}

#endif
