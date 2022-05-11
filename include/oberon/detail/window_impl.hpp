#ifndef OBERON_DETAIL_WINDOW_IMPL_HPP
#define OBERON_DETAIL_WINDOW_IMPL_HPP

#include <vector>

#include "../window.hpp"

#include "x11_vulkan.hpp"


namespace oberon::window_flag_bits {

  OBERON_DEFINE_BIT(none, 0);
  OBERON_DEFINE_BIT(shown, 1);

}

namespace oberon::window_signal_bits {

  OBERON_DEFINE_BIT(none, 0);
  OBERON_DEFINE_BIT(destroy, 1);

}

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
    xcb_timestamp_t m_last_ping{ };
    bitmask m_window_state{ window_flag_bits::none_bit };
    bitmask m_window_signals{ window_signal_bits::none_bit };

    // Vulkan
    VkSurfaceKHR m_surface{ };
    VkSwapchainKHR m_swapchain{ };
    std::vector<VkImage> m_swapchain_images{ };
    std::vector<VkImageView> m_swapchain_image_views{ };


    void open_parent_systems(io_subsystem& io, graphics_subsystem& graphics);
    void open_x_window(const std::string_view title, const bounding_rect& bounds);
    void open_vk_surface();
    void open_vk_swapchain();
    void open_vk_synch_artifacts();
    void close_vk_synch_artifacts();
    void close_vk_swapchain() noexcept;
    void close_vk_surface() noexcept;
    void close_x_window() noexcept;
    void close_parent_systems() noexcept;
  public:
    window_impl(context& ctx, const std::string_view title, const bounding_rect& bounds);
    window_impl(const window_impl& other) = delete;
    window_impl(window_impl&& other) = delete;

    ~window_impl() noexcept;

    window_impl& operator=(const window_impl& rhs) = delete;
    window_impl& operator=(window_impl&& rhs) = delete;

    bitmask get_signals() const;
    void clear_signals(const bitmask signals);
    bitmask get_flags() const;

    void show();
    void hide();
    void accept_message(const ptr<void> message);
  };

}

#endif
