#ifndef OBERON_LINUX_GRAPHICS_HPP
#define OBERON_LINUX_GRAPHICS_HPP

#include <filesystem>
#include <array>

#include "../memory.hpp"
#include "../graphics.hpp"

#include "vk.hpp"

namespace oberon::linux {

  class system;
  class window;

  class graphics : public oberon::graphics {
  private:
    struct queue_selection final {
      u32 graphics_queue{ };
      u32 presentation_queue{ };
    };

    enum pipeline_stage_index : usize {
      PIPELINE_VERTEX_STAGE,
      PIPELINE_FRAGMENT_STAGE,
      MAX_PIPELINE_STAGE
    };

    struct graphics_program final {
      std::array<usize, MAX_PIPELINE_STAGE> pipeline_stage_indices{ };
      usize program_index{ };
    };

    static queue_selection select_queues_heuristic(const graphics& gfx, const u32 vendor,
                                                   const VkPhysicalDevice device);
    static queue_selection select_queues_amd(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);
    static queue_selection select_queues_nvidia(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);
    static queue_selection select_queues_intel(const graphics& gfx, const u32 vendor, const VkPhysicalDevice device);

    ptr<system> m_parent{ };
    ptr<window> m_target{ };

    std::vector<graphics_device> m_graphics_devices{ };
    VkPhysicalDevice m_vk_selected_physical_device{ };
    queue_selection m_vk_selected_queue_families{ };
    VkDevice m_vk_device{ };
    VkQueue m_vk_graphics_queue{ };
    VkQueue m_vk_present_queue{ };
    VkCommandPool m_vk_command_pool{ };
    std::array<VkCommandBuffer, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_command_buffers{ };
    std::array<VkSemaphore, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_image_available_sems{ };
    std::array<VkSemaphore, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_render_finished_sems{ };
    std::array<VkFence, OBERON_LINUX_VK_MAX_FRAMES_IN_FLIGHT> m_vk_in_flight_frame_fences{ };
    u32 m_frame_index{ 0 };
    VkRect2D m_vk_render_area{ { 0, 0 }, { 640, 360 } };
    VkSurfaceFormatKHR m_vk_surface_format{ };
    VkSwapchainKHR m_vk_swapchain{ };
    std::vector<VkImage> m_vk_swapchain_images{ };
    std::vector<VkImageView> m_vk_swapchain_image_views{ };
    u32 m_image_index{ 0 };
    VkResult m_vk_last_frame{ VK_SUCCESS };
    std::vector<VkPipelineShaderStageCreateInfo> m_vk_pipeline_shader_stages{ };
    std::vector<VkPipeline> m_vk_graphics_pipelines{ };
    std::vector<VkPipelineLayout> m_vk_graphics_pipeline_layouts{ };
    graphics_program m_vk_test_image_program{ };
    bool m_is_in_frame{ };
    bool m_is_renderer_dirty{ };

    // Requires device selection.
    VkSurfaceFormatKHR select_surface_format(const VkFormat preferred_format,
                                             const VkColorSpaceKHR preferred_color_space);

    void initialize_device(const VkPhysicalDevice physical_device);
    void deinitialize_device();
    void initialize_renderer(const VkSwapchainKHR old);
    void initialize_swapchain_image_views();
    void deinitialize_swapchain_image_views();
    void initialize_graphics_programs();
    void deinitialize_graphics_programs();
    void deinitialize_renderer(const VkSwapchainKHR old);
    void reinitialize_renderer();
    std::vector<char> read_shader_binary(const std::filesystem::path& file);
    graphics_program initialize_test_image_program();
    u32 acquire_next_image(VkSemaphore& image_available);
    void wait_for_in_flight_fences(ptr<VkFence> fences, const usize sz);
    void begin_rendering(VkCommandBuffer& command_buffer, VkImage& image, VkImageView& image_view);
    void end_rendering(VkCommandBuffer& command_buffer, VkImage& image);
    void present_image(const usize frame_index, const usize image_index);
    void present_image(VkCommandBuffer& command_buffer, VkSemaphore& image_available, VkSemaphore& render_finished,
                       VkFence in_flight_fence, const u32 image_index);
  public:
    graphics(system& sys, window& win);
    graphics(const graphics& other) = delete;
    graphics(graphics&& other) = delete;

    virtual ~graphics() noexcept;

    graphics& operator=(const graphics& rhs) = delete;
    graphics& operator=(graphics&& rhs) = delete;

    const std::vector<graphics_device>& available_devices() const override;
    const graphics_device& preferred_device() const override;
    bool is_device_opened() const override;
    void open_device(const graphics_device& device) override;
    void close_device() override;
    void wait_for_device_to_idle() override;
    void dirty_renderer() override;
    bool is_renderer_dirty() const override;
    bool is_in_frame() const override;
    void begin_frame() override;
    void end_frame() override;
    void draw_test_image() override;
  };

}

#endif
