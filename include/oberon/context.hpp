#ifndef OBERON_CONTEXT_HPP
#define OBERON_CONTEXT_HPP

#include <string_view>

#include "object.hpp"

namespace oberon {
namespace detail {
  struct context_impl;
}
  class render_window;

  class context : public object {
  protected:
      context(const ptr<detail::context_impl> child_impl);
  public:
    static bool has_layer(const std::string_view& layer_name);
    static bool has_extension(const std::string_view& extension_name);
    static bool layer_has_extension(const std::string_view& layer_name, const std::string_view& extension_name);

    inline virtual ~context() noexcept = 0;
  };

  context::~context() noexcept { }
}

#endif
