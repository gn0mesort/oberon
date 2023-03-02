/**
 * @file errors.cpp
 * @brief errors.hpp implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2022
 * @copyright AGPL-3.0+
 */
#include "oberon/errors.hpp"

#include <cstdarg>

namespace oberon {


  void check(const std::source_location& location, const bool condition, const i32 result,
             const cstring message_format, ...) {
    if (!condition)
    {
      // va_lists require this declaration style specifically
      std::va_list args;
      // Calculate and allocate space for message.
      va_start(args, message_format);
      std::va_list args_cp;
      va_copy(args_cp, args); // The next call will destroy args so we need another copy.
      auto sz = std::vsnprintf(nullptr, 0, message_format, args) + 1; // length + \0
      va_end(args);
      auto message = std::string(sz, '\0');
      std::vsnprintf(std::data(message), std::size(message), message_format, args_cp);
      va_end(args_cp);
      throw check_failed_error{ message, result, location };
    }
  }

}
