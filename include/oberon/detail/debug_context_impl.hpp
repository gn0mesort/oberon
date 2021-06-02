#ifndef OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP
#define OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP

#include "../debug_context.hpp"

#include "context_impl.hpp"
#include "vulkan_function_table.hpp"

namespace oberon {
namespace detail {
  struct debug_function_table final : public vulkan_function_table {
    // Instance extensions
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT{ };
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR{ };
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{ };

    // Device extensions
  };

  struct debug_context_impl final : public context_impl {
    debug_function_table dbgft{ };
    std::unordered_map<std::string_view, bool> optional_extensions{ };
    VkDebugUtilsMessengerEXT debug_messenger{ };
  };

  /**
   * @pre ctx contains a vulkan_function_table initialized with at least the global Vulkan functions.
   * @pre The lifetime of requested_layers will exceed the lifetime of the returned std::vector<cstring>.
   */
  std::vector<cstring> debug_context_select_layers(
    debug_context_impl& ctx,
    const std::unordered_set<std::string_view>& requested_layers
  );

  /**
   * @pre ctx contains a vulkan_function_table initialized with at least the global Vulkan functions.
   * @pre layer is a pointer to string returned by debug_context_select_layers.
   */
  std::unordered_set<std::string> debug_context_fetch_extensions(debug_context_impl& ctx, const cstring layer);

  /**
   * @pre ctx contains a vulkan_function_table initialized with at least the global Vulkan functions.
   * @pre available_extensions is a set of std::strings returned as-if by debug_context_fetch_extensions().
   */
  std::vector<cstring> debug_context_select_extensions(
    debug_context_impl& ctx,
    const std::unordered_set<std::string>& available_extensions
  );


  /**
   * @pre ctx contains a debug_function_table initialized with at least the global Vulkan functions.
   * @pre ctx contains a valid VkInstance handle.
   *
   * @post ctx contains a debug_function_table initialized with at least the required Vulkan instance extension
   *       functions.
   */
  void debug_context_load_instance_extension_pfns(debug_context_impl& ctx);

  /**
   * @pre ctx contains a vulkan_function_table initialized with at least the required Vulkan instance functions.
   * @pre ctx contains a valid VkInstance handle.
   * @pre required_extensions is a set of valid Vulkan device extension names.
   */
  std::vector<VkPhysicalDevice> debug_context_fetch_physical_devices(
    debug_context_impl& ctx,
    const std::unordered_set<std::string_view>& required_extensions
  );

  /**
   * @pre ctx contains a debug_function_table initialized with at least the required Vulkan instance extension
   *      functions.
   * @pre ctx contains a valid VkInstance handle.
   * @pre ctx contains a valid VkPhysicalDevice handle.
   *
   * @post ctx contains a valid queue family index for both graphics and presentation operations.
   */
  void debug_context_select_physical_device_queue_families(debug_context_impl& ctx);

  /**
   * @pre queue_family_index is a valid queue family index from a valid context.
   * @pre priority is a number between 0.0f and 1.0f.
   * @pre the lifetime of priority exceeds the lifetime of queue_info.
   *
   * @post queue_info is overwritten with valid parameters for the creation of a queue_family_index VkQueue with a
   *       priority of priority
   */
  void debug_context_fill_queue_create_info(
    u32 queue_family_index,
    VkDeviceQueueCreateInfo& queue_info, float& priority
  );
}
}

#endif
