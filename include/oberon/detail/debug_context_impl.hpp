#ifndef OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP
#define OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP

#include "../debug_context.hpp"

#include <unordered_set>

#include "context_impl.hpp"
#include "../debug.hpp"

namespace oberon {
namespace detail {

  struct debug_context_impl final : public context_impl {
    VkDebugUtilsMessengerEXT debug_messenger{ };
  };

  /**
   * Validate that all requested Vulkan layers are available.
   *
   * @param ctx A context loaded with at least a valid implementation of vkEnumerateInstanceLayers().
   * @param requested_layers A set of extension names to validate.
   *
   * @return 0 if all layers are available. Otherwise -1.
   */
  iresult validate_requested_layers(
    const debug_context_impl& ctx,
    const std::unordered_set<std::string>& requested_layers
  ) noexcept;

  /**
   * Prepare VkDebugUtilsMessengerCreateInfoEXT and VkValidationFeaturesEXT structures for use during Vulkan instance
   * creation.
   *
   * If VK_EXT_validation_features is available this will prepare validation_features.pNext as an extension pointer
   * chain for a VkInstanceCreateInfo structure. The chain will be:
   *
   * VkValidationFeaturesEXT -> VkDebugUtilsMessengerCreateInfoEXT
   *
   * @param ctx A context for use in preparing debugging information. This *must* contain a set of valid instance
   *            extensions.
   *
   * @return 0 in all valid cases.
   */
  iresult preload_debugging_context(
    const debug_context_impl& ctx,
    VkDebugUtilsMessengerCreateInfoEXT& debug_info,
    VkValidationFeaturesEXT& validation_features
  ) noexcept;

  /**
   * Create a VkDebugUtilsMessengerEXT and store it into ctx.
   *
   * @param ctx A context to store A debug messenger handle into. This *must* be prepared with a valid Vulkan instance
   *            handle and the VK_EXT_debug_utils extension *must* be enabled. Additionally, the corresponding Vulkan
   *            functions *must* be loaded into ctx.vkft.
   *
   * @return 0 on success. Otherwise a corresponding VkResult explaining why debug messenger creation failed.
   */
  iresult create_debug_messenger(
    debug_context_impl& ctx,
    const VkDebugUtilsMessengerCreateInfoEXT& debug_info
  ) noexcept;

  /**
   * Destroy a VkDebugUtilsMessengerEXT stored in ctx.
   *
   * If no VkDebugUtilsMessengerEXT is stored in ctx this function immediately returns.
   *
   * @param ctx A context containing a debug messenger to destroy. If ctx.debug_messenger is not empty then this *must*
   *            be prepared with a valid Vulkan instance and the VK_EXT_debug_utils extension *must* be enabled.
   *            Additionally, the corresponding Vulkan functions *must* be loaded into ctx.vkft.
   */
  iresult destroy_debug_messenger(debug_context_impl& ctx) noexcept;

}
}

#endif
