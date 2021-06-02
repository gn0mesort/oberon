#include "oberon/errors.hpp"

namespace oberon {
  critical_error::critical_error(const cstring message) noexcept : m_message{ message } { }
  
  critical_error::critical_error(const cstring message, const i32 result) noexcept : 
  m_message{ message }, m_result{ result } { }

  cstring critical_error::message() const noexcept {
    return m_message;
  }

  i32 critical_error::result() const noexcept {
    return m_result;
  }

  fatal_error::fatal_error(const std::string_view& message) : m_message{ message } { }
  
  fatal_error::fatal_error(const std::string_view& message, const i32 result) :
  m_message{ message }, m_result{ result } { }

  cstring fatal_error::message() const noexcept {
    return m_message.c_str();
  }

  i32 fatal_error::result() const noexcept {
    return m_result;
  }

  nonfatal_error::nonfatal_error(const std::string_view& message) : m_message{ message } { }

  nonfatal_error::nonfatal_error(const std::string_view& message, const i32 result) :
  m_message{ message }, m_result{ result } { }

  cstring nonfatal_error::message() const noexcept {
    return m_message.c_str();
  }

  i32 nonfatal_error::result() const noexcept {
    return m_result;
  }
}
