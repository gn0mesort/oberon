#ifndef OBERON_DETAIL_CONTEXT_IMPL_HPP
#define OBERON_DETAIL_CONTEXT_IMPL_HPP

#include "../context.hpp"

#include "object_impl.hpp"

#include "graphics.hpp"
#include "vulkan_function_table.hpp"

namespace oberon {
namespace detail {
  struct context_impl : public object_impl { 
    ptr<xcb_connection_t> x_connection{ };
    ptr<xcb_screen_t> x_screen{ };

    PFN_vkGetInstanceProcAddr loader{ };
    ptr<vulkan_function_table> vkft{ };
    VkInstance instance{ };
    VkPhysicalDevice physical_device{ };
    u32 graphics_transfer_queue_family{ };
    u32 presentation_queue_family{ };
    VkDevice device{ };
    VkQueue graphics_transfer_queue{ };
    VkQueue presentation_queue{ };

    inline virtual ~context_impl() noexcept = 0;
  };

  context_impl::~context_impl() noexcept { }

  /**
   * @post ctx.x_connection is a valid pointer to an xcb_connection_t.
   * @post ctx.x_screen is a valid pointer to the default screen associated with the xcb_connection_t pointed to by
   *       ctx.x_connection.
   */
  void context_initialize_x(context_impl& ctx, const cstring displayname);

  /**
   * @post ctx.loader is a valid pointer to an implementation of vkGetInstanceProcAddr().
   */
  void context_load_vulkan_library(context_impl& ctx);

  /**
   * @pre ctx.vkft is a valid pointer to an implementation of vulkan_function_table.
   *
   * @post ctx.vkft contains valid pointers to implementations of the 5 globally available Vulkan functions.
   */
  void context_load_global_pfns(context_impl& ctx);
 
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
  void context_initialize_vulkan_instance(
    context_impl& ctx,
    const cstring application_name,
    const u32 application_version,
    const readonly_ptr<cstring> layers,
    const u32 layer_count,
    const readonly_ptr<cstring> extensions,
    const u32 extension_count,
    const readonly_ptr<void> next
  );

  /**
   * @pre ctx contains a vulkan_function_table with at least the global Vulkan functions.
   * @pre ctx contains a valid VkInstance handle.
   *
   * @post ctx contains a vulkan_function_table with at least the required Vulkan instance functions loaded.
   */
  void context_load_instance_pfns(context_impl& ctx);

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
  void context_initialize_vulkan_device(
    context_impl& ctx,
    const readonly_ptr<cstring> extensions,
    const u32 extension_count,
    const readonly_ptr<VkPhysicalDeviceFeatures> features,
    const readonly_ptr<VkDeviceQueueCreateInfo> queue_infos,
    const u32 queue_info_count,
    const readonly_ptr<void> next
  );

  /**
   * @pre ctx contains a vulkan_function_table with at least the required Vulkan instance functions loaded.
   * @pre ctx contains a valid VkDevice handle.
   *
   * @post ctx contains a vulkan_function_table with at least the required Vulkan device functions loaded.
   */
  void context_load_device_pfns(context_impl& ctx);

#define OBERON_GLOBAL_FN(ctx, name) \
  (context_load_global_fn<PFN_##name>((ctx), (#name)))

  /**
   * @pre ctx contains a valid vkGetInstanceProcAddr implementation pointer.
   */
  template <typename FunctionPointer>
  FunctionPointer context_load_global_fn(const context_impl& ctx, const cstring name) {
    auto pfn = reinterpret_cast<FunctionPointer>(ctx.loader(nullptr, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load global function: \"" + std::string{ name } + "\"." };
    }
    return pfn;
  }

#define OBERON_INSTANCE_FN(ctx, name) \
  (context_load_instance_fn<PFN_##name>((ctx), (#name)))

  /**
   * @pre ctx contains a valid vkGetInstanceProcAddr implementation pointer.
   * @pre ctx contains a valid VkInstance handle.
   */
  template <typename FunctionPointer>
  FunctionPointer context_load_instance_fn(const context_impl& ctx, const cstring name) {
    if (!ctx.instance)
    {
      throw fatal_error{ "Cannot load function because the Vulkan instance is not valid." };
    }
    auto pfn = reinterpret_cast<FunctionPointer>(ctx.loader(ctx.instance, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load instance function: \"" + std::string{ name } + "\"." }; 
    }
    return pfn;
  }

#define OBERON_DEVICE_FN(ctx, name) \
  (context_load_device_fn<PFN_##name>((ctx), (#name)))

  /**
   * @pre ctx contains a vulkan_function_table with at least the required Vulkan instance functions loaded.
   * @pre ctx contains a valid VkDevice handle.
   */
  template <typename FunctionPointer>
  FunctionPointer context_load_device_fn(const context_impl& ctx, const cstring name) {
    if (!ctx.device)
    {
      throw fatal_error{ "Cannot load function because the Vulkan device is not valid." };
    }
    auto pfn = reinterpret_cast<FunctionPointer>(ctx.vkft->vkGetDeviceProcAddr(ctx.device, name));
    if (!pfn)
    {
      throw fatal_error{ "Failed to load device function: \"" + std::string{ name } + "\"." };
    }
    return pfn;
  }

  /**
   * @pre ctx contains a vulkan_function_table with at least the required Vulkan device functions loaded.
   * @pre ctx contains a valid VkDevice handle.
   *
   * @post ctx does not contain a valid VkDevice handle.
   */
  void context_deinitialize_vulkan_device(context_impl& ctx);

  /**
   * @pre ctx contains a vulkan_function_table with at least the required Vulkan instance functions loaded.
   * @pre ctx contains a valid VkInstance handle.
   *
   * @post ctx does not contain a valid VkInstance handle.
   */
  void context_deinitialize_vulkan_instance(context_impl& ctx);

  /**
   * @pre ctx contains a valid X11 connection pointer.
   * @pre ctx contains a valid X11 screen pointer.
   *
   * @post ctx does not contain a valid X11 connection pointer.
   * @post ctx does not contain a valid X11 screen pointer.
   */
  void context_deinitialize_x(context_impl& ctx);
}
}

#endif
