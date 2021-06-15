#ifndef OBERON_DETAIL_CONTEXT_IMPL_HPP
#define OBERON_DETAIL_CONTEXT_IMPL_HPP

#include "../context.hpp"

#include <unordered_set>

#include "object_impl.hpp"

#include "x11.hpp"
#include "vulkan.hpp"
#include "vulkan_function_table.hpp"

namespace oberon {
namespace detail {
  struct context_impl : public object_impl {
    ptr<xcb_connection_t> x11_connection{ };
    ptr<xcb_screen_t> x11_screen{ };

    vulkan_function_table vkft{ };
    std::unordered_set<std::string> instance_extensions{ };
    std::unordered_set<std::string> device_extensions{ };

    VkInstance instance{ };
    VkPhysicalDevice physical_device{ };
    u32 graphics_transfer_queue_family{  };
    u32 presentation_queue_family{ };
    VkDevice device{ };
    VkQueue graphics_transfer_queue{ };
    VkQueue presentation_queue{ };

    virtual ~context_impl() noexcept = default;
  };


  /**
   * @post ctx.x_connection is a valid pointer to an xcb_connection_t.
   * @post ctx.x_screen is a valid pointer to the default screen associated with the xcb_connection_t pointed to by
   *       ctx.x_connection.
   */
  iresult connect_to_x11(context_impl& ctx, const cstring displayname) noexcept;

  iresult get_instance_extensions(
    context_impl& ctx,
    const std::unordered_set<std::string>& layers,
    const std::unordered_set<std::string>& required_extensions,
    const std::unordered_set<std::string>& optional_extensions
  ) noexcept;

  /**
   * @pre ctx contains a vulkan_function_table with at least the global Vulkan functions.
   * @pre application_version is a 32-bit integer containing version data as-if packed by VK_MAKE_VERSION().
   * @pre layers is a const pointer to an array of strings only naming valid Vulkan layers currently available to the
   *      system.
   * @pre layer_count is not greater than the number of strings in layers.
   * @pre extensions is a const pointer to an array of strings only naming valid Vulkan instance extensions available
   *      to the system.
   * @pre extension_count is not greater than the number of strings in extensions.
   * @pre next is a valid extension chain for VkInstanceCreateInfo or nullptr.
   *
   * @post ctx.instance is a valid Vulkan handle that references a VkInstance.
   */
  iresult create_vulkan_instance(
    context_impl& ctx,
    const cstring application_name,
    const u32 application_version,
    const std::unordered_set<std::string>& layers,
    const readonly_ptr<void> next
  ) noexcept;

  iresult select_physical_device(
    context_impl& ctx,
    const std::unordered_set<std::string>& required_extensions,
    const std::unordered_set<std::string>& optional_extensions
  ) noexcept;

  iresult select_physical_device_queue_families(context_impl& ctx) noexcept;

  /**
   * @pre ctx contains a vulkan_function_table initialized with at least the required Vulkan instance functions.
   * @pre ctx contains a valid VkIntance handle.
   * @pre ctx contains a valid VkPhysicalDevice handle.
   * @pre ctx contains valid queue family indices for graphics and presentation.
   * @pre extensions is a const pointer to an array of strings only naming valid Vulkan device extensions available to
   *      the device driver.
   * @pre extension_count is not greater than the number of strings in extensions.
   * @pre features is a const pointer to a VkPhysicalDeviceFeatures structure where only available Vulkan device
   *      features have been enabled (set to true/1).
   * @pre queue_infos is a const pointer to an array of VkDeviceQueueCreateInfo structures describing the queue
   *      queue families indexed in ctx.
   * @pre queue_info_count is not greater than the number of structures in queue_infos.
   * @pre queue_info_count is 1 or 2.
   * @pre next is a valid extension chain for VkDeviceCreateInfo or nullptr.
   *
   * @post ctx contains a valid VkDevice handle.
   */
  iresult create_vulkan_device(context_impl& ctx, const readonly_ptr<void> next) noexcept;

  iresult get_device_queues(context_impl& ctx) noexcept;

  /**
   * @pre ctx contains a vulkan_function_table with at least the required Vulkan device functions loaded.
   * @pre ctx contains a valid VkDevice handle.
   *
   * @post ctx does not contain a valid VkDevice handle.
   */
  iresult destroy_vulkan_device(context_impl& ctx) noexcept;

  /**
   * @pre ctx contains a vulkan_function_table with at least the required Vulkan instance functions loaded.
   * @pre ctx contains a valid VkInstance handle.
   *
   * @post ctx does not contain a valid VkInstance handle.
   */
  iresult destroy_vulkan_instance(context_impl& ctx) noexcept;

  /**
   * @pre ctx contains a valid X11 connection pointer.
   * @pre ctx contains a valid X11 screen pointer.
   *
   * @post ctx does not contain a valid X11 connection pointer.
   * @post ctx does not contain a valid X11 screen pointer.
   */
  iresult disconnect_from_x11(context_impl& ctx) noexcept;
}
}

#endif
