#include "oberon/errors.hpp"

#include <cstdlib>

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  simple_error::simple_error(const cstring msg, const std::source_location& loc) :
  m_message{ msg },
  m_location{ loc } { }

  simple_error::simple_error(const std::string& msg, const std::source_location& loc) :
  m_message{ msg },
  m_location{ loc } { }

  cstring simple_error::what() const noexcept {
    return std::data(m_message);
  }

  std::string_view simple_error::message() const noexcept {
    return { std::begin(m_message), std::end(m_message) };
  }

  iresult simple_error::result() const noexcept {
    return EXIT_FAILURE;
  }

  const std::source_location& simple_error::location() const noexcept {
    return m_location;
  }

}
}
