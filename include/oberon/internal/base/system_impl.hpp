/**
 * @file system_impl.hpp
 * @brief Internal system API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_BASE_SYSTEM_IMPL_HPP
#define OBERON_INTERNAL_BASE_SYSTEM_IMPL_HPP

#include <span>

#include "../../memory.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::base {

  class graphics_context;

  /**
   * @class system_impl
   * @brief The base system implementation.
   */
  class system_impl {
  protected:
    ptr<graphics_context> m_graphics_context{ };

    system_impl(ptr<graphics_context>&& gfx);
  public:
    /// @cond
    system_impl(const system_impl& other) = delete;
    system_impl(system_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `system_impl`.
     */
    virtual ~system_impl() noexcept;

    /// @cond
    system_impl& operator=(const system_impl& rhs) = delete;
    system_impl& operator=(system_impl&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve a list of available `graphics_devices`.
     * @return A list containing 0 or more `graphics_devices`.
     */
    virtual std::span<graphics_device> graphics_devices() = 0;
  };

}

#endif
