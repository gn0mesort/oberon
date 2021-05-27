#ifndef OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP
#define OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP

#include "../debug_context.hpp"

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.hpp>

namespace oberon {
namespace detail {
  struct debug_context_impl final {
    bool has_validation_features{ false };
    vk::DispatchLoaderDynamic dl{ };
    vk::Instance instance{ };
    vk::DebugUtilsMessengerEXT debug_messenger{ };
  };
}
}

#endif
