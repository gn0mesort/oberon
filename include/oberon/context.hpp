#ifndef OBERON_CONTEXT_HPP
#define OBERON_CONTEXT_HPP

#include <string_view>

namespace oberon {
  class context {
  public:
    static bool has_layer(const std::string_view& layer_name);
    static bool has_extension(const std::string_view& extension_name);
    static bool layer_has_extension(const std::string_view& layer_name, const std::string_view& extension_name);

    inline virtual ~context() noexcept = 0;
  };

  context::~context() noexcept { }
}

#endif
