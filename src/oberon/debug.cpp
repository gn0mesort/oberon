#include "oberon/debug.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include <string>

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  void assert(const std::source_location& location, const bool condition,
              const std::string_view& message_format, ...) {
    if (!condition)
    {
      std::va_list args; // This is a special weirdo declaration.
      // Calculate and allocate space for message.
      auto sz = std::vsnprintf(nullptr, 0, std::data(message_format), args);
      auto message = std::string(sz, '\0');
      std::vsnprintf(std::data(message), std::size(message), std::data(message_format), args);
      std::fprintf(
        stderr,
        "%s:%" PRIu32 ": Assertion failed \"%s\"\n",
        location.file_name(), location.line(), std::data(message)
      );
      std::abort();
    }
  }

}
}
