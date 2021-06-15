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

  iresult validate_requested_layers(
    const debug_context_impl& ctx,
    const std::unordered_set<std::string>& requested_layers
  ) noexcept;

  iresult preload_debugging_context(
    const debug_context_impl& ctx,
    VkDebugUtilsMessengerCreateInfoEXT& debug_info,
    VkValidationFeaturesEXT& validation_features
  ) noexcept;

  iresult create_debug_messenger(
    debug_context_impl& ctx,
    const VkDebugUtilsMessengerCreateInfoEXT& debug_info
  ) noexcept;

  iresult destroy_debug_messenger(debug_context_impl& ctx) noexcept;

}
}

#endif
