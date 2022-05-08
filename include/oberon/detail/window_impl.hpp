#ifndef OBERON_DETAIL_WINDOW_IMPL_HPP
#define OBERON_DETAIL_WINDOW_IMPL_HPP

#include "../window.hpp"

#include "x11_vulkan.hpp"

namespace oberon::detail {

  class io_subsystem;
  class graphics_subsystem;

  class window_impl final {
  private:
    // Parent systems
    ptr<io_subsystem> m_io{ };
    ptr<graphics_subsystem> m_graphics{ };

    // X11
    xcb_window_t m_window_id{ };

    // Vulkan
    VkSurfaceKHR m_surface{ };
    void open_parent_systems(io_subsystem& io, graphics_subsystem& graphics);
    void open_x_window(const std::string_view title, const bounding_rect& bounds);
    void close_x_window() noexcept;
    void close_parent_systems() noexcept;
  public:
    window_impl(context& ctx, const std::string_view title, const bounding_rect& bounds);
    window_impl(const window_impl& other) = delete;
    window_impl(window_impl&& other) = delete;

    ~window_impl() noexcept;

    window_impl& operator=(const window_impl& rhs) = delete;
    window_impl& operator=(window_impl&& rhs) = delete;

    void show();
    void hide();
  };

}

#endif
