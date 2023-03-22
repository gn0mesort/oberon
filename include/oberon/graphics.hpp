/**
 * @file graphics.hpp
 * @brief Graphics class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_GRAPHICS_HPP
#define OBERON_GRAPHICS_HPP

#include <string>
#include <vector>
#include <unordered_set>

#include "types.hpp"
#include "memory.hpp"
#include "buffer.hpp"

/**
 * @def OBERON_GRAPHICS_DEVICE_TYPES
 * @brief A list of graphics device (i.e., GPU) types and enumerator values for them.
 * @details These are designed to match the values of VkPhysicalDeviceType in the Vulkan specification.
 * @see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceType.html
 */
#define OBERON_GRAPHICS_DEVICE_TYPES \
  OBERON_GRAPHICS_DEVICE_TYPE(integrated, 1) \
  OBERON_GRAPHICS_DEVICE_TYPE(discrete, 2) \
  OBERON_GRAPHICS_DEVICE_TYPE(virtualized, 3) \
  OBERON_GRAPHICS_DEVICE_TYPE(cpu, 4)

/**
 * @def OBERON_PIPELINE_STAGES
 * @brief A list of supported graphics pipeline stages and enumerator values for them.
 * @details These are designed to match the values of VkShaderStageFlagBits in the Vulkan specification.
 * @see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderStageFlagBits.html
 */
#define OBERON_PIPELINE_STAGES \
  OBERON_PIPELINE_STAGE(vertex, 0x1) \
  OBERON_PIPELINE_STAGE(fragment, 0x10)

/**
 * @def OBERON_PRESENTATION_MODES
 * @brief A list of supported presentation modes and enumerator values for them.
 * @details These are designed to match the values of VkPresentModeKHR in the Vulkan specification. For names that
 *          share a name with a value in VkPresentModeKHR the corresponding enumerator value should be one more than
 *          in the Vulkan specification. That is, if VK_PRESENT_MODE_FIFO_KHR is 2 then fifo must be 3.
 * @see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
 */
#define OBERON_PRESENTATION_MODES \
  OBERON_PRESENTATION_MODE(immediate, 1) \
  OBERON_PRESENTATION_MODE(mailbox, 2) \
  OBERON_PRESENTATION_MODE(fifo, 3) \
  OBERON_PRESENTATION_MODE(fifo_relaxed, 4)

namespace oberon {

  constexpr const u32 OBERON_DOUBLE_BUFFER{ 2 };
  constexpr const u32 OBERON_TRIPLE_BUFFER{ 3 };

/// @cond
#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) name = (value),
/// @endcond

  /**
   * @brief An enumeration of possible graphics device types.
   */
  enum class graphics_device_type {
    other = 0,
    OBERON_GRAPHICS_DEVICE_TYPES
  };

/// @cond
#undef OBERON_GRAPHICS_DEVICE_TYPE
/// @endcond

/// @cond
#define OBERON_PIPELINE_STAGE(name, value) name = (value),
/// @endcond

  /**
   * @brief An enumeration of possible graphics pipeline stages.
   */
  enum class pipeline_stage {
    none = 0,
    OBERON_PIPELINE_STAGES
  };

/// @cond
#undef OBERON_PIPELINE_STAGE
/// @endcond

/// @cond
#define OBERON_PRESENTATION_MODE(name, value) name = (value),
/// @endcond

  /**
   * @brief An enumeration of possible presentation modes.
   */
  enum presentation_mode {
    automatic = 0,
    OBERON_PRESENTATION_MODES
  };

/// @cond
#undef OBERON_PRESENTATION_MODE
/// @endcond

  /**
   * @brief A simple structure representing a graphics device (i.e., GPU).
   */
  struct graphics_device final {
    /**
     * @brief The type of the device.
     */
    graphics_device_type type{ };

    /**
     * @brief A 32-bit PCI vendor ID representing the device's vendor.
     */
    u32 vendor_id{ };

    /**
     * @brief A 32-bit PCI device ID representing the specific device.
     */
    u32 device_id{ };

    /**
     * @brief An implementation specific handle identifying the device.
     */
    uptr handle{ };

    /**
     * @brief The name of the device as provided by the implementation.
     */
    std::string name{ };

    /**
     * @brief The name of the corresponding device driver as provided by the implementation.
     */
    std::string driver_name{ };

    /**
     * @brief Driver specific information (e.g., the version) as provided by the implementation.
     */
    std::string driver_info{ };
  };

