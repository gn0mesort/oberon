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

  std::vector<cstring> debug_context_select_layers(
    debug_context_impl& ctx,
    const std::unordered_set<std::string_view>& requested_layers
  );
  std::unordered_set<std::string> debug_context_fetch_extensions(debug_context_impl& ctx, const cstring layer);
  std::vector<cstring> debug_context_select_extensions(
    debug_context_impl& ctx,
    const std::unordered_set<std::string>& available_extensions
  );
  void debug_context_load_instance_extension_pfns(debug_context_impl& ctx);
  std::vector<VkPhysicalDevice> debug_context_fetch_physical_devices(
    debug_context_impl& ctx,
    const std::unordered_set<std::string_view>& required_extension
  );
  void debug_context_select_physical_device_queue_families(debug_context_impl& ctx);
  void debug_context_fill_queue_create_info(
    u32 queue_family_index,
    VkDeviceQueueCreateInfo& queue_info, float& priority
  );
}
}

#endif
