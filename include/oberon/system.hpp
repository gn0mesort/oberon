/**
 * @file system.hpp
 * @brief liboberon system objects.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

#include <span>

#include "memory.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(system_impl);

}

namespace oberon {

  class graphics_device;

  /**
   * @class system
   * @brief An object representing the fully initialized Oberon library.
   */
  class system final {
  public:
    using implementation_type = internal::base::system_impl;
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::system_impl);
  public:
    /// @cond
    system(ptr<implementation_type>&& impl);
    system(const system& other) = delete;
    system(system&& other) = delete;

    ~system() noexcept = default;

    system& operator=(const system& rhs) = delete;
    system& operator=(system&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the internal `system` implementation.
     * @return A reference to the `system`'s implementation object.
     */
    implementation_type& implementation();

    /**
     * @brief Retrieve all available graphics devices.
     * @return A `std::span` which can be used to access any `graphics_device` provided by the `system`.
     */
    std::span<graphics_device> graphics_devices();

    /**
     * @brief Retrieve the `system`'s preferred `graphics_device`.
     * @details The exact method by which a system selects this device is implementation specific. At the simplest,
     *          this method might return `graphics_devices[0]`. If no devices are available then calling this method
     *          will generate an error.
     * @return A reference to a `graphics_device`
     */
    graphics_device& preferred_graphics_device();
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, system);

}

#endif
