/**
 * @file graphics.hpp
 * @brief Linux, X11, and Vulkan graphics class implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
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

  /**
   * @brief An object implementing oberon::graphics using Vulkan and X11.
   */
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
    buffer_mode m_buffer_mode{ };
    VkPresentModeKHR m_present_mode{ VK_PRESENT_MODE_FIFO_KHR };
    std::unordered_set<presentation_mode> m_available_present_modes{ };

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
    /**
     * @brief Create a new graphics object.
     * @param sys The parent system.
     * @param win The window that will be rendered to.
     */
    graphics(system& sys, window& win);

    /// @cond
    graphics(const graphics& other) = delete;
    graphics(graphics&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a graphics object.
     */
    virtual ~graphics() noexcept;

    /// @cond
    graphics& operator=(const graphics& rhs) = delete;
    graphics& operator=(graphics&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve a list of currently available graphics devices.
     * @return A list of 0 or more graphics_device structures.
     */
    const std::vector<graphics_device>& available_devices() const override;

    /**
     * @brief Retrieve the implementation's preferred graphics device.
     * @details The preferred device is selected by finding the first discrete graphics_device. If no discrete
     *          graphics_devices are available then the first graphics_device is returned.
     * @return The preferred device.
     */
    const graphics_device& preferred_device() const override;

    /**
     * @brief Retrieve the last requested buffering mode.
     * @details This is only the last mode requested. It is not necessarily representative of the number of buffers
     *          actually in use (if any).
     * @return The last buffering mode that the client application requested. If no mode has been requested then the
     *         default is automatic.
     */
    buffer_mode last_requested_buffer_mode() const override;

    /**
     * @brief Retrieve the number of image buffers currently in use by the implementation.
     * @details This may not match the number of buffers that the requested mode would imply. For example, it is
     *          entirely possible for an application to request double buffering and for this method to return 3.
     * @return If a device is currently open this returns the number of buffers in use. Otherwise 0.
     */
    u32 current_buffer_count() const override;

    /**
     * @brief Request a new buffering mode.
     * @details This attempts to update the number of swapchain images in use to match the requested mode. Ultimately,
     *          the number of images available will always be up to the driver. The renderer is dirtied if a device
     *          is opened when this request is made.
     * @param mode The new mode to request.
     */
    void request_buffer_mode(const buffer_mode mode) override;

    /**
     * @brief Retreive a set of supported presentation modes.
     * @details The set of available modes may change depending on which device (if any) is open.
     * @return The set of presentation modes currently supported by the implementation.
     */
    const std::unordered_set<presentation_mode>& available_presentation_modes() const override;

    /**
     * @brief Retrieve the current presentation mode.
     * @details When no device is open this method's result may not be meaningful.
     * @return The current presentation mode.
     */
    presentation_mode current_presentation_mode() const override;
    /**
     * @brief Request a new presentation mode.
     * @details This changes the Vulkan presentation mode so long as the input mode is a valid Vulkan presentation
     *          mode and is available in the current configuration. If the mode is not available or is not valid the
     *          mode will be changed to fifo instead. The fifo mode is the only mode guaranteed to be available. After
     *          making this request the renderer is dirtied if a device is opened.
     * @param mode The new mode to request.
     */
    void request_presentation_mode(const presentation_mode mode) override;

    /**
     * @brief Check whether a device is currently opened.
     * @return True if a device is opened and ready for use. False otherwise.
     */
    bool is_device_opened() const override;

    /**
     * @brief Open a device for rendering.
     * @details If a device is already opened it will be closed and the new device will then be opened.
     * @param device The specific device to open. This must be one of the devices returned by available_devices().
     */
    void open_device(const graphics_device& device) override;

    /**
     * @brief Close a device.
     * @details If no device is opened this is a no-op.
     */
    void close_device() override;

    /**
     * @brief Wait for the rendering device to become idle.
     * @details If no device is opened this is a no-op.
     */
    void wait_for_device_to_idle() override;

    /**
     * @brief Indicate that the renderer is dirty and potentially needs reinitialization.
     * @details If no device is opened this is a no-op.
     */
    void dirty_renderer() override;

    /**
     * @brief Check if the renderer is currently dirty.
     * @return True if the renderer is dirty. False otherwise.
     */
    bool is_renderer_dirty() const override;

    /**
     * @brief Check if the renderer is in the middle of a frame.
     * @return True if the renderer is currently processing a frame. False otherwise.
     */
    bool is_in_frame() const override;

    /**
     * @brief Begin processing a frame.
     * @details If no device is opened or the renderer is already processing a frame this is a no-op.
     */
    void begin_frame() override;

    /**
     * @brief End processing a frame.
     * @details If no device is opened or the renderer is not processing a frame this is a no-op.
     */
    void end_frame() override;

    /**
     * @brief Draw a test image.
     * @details This draws a simple RGB triangle. If no device is opened or the renderer is not processing a frame
     *          this is a no-op.
     */
    void draw_test_image() override;
  };

}

#endif
