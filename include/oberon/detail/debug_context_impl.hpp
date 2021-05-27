#ifndef OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP
#define OBERON_DETAIL_DEBUG_CONTEXT_IMPL_HPP

#include "../debug_context.hpp"

#include "graphics.hpp"

namespace oberon {
namespace detail {
  struct debug_context_impl final {
    bool has_validation_features{ false };

    ptr<xcb_connection_t> x_connection{ };
    ptr<xcb_screen_t> x_screen{ };

    vk::DispatchLoaderDynamic dl{ };
    vk::Instance instance{ };
    vk::DebugUtilsMessengerEXT debug_messenger{ };
  };
}
}

#endif
