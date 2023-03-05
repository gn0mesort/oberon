/**
 * @file x11-errors.cpp
 * @brief X11 error handling implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/x11-errors.hpp"

namespace oberon::linux {

#define OBERON_LINUX_X_ERROR(name, msg, code) case x_error_code::name: return (msg);
  std::string to_string(const x_error_code code) {
    switch (code)
    {
    OBERON_LINUX_X_ERRORS
    default:
      return "An unknown X11 error occurred.";
    }
  }
#undef OBERON_LINUX_X_ERROR

}