  /**
   * @brief An object representing the underlying system's graphics capabilities.
   */
  class graphics {
  public:
    /**
     * @brief Create a new graphics object.
     */
    graphics() = default;

    /**
     * @brief Copy a graphics object.
     * @param other The object to copy.
     */
    graphics(const graphics& other) = default;

    /**
     * @brief Move a graphics object.
     * @param other The object to move.
     */
    graphics(graphics&& other) = default;

    /**
     * @brief Destroy a graphics object.
     */
    inline virtual ~graphics() noexcept = 0;


    /**
     * @brief Copy a graphics object.
     * @param rhs The object to copy.
     * @return A reference to the assigned to graphics object.
     */
    graphics& operator=(const graphics& rhs) = default;

    /**
     * @brief Move a graphics object.
     * @param rhs The object to move.
     * @return A reference to the assigned to graphics object.
     */
    graphics& operator=(graphics&& rhs) = default;


    /**
     * @brief Retrieve a list of currently available graphics devices.
     * @return A list of 0 or more graphics_device structures.
     */
    virtual const std::vector<graphics_device>& available_devices() const = 0;

    /**
     * @brief Retrieve the implementation's preferred graphics device.
     * @details The method by which a device is selected is totally up to the implementation. This might be as
     *          simple as selecting the first graphics device from the list of available devices.
     * @return The preferred device.
     */
    virtual const graphics_device& preferred_device() const = 0;

    /**
     * @brief Retrieve the number of image buffers currently in use by the implementation.
     * @details This may not match the number of buffers that the requested mode would imply. For example, it is
     *          entirely possible for an application to request double buffering and for this method to return 3.
     * @return If a device is currently open this returns the number of buffers in use. Otherwise 0.
     */
    virtual u32 current_buffer_count() const = 0;

    virtual void request_buffer_count(const u32 count) = 0;

    /**
     * @brief Retreive a set of supported presentation modes.
     * @details The set of available modes may change depending on which device (if any) is open.
     * @return The set of presentation modes currently supported by the implementation.
     */
    virtual const std::unordered_set<presentation_mode>& available_presentation_modes() const = 0;

    /**
     * @brief Retrieve the current presentation mode.
     * @details When no device is open this method's result may not be meaningful.
     * @return The current presentation mode.
     */
    virtual presentation_mode current_presentation_mode() const = 0;

    /**
     * @brief Request a new presentation mode.
     * @details Just like with buffering modes, a request to change the presentation mode is merely a suggestion to
     *          the implementation. Requests are not required to be honored. If a request would be honored but the
     *          requested mode is not available then the implementation should select its preferred mode. If a
     *          request would be honored and the result would degrade renderer performance then the renderer must be
     *          dirtied.
     * @param mode The new mode to request.
     */
    virtual void request_presentation_mode(const presentation_mode mode) = 0;

    /**
     * @brief Change the current render device.
     * @param device The specific device to use for rendering. This must be one of the devices returned by
     *               available_devices().
     */
    virtual void change_device(const graphics_device& device) = 0;

    /**
     * @brief Draw a test image.
     * @details This draws a simple RGB triangle. If no device is opened or the renderer is not processing a frame
     *          this is a no-op.
     */
    virtual void draw_test_image() = 0;

    virtual void draw_buffer_unlit_pc(buffer& buf) = 0;
    virtual buffer& allocate_buffer(const buffer_type type, const usize sz) = 0;
    virtual void free_buffer(buffer& buf) = 0;

    virtual void submit_and_present_frame() = 0;
    virtual void flush_device_queues() const = 0;
  };

  /// @cond
  graphics::~graphics() noexcept { }
  /// @endcond

  /**
   * @brief Convert a graphics_device_type to a string.
   * @param type The type to convert to a string.
   * @return The name of the input type or "other" if the type is not recognized.
   */
  std::string to_string(const graphics_device_type type);

  /**
   * @brief Convert a pipeline_stage to a string.
   * @param stage The stage to convert to a string.
   * @return The name of the input stage or "none" if the stage is not recognized.
   */
  std::string to_string(const pipeline_stage stage);

  /**
   * @brief Convert a presentation_mode to a string.
   * @param mode The mode to convert to a string.
   * @return The name of the input mode or "automatic" if the mode is unrecognized.
   */
  std::string to_string(const presentation_mode mode);

}

#endif
