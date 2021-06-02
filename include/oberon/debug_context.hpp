#ifndef OBERON_DEBUG_CONTEXT_HPP
#define OBERON_DEBUG_CONTEXT_HPP

#include <unordered_set>
#include <string_view>

#include "context.hpp"

namespace oberon {
namespace detail {
  struct debug_context_impl;
}
  class debug_context final : public context {
  public:
    debug_context(
      const std::string_view& application_name,
      const u16 application_version_major,
      const u16 application_version_minor,
      const u16 application_version_patch,
      const std::unordered_set<std::string_view>& requested_layers
    );

    ~debug_context() noexcept;
  };
}

#endif
