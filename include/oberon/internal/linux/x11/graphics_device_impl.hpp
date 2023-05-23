/**
 * @file graphics_device_impl.hpp
 * @brief Internal Linux+X11 graphics_device API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_LINUX_X11_GRAPHICS_DEVICE_IMPL_HPP
#define OBERON_INTERNAL_LINUX_X11_GRAPHICS_DEVICE_IMPL_HPP

#include <unordered_set>
#include <string>

#include "../../base/graphics_device_impl.hpp"

namespace oberon::internal::linux::x11 {

  class wsi_context;

  /**
   * @class graphics_device_impl
   * @brief The Linux+X11 implementation of `graphics_device`s.
   */
  class graphics_device_impl final : public base::graphics_device_impl {
  private:
    ptr<wsi_context> m_wsi_context{ };

    u32 select_queue_family();
    std::unordered_set<std::string> available_extensions();
  public:
    /**
     * @brief Create a `graphics_device_impl`.
     * @param wsi A reference to the system WSI context.
     * @param gfx A reference to the system graphics context.
     * @param physical_device The `physical_graphics_device` on which the device will be based.
     * @param required_extensions A set of required Vulkan device extensions. If any of these extensions are
     *                            unsupported then an error will be thrown.
     * @param requested_extensions A set of requested Vulkan device extensions. If an extension in the set is
     *                             unsupported then it will be ignored.
     */
    graphics_device_impl(wsi_context& wsi, base::graphics_context& gfx,
                         const base::physical_graphics_device& physical_device,
                         const std::unordered_set<std::string>& required_extensions,
                         const std::unordered_set<std::string>& requested_extensions);

    /// @cond
    graphics_device_impl(const graphics_device_impl& other) = delete;
    graphics_device_impl(graphics_device_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `graphics_device_impl`.
     */
    ~graphics_device_impl() noexcept = default;

    /// @cond
    graphics_device_impl& operator=(const graphics_device_impl& rhs) = delete;
    graphics_device_impl& operator=(graphics_device_impl&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the underlying `wsi_context` of the `graphics_device`.
     * @return A reference to the `wsi_context`.
     */
    wsi_context& wsi();
  };

}

#endif
