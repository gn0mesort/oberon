#ifndef OBERON_LINUX_WINDOW_HPP
#define OBERON_LINUX_WINDOW_HPP

#include <string_view>
#include <vector>

#include "../memory.hpp"
#include "../errors.hpp"
#include "../window.hpp"

#include "../bounds.hpp"

#include "x11.hpp"
#include "vulkan.hpp"

namespace oberon {

  class context;

}

namespace oberon::linux {

  class io_subsystem;
  class graphics_subsystem;

  class window final : public abstract_window {
  private:
    ptr<io_subsystem> m_parent_io{ };
    ptr<graphics_subsystem> m_parent_gfx{ };

    xcb_window_t m_window_id{ };
    xcb_atom_t m_wm_delete_window{ };
    mutable bool m_window_bounds_dirty{ true };
    mutable bounding_rect m_window_bounds{ };
    VkSurfaceKHR m_surface{ };
    std::vector<VkSurfaceFormatKHR> m_available_surface_formats{ };
    std::vector<VkPresentModeKHR> m_available_present_modes{ };
    // Probably a safe default
    VkSurfaceFormatKHR m_current_surface_format{ VK_FORMAT_B8G8R8A8_SRGB,
                                                 VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    VkPresentModeKHR m_current_present_mode{ VK_PRESENT_MODE_FIFO_KHR };
    VkSurfaceCapabilitiesKHR m_surface_capabilities{ };
    VkSwapchainCreateInfoKHR m_swapchain_info{ };
    // Default to at least double-buffering.
    // Image counts are always + 1 when the swapchain is created so 1 = double, 2 = triple.
    // This is just a suggestion to the system though.
    u32 m_swapchain_image_count{ 1 };
    VkSwapchainKHR m_swapchain{ };
    std::vector<VkImage> m_swapchain_images{ };
    std::vector<VkImageView> m_swapchain_image_views{ };

    void open_x_window(const std::string_view title, const bounding_rect& bounds);
    void discover_x_geometry() const;
    void open_vk_surface();
    void discover_vk_surface_features();
    void configure_vk_swapchain();
    void open_vk_swapchain(const VkSwapchainKHR old);
    void close_vk_swapchain() noexcept;
    void close_vk_surface() noexcept;
    void close_x_window() noexcept;
  public:
    window(context& ctx, const std::string_view title, const bounding_rect& bounds);
    window(const window& other) = delete;
    window(window&& other) = default;

    ~window() noexcept;

    window& operator=(const window& rhs) = delete;
    window& operator=(window&& rhs) = default;

    void show() override;
    void hide() override;
    const bounding_rect& bounds() const override;
  };

  static_assert(is_window_v<window>);

  OBERON_EXCEPTION_TYPE(vk_create_surface_failed, "Failed to create Vulkan window surface.", 1);
  OBERON_EXCEPTION_TYPE(vk_query_surface_failed, "Failed to query Vulkan window surface properties.", 1);
  OBERON_EXCEPTION_TYPE(vk_surface_unsupported, "Presentation is not supported in the current Vulkan configuration.",
                        1);
  OBERON_EXCEPTION_TYPE(vk_create_swapchain_failed, "Failed to create Vulkan image swapchain.", 1);
  OBERON_EXCEPTION_TYPE(vk_enumerate_swapchain_images_failed, "Failed to enumerate Vulkan swapchain images.", 1);
  OBERON_EXCEPTION_TYPE(vk_create_image_view_failed, "Failed to create Vulkan image view.", 1);

}

#endif
