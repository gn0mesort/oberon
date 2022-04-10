#include "oberon/debug.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include <string>

namespace oberon {
  void assert(const std::source_location& location, const bool condition, const std::string_view message_format, ...) {
    if (!condition)
    {
      // va_lists require this declaration style specifically
      std::va_list args;
      // Calculate and allocate space for message.
      va_start(args, message_format);
      std::va_list args_cp;
      va_copy(args_cp, args); // The next call will destroy args so we need another copy.
      auto sz = std::vsnprintf(nullptr, 0, std::data(message_format), args) + 1; // length + \0
      va_end(args);
      auto message = std::string(sz, '\0');
      std::vsnprintf(std::data(message), std::size(message), std::data(message_format), args_cp);
      va_end(args_cp);
      std::fprintf(stderr, "%s:%" PRIu32 ": Assertion failed \"%s\"\n", location.file_name(), location.line(),
                   std::data(message));
      std::abort();
    }
  }
}
