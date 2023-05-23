/**
 * @file system_impl.hpp
 * @brief Internal Linux+X11 system API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_LINUX_X11_SYSTEM_IMPL_HPP
#define OBERON_INTERNAL_LINUX_X11_SYSTEM_IMPL_HPP

#include <span>
#include <string>
#include <array>

#include "../../base/system_impl.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::linux::x11 {

  class wsi_context;

  /**
   * @class system_impl
   * @brief The Linx+X11 implementation of the `system`.
   */
  class system_impl final : public base::system_impl {
  private:
    static inline std::array<u8, 16> s_exclusive_device_uuid{ 0, 0, 0, 0,
                                                              0, 0, 0, 0,
                                                              0, 0, 0, 0,
                                                              0, 0, 0, 0 };

    ptr<wsi_context> m_wsi_context{ };
    usize m_graphics_device_count{ };
    csequence m_graphics_device_binary{ };
  public:
    /**
     * @brief Instruct the system to only initialize the device with the matching UUID.
     * @param uuid The UUID of the device that will be initialized.
     */
    static void initialize_only(const basic_readonly_sequence<u8> uuid);

    /**
     * @brief Create a `system_impl`.
     * @details If `initialize_only()` has been called with a valid non-null UUID then only the corresponding device
     *          will be enabled. If the corresponding device doesn't support Oberon's requirements then the system
     *          will be initialized with 0 devices.
     * @param wsi A pre-existing `wsi_context`. The `system_impl` will take ownership of it.
     * @param gfx A pre-existing `graphics_context`. The `system_impl` will take ownership of it.
     */
    system_impl(ptr<wsi_context>&& wsi, ptr<base::graphics_context>&& gfx);

    /// @cond
    system_impl(const system_impl& other) = delete;
    system_impl(system_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `system_impl`.
     */
    ~system_impl() noexcept;

    /// @cond
    system_impl& operator=(const system_impl& rhs) = delete;
    system_impl& operator=(system_impl&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve a list of available `graphics_devices`.
     * @return A list containing 0 or more `graphics_devices`.
     */
    std::span<graphics_device> graphics_devices() override;
  };

}

#endif
