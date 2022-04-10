#ifndef OBERON_ERRORS_HPP
#define OBERON_ERRORS_HPP

#include <exception>

#include "types.hpp"
#include "memory.hpp"

#define OBERON_EXCEPTION_TYPE(name, msg, res) \
  class name##_error final : public oberon::error {\
  private:\
    oberon::cstring m_message{ (msg) };\
    oberon::i32 m_result{ (res) };\
  public:\
    inline oberon::cstring what() const noexcept override { return m_message; }\
    inline oberon::cstring message() const noexcept override { return m_message; }\
    inline oberon::i32 result() const noexcept override { return m_result; }\
  }

#define OBERON_CHECK(exp, status, error) \
  do\
  {\
    if (auto res = (exp); res != (status)) { throw error{ }; }\
  }\
  while (0)

namespace oberon {

  class error : public std::exception {
  public:
    virtual ~error() noexcept = default;

    virtual cstring message() const noexcept = 0;
    virtual i32 result() const noexcept = 0;
  };

  OBERON_EXCEPTION_TYPE(not_implemented, "Functionality not implemented.", 1);

}

#endif
