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
  class vk_device;

  /**
   * @brief An object implementing oberon::graphics using Vulkan and X11.
   */
  class graphics : public oberon::graphics {
  private:
    VkPipelineLayout m_test_image_pipeline_layout{ };
    VkPipeline m_test_image_pipeline{ };
    VkPipelineLayout m_unlit_pc_pipeline_layout{ };
    VkPipeline m_unlit_pc_pipeline{ };

    ptr<system> m_parent{ };
    ptr<window> m_target{ };
    std::vector<graphics_device> m_graphics_devices{ };
    ptr<vk_device> m_vk_device{ };

    ptr<vk_device> create_device(const graphics_device& physical_device);
    std::vector<char> read_shader_binary(const std::filesystem::path& file);
    void initialize_graphics_pipelines();
    void initialize_test_image_pipeline(const VkPipelineRenderingCreateInfo& rendering_info,
                                        const VkDescriptorSetLayout uniform_descriptor_layout);
    void initialize_unlit_pc_pipeline(const VkPipelineRenderingCreateInfo& rendering_info,
                                      const VkDescriptorSetLayout uniform_descriptor_layout);
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
     * @brief Retrieve the number of image buffers currently in use by the implementation.
     * @details This may not match the number of buffers that the requested mode would imply. For example, it is
     *          entirely possible for an application to request double buffering and for this method to return 3.
     * @return If a device is currently open this returns the number of buffers in use. Otherwise 0.
     */
    u32 current_buffer_count() const override;

    void request_buffer_count(const u32 buffers) override;

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
     * @brief Change the current render device.
     * @details This is expensive. The current device will be destroyed (which drains all device queues) and then
     *          the new device will be constructed from the input description.
     * @param device The specific device to use for rendering. This must be one of the devices returned by
     *               available_devices().
     */
    void change_device(const graphics_device& device) override;
    void flush_device_queues() const override;

    /**
     * @brief Draw a test image.
     * @details This draws a simple RGB triangle.
     */
    void draw_test_image() override;
    void write_uniform_buffer(const uniform_buffer& ub) override;
    void draw_buffer_unlit_pc(oberon::buffer& buf) override;
    oberon::buffer& allocate_buffer(const buffer_type type, const usize sz) override;
    void free_buffer(oberon::buffer& buf) override;

    void submit_and_present_frame() override;

    vk_device& device();
  };

}

#endif
