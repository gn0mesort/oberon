/**
 * @file graphics_device.hpp
 * @brief Graphics device objects..
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_GRAPHICS_DEVICE_HPP
#define OBERON_GRAPHICS_DEVICE_HPP

#include <string>

#include "types.hpp"
#include "memory.hpp"

#include "concepts/has_internal_implementation.hpp"

/// @cond
#define OBERON_GRAPHICS_DEVICE_TYPES \
  OBERON_GRAPHICS_DEVICE_TYPE(other, 0) \
  OBERON_GRAPHICS_DEVICE_TYPE(integrated, 1) \
  OBERON_GRAPHICS_DEVICE_TYPE(discrete, 2) \
  OBERON_GRAPHICS_DEVICE_TYPE(virtualized, 3) \
  OBERON_GRAPHICS_DEVICE_TYPE(cpu, 4)
/// @endcond

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(graphics_device_impl);

}

namespace oberon {

/// @cond
#define OBERON_GRAPHICS_DEVICE_TYPE(name, value) name = (value),
/// @endcond

  /**
   * @enum graphics_device_type
   * @brief Types of `graphics_device`s
   * @details This should be kept in sync with Vulkan's `VkPhysicalDeviceType` enumeration. The different types are
   *          not all created equal. For example, `other` devices essentially do not exist in reality. Generally,
   *          `discrete` devices represent a piece of physical hardware, `integrated` devices represent physical
   *          hardware that is integrated with a CPU, and `cpu` devices are software renderers like llvmpipe. Like
   *          `other` devices, `virtualized` devices are rare.
   */
  enum class graphics_device_type {
    OBERON_GRAPHICS_DEVICE_TYPES
  };

/// @cond
#undef OBERON_GRAPHICS_DEVICE_TYPE
/// @endcond

  /**
   * @class graphics_device
   * @brief An object representing an available physical or virtual graphics device.
   * @details Although a host machine might expose any number of devices, Oberon only initializes devices that meet
   *          its minimum requirements.
   */
  class graphics_device final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::graphics_device_impl);
  public:
    using implementation_type = internal::base::graphics_device_impl;

    /// @cond
    graphics_device(ptr<internal::base::graphics_device_impl>&& impl);
    graphics_device(const graphics_device& other) = delete;
    graphics_device(graphics_device&& other) = delete;

    ~graphics_device() noexcept = default;

    graphics_device& operator=(const graphics_device& rhs) = delete;
    graphics_device& operator=(graphics_device&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the `graphics_device`'s implementation.
     * @return A reference to the `graphics_device`'s implementation object.
     */
    implementation_type& implementation();

    /**
     * @brief Retrieve the type of the `graphics_device`.
     * @return The type of device represented by the `graphics_device`.
     */
    graphics_device_type type() const;
    /**
     * @brief Retrieve the `graphics_device`'s name.
     * @return The name of the `graphics_device`.
     */
    std::string name() const;

    /**
     * @brief Retrieve the name of the driver that controls the `graphics_device`.
     * @return The name of the device driver.
     */
    std::string driver_name() const;

    /**
     * @brief Retrieve an information string from the driver.
     * @return An informational string describing the `graphics_device`'s driver.
     */
    std::string driver_info() const;

    /**
     * @brief Retrieve the PCI vendor ID belonging to the `graphics_device`.
     * @details While PCI vendor IDs are normally 16-bit values, Vulkan devices can have larger IDs. These expanded
     *          IDs, generally, represent vendors who do not have registered PCI IDs.
     *          Intel uses the ID 0x8086.
     *          Nvidia uses the ID 0x10de.
     *          AMD uses the ID 0x1002.
     * @return A 32-bit PCI vendor ID.
     */
    u32 vendor_id() const;

    /**
     * @brief Retrieve the PCI device ID belonging to the `graphics_device`.
     * @return A 32-bit PCI device ID.
     */
    u32 device_id() const;

    /**
     * @brief Calculate the total memory that is local to the `graphics_device`.
     * @return The total memory belonging to the `graphics_device` in bytes.
     */
    usize total_memory() const;

    /**
     * @brief Retrieve a UUID representing the `graphics_device`
     * @details This ID will be unique to the current host machine configuration. However, it is not necessarily
     *          to uniquely identify a device if the machine is reconfigured. Although the value has the structure of
     *          a UUID, it isn't guaranteed to actually be a valid UUID. For example, Mesa returns the bus number of
     *          the device.
     *          In short, this is a unique string representing the `graphics_device` but it is not guaranteed to
     *          remain unique if hardware is moved around. It is also not guaranteed to be a meaningful UUID with
     *          proper version and variant information.
     * @return The string representation of the `graphics_device`'s UUID.
     */
    std::string uuid() const;

  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, graphics_device);

  /**
   * @brief Convert a `graphics_device_type` to a string.
   * @param type The `graphics_device_type` to convert.
   * @return A string representing the input type.
   */
  std::string to_string(const graphics_device_type type);

}

#endif
