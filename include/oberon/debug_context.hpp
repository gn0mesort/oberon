#ifndef OBERON_DEBUG_CONTEXT_HPP
#define OBERON_DEBUG_CONTEXT_HPP

#include <unordered_set>
#include <string>
#include <tuple>

#include "memory.hpp"
#include "context.hpp"

namespace oberon {
namespace detail {
  struct debug_context_impl;
  struct debug_context_dtor final {
    void operator()(const ptr<debug_context_impl> impl) const noexcept;
  };
}
  class debug_context final : public context {
  private:
    d_ptr<detail::debug_context_impl, detail::debug_context_dtor> m_impl{ };

    static std::pair<bool, std::string_view> validate_layers(const std::unordered_set<std::string_view>& layers);
    static std::pair<bool, std::string_view> validate_required_extensions(
      const std::unordered_set<std::string_view>& layers
    );
    static bool has_extension(
      const std::unordered_set<std::string_view>& layers,
      const std::string_view& extension_name
    );
  public:
    static const std::unordered_set<std::string>& required_extensions();
    static const std::unordered_set<std::string>& optional_extensions();
    static const std::unordered_set<std::string>& required_device_extensions();
    static const std::unordered_set<std::string>& optional_device_extensions();

    debug_context(const std::unordered_set<std::string_view>& requested_layers);

    ~debug_context() noexcept;
  };
}

#endif
