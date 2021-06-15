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
   * Connects to the X11 server described by displayname.
   *
   * This will store the X11 connection and screen information in ctx.
   *
   * @param ctx The context to store connection information into.
   * @param displayname A C style string describing the X11 server to connect to (e.g. ":0.0") or nullptr.
   *                    If this is null then the system default X11 server string will be used (the value of the
   *                    DISPLAY environment variable).
   *
   * @return 0 on success. -1 if there was an error connecting to the server.
   */
  iresult connect_to_x11(context_impl& ctx, const cstring displayname) noexcept;

  /**
   * Stores the intersection of required_extensions, optional_extensions, and the set of available Vulkan instance
   * extensions in ctx.
   *
   * @param ctx The context to store extensions in.
   * @param layers A set of Vulkan layer names to gather additional extension information from.
   *               Extensions made available through layers in this set are included as if they were available
   *               directly from the Vulkan instance. No effort is made to validate that the layers named in this set
   *               are valid or available. Layer names *must* be validated before being passed to this function.
   * @param required_extensions A set of required extension names. If one or more of these extensions are not available
   *                            this function will return an error.
   * @param optional_extensions A set of optional extension names. It is not an error for these extensions to not be
   *                            available.
   *
   * @return 0 on success. -1 if any of the required extensions were unavailable.
   */
  iresult get_instance_extensions(
    context_impl& ctx,
    const std::unordered_set<std::string>& layers,
    const std::unordered_set<std::string>& required_extensions,
    const std::unordered_set<std::string>& optional_extensions
  ) noexcept;

  /**
   * Creates an new Vulkan instance and stores the resulting handle in ctx.
   *
   * @param ctx The context to store the Vulkan instance handle in.
   * @param application_name An application name for use in instance creation. This can be null and only matters if
   *                         the Vulkan driver contains application specific optimizations for your your application.
   * @param application_version A packed Vulkan version number for your application. This should be packed as if by
   *                            VK_MAKE_VERSION(major, minor, patch). As with the application name this is only
   *                            relevant for application specific driver optimizations and can safely be set to 0.
   * @param layers A set of Vulkan layers to explicitly load when creating the instance. If this is not empty all of
   *               the contained layer names *must* be validated externally for the current system.
   * @param next An extension pointer chain for use in instance creation. This is used to extend the
   *             VkInstanceCreateInfo structure and *must* be a valid pointer chain for such a structure.
   *
   * @return 0 on success. Otherwise the corresponding VkResult indicating why instance creation failed.
   */
  iresult create_vulkan_instance(
    context_impl& ctx,
    const cstring application_name,
    const u32 application_version,
    const std::unordered_set<std::string>& layers,
    const readonly_ptr<void> next
  ) noexcept;

  /**
   * Select a physical device from the available Vulkan devices and store it in ctx.
   *
   * This function succeeds only when at least one available device has all of the following properties:
   *   - At least one queue family capable of graphics (and transfer) operations.
   *   - At least one queue family capable of presentation operations.
   *   - All of the required device extensions (as found in required_extensions) are available.
   *
   * @param ctx The context to store the selected physical device into. This *must* be a context with a prepared
   *            Vulkan instance handle *and* the corresponding Vulkan functions loaded into ctx.vkft.
   * @param required_extensions A set of required device extension names.
   * @param optional_extensions A set of optional device extension names.
   *
   * @return 0 on success. -1 if no physical devices were found with the minimum required features.
   */
  iresult select_physical_device(
    context_impl& ctx,
    const std::unordered_set<std::string>& required_extensions,
    const std::unordered_set<std::string>& optional_extensions
  ) noexcept;


  /**
   * Selects queue families for use in device creation and stores the selected information into ctx.
   *
   * This will select a single queue family for all operations if possible. Otherwise two queue families (one for
   * graphics/transfer and one for presentation) will be selected instead.
   *
   * @param ctx A context to store selected queue family information into. This *must* be a context with prepared
   *            instance and physical_device handles. Additionally this context *must* have the corresponding Vulkan
   *            instance functions loaded into ctx.vkft.
   *
   * @return 0 in all valid cases.
   */
  iresult select_physical_device_queue_families(context_impl& ctx) noexcept;

  /**
   * Creates a new Vulkan device and stores the resulting handle in ctx.
   *
   * @param ctx A context to store the newly created Vulkan device handle in. This *must* be a context with prepared
   *            instance and physical_device handles. It *must* also have queue families selected. Finally, it *must*
   *            have the corresponding Vulkan instance functions loaded into ctx.vkft.
   * @param next An extension pointer chain for use in device creation. This is used to extend the VkDeviceCreateInfo
   *             structure and *must* be a valid pointer chain for such a structure.
   *
   * @return 0 on success. Otherwise a corresponding VkResult indicating why device creation failed.
   */
  iresult create_vulkan_device(context_impl& ctx, const readonly_ptr<void> next) noexcept;


  /**
   * Retrieves Vulkan device queues corresponding to the selected Vulkan device queue families and stores them in ctx.
   *
   * Only one queue of each queue family is retrieved (the queue with index 0). If both queue families are the same
   * then they are interchangeable.
   *
   * @param ctx The context to retrieve the corresponding queues of.
   *
   * @return 0 in all valid cases.
   */
  iresult get_device_queues(context_impl& ctx) noexcept;

  /**
   * Destroy the Vulkan device stored in ctx.
   *
   * If ctx.device is not prepared nothing will be done.
   *
   * @param ctx A context containing the Vulkan device handle to destroy. If ctx.device is not empty then this *must*
   *            be a context prepared with valid device, and instance handles. Additionally, it *must* have the
   *            corresponding Vulkan functions loaded into ctx.vkft.
   *
   * @return 0 in all valid cases.
   */
  iresult destroy_vulkan_device(context_impl& ctx) noexcept;

  /**
   * Destroy the Vulkan instance stored in ctx.
   *
   * If ctx.instance is not prepared nothing will be done.
   *
   * @param ctx A context containing the Vulkan instance handle to destroy. If ctx.instance is not empty then this
   *            *must* be a context prepared with a valid instance handle. Additionally, it *must* have the
   *            corresponding Vulkan functions loaded into ctx.vkft.
   *
   * @return 0 in all valid cases.
   */
  iresult destroy_vulkan_instance(context_impl& ctx) noexcept;

  /**
   * Disconnect the X11 server connection stored in ctx.
   *
   * If ctx.x11_connection is not prepared nothing will be done.
   *
   * @param ctx A context containing the X11 server connection to disconnect from.
   *
   * @return 0 in all valid cases.
   */
  iresult disconnect_from_x11(context_impl& ctx) noexcept;
}
}

#endif
