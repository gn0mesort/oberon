#ifndef OBERON_LINUX_GRAPHICS_HPP
#define OBERON_LINUX_GRAPHICS_HPP

#include "../memory.hpp"
#include "../graphics.hpp"

#include "vk.hpp"

namespace oberon::linux {

  class system;
  class window;

  class graphics final : public oberon::graphics {
  private:
    struct queue_selection final {
      u32 graphics_queue{ };
      u32 presentation_queue{ };
    };

    static queue_selection select_queues_heuristic(const graphics& gfx, const u32 vendor,
                                                   const VkPhysicalDevice device);
    static queue_selection select_queues_amd(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);
    static queue_selection select_queues_nvidia(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);
    static queue_selection select_queues_intel(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);

    ptr<system> m_parent{ };
    ptr<window> m_target{ };

    std::vector<VkPhysicalDevice> m_physical_devices{ };
    std::vector<graphics_device> m_graphics_devices{ };
    queue_selection m_selected_queue_families{ };
    VkPhysicalDevice m_vk_selected_physical_device{ };
    VkDevice m_vk_device{ };
    VkQueue m_vk_graphics_queue{ };
    VkQueue m_vk_present_queue{ };
    VkCommandPool m_vk_command_pool{ };
    std::array<VkCommandBuffer, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_command_buffers{ };
    std::array<VkSemaphore, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_image_available_sems{ };
    std::array<VkSemaphore, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_render_finished_sems{ };
    std::array<VkFence, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_in_flight_frame_fences{ };
    VkRect2D m_vk_render_area{ { 0, 0 }, { 640, 360 } };
    VkSwapchainKHR m_vk_swapchain{ };
    std::vector<VkImage> m_vk_swapchain_images{ };
    std::vector<VkImageView> m_vk_swapchain_image_views{ };
    u32 m_frame_index{ 0 };
    u32 m_image_index{ 0 };
    VkResult m_vk_last_frame{ VK_SUCCESS };

    void initialize_device(const VkPhysicalDevice physical_device);
    void deinitialize_device();
    void initialize_renderer(const VkSwapchainKHR old);
    void deinitialize_image_views();
    void deinitialize_renderer(const VkSwapchainKHR old);
    void deinitialize_renderer();
  public:
    graphics(system& sys, window& win);
    graphics(const graphics& other) = delete;
    graphics(graphics&& other) = delete;

    ~graphics() noexcept;

    graphics& operator=(const graphics& rhs) = delete;
    graphics& operator=(graphics&& rhs) = delete;

    const std::vector<graphics_device>& available_devices() const override;
    void select_device(const graphics_device& device) override;
    void wait_for_device_to_idle() override;
    void reinitialize_renderer() override;
    void begin_frame() override;
    void end_frame() override;
  };

}

#endif
