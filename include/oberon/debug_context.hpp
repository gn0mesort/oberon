#ifndef OBERON_DEBUG_CONTEXT_HPP
#define OBERON_DEBUG_CONTEXT_HPP

#include <unordered_set>
#include <string>

#include "context.hpp"

namespace oberon {
namespace detail {

  struct debug_context_impl;

}

  class debug_context final : public context {
  private:
    void v_dispose() noexcept override;
  public:
    debug_context(
      const std::string& application_name,
      const u16 application_version_major,
      const u16 application_version_minor,
      const u16 application_version_patch,
      const std::unordered_set<std::string>& requested_layers
    );

    ~debug_context() noexcept;
  };

}

#endif
