#ifndef OBERON_ERRORS_HPP
#define OBERON_ERRORS_HPP

#include <exception>
#include <string>
#include <string_view>

#include "types.hpp"
#include "memory.hpp"

#define OBERON_STATIC_EXCEPTION_TYPE(name, msg, res) \
  class name##_error final : public oberon::error {\
  private:\
    oberon::cstring m_message{ (msg) };\
    oberon::i32 m_result{ (res) };\
  public:\
    inline oberon::cstring what() const noexcept override { return m_message; }\
    inline oberon::cstring message() const noexcept override { return m_message; }\
    inline oberon::i32 result() const noexcept override { return m_result; }\
  }

#define OBERON_DYNAMIC_EXCEPTION_TYPE(name) \
  class name##_error final : public oberon::error {\
  private:\
    std::string m_message{ };\
    oberon::i32 m_result{ };\
  public:\
    inline name##_error(const std::string_view message, const oberon::i32 result) :\
    m_message{ message }, m_result{ result } { }\
    inline oberon::cstring what() const noexcept override { return std::data(m_message); }\
    inline oberon::cstring message() const noexcept override { return std::data(m_message); }\
    inline oberon::i32 result() const noexcept override { return m_result; }\
  }

// Enable skipping invariant checks. Risky!
#if !defined(OBERON_BYPASS_INVARIANTS)
  #define OBERON_BYPASS_INVARIANTS 0
#endif

#if OBERON_BYPASS_INVARIANTS
  #define OBERON_INVARIANT(exp, error) ((void) (exp))
#else
  #define OBERON_INVARIANT(exp, error) \
    do\
    {\
      if (!(exp)) { throw (error); }\
    }\
    while (0)
#endif

namespace oberon {

  class error : public std::exception {
  public:
    virtual ~error() noexcept = default;

    virtual cstring message() const noexcept = 0;
    virtual i32 result() const noexcept = 0;
  };

  OBERON_STATIC_EXCEPTION_TYPE(not_implemented, "Functionality not implemented.", 1);

}

#endif
