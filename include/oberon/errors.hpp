#ifndef OBERON_ERRORS_HPP
#define OBERON_ERRORS_HPP

#include <exception>
#include <string>
#include <string_view>
// clangd doesn't detect this header correctly with gcc in C++20 mode.
#if __has_include(<source_location>) && !defined(USING_CLANGD)
  #include <source_location>
#elif __has_include(<experimental/source_location>)
  #include <experimental/source_location>

  namespace std {

    using source_location = std::experimental::source_location;

  }
#else
  #error This header requires std::source_location.
#endif

#include "types.hpp"
#include "memory.hpp"

#define OBERON_STATIC_EXCEPTION_TYPE(name, msg, res) \
  class name final : public oberon::error { \
  private: \
    std::source_location m_srcloc{ }; \
    oberon::cstring m_message{ (msg) }; \
    oberon::i32 m_result{ (res) }; \
  public: \
    inline name(const std::source_location& srcloc = std::source_location::current()) : \
    m_srcloc{ srcloc } { } \
    inline oberon::cstring type() const noexcept override { return #name; } \
    inline oberon::cstring what() const noexcept override { return m_message; } \
    inline oberon::cstring message() const noexcept override { return m_message; } \
    inline oberon::i32 result() const noexcept override { return m_result; } \
    inline const std::source_location& location() const noexcept override { return m_srcloc; } \
  }

#define OBERON_DYNAMIC_EXCEPTION_TYPE(name) \
  class name final : public oberon::error { \
  private: \
    std::source_location m_srcloc{ }; \
    std::string m_message{ }; \
    oberon::i32 m_result{ }; \
  public: \
    inline name(const std::string_view message, const oberon::i32 result, \
                const std::source_location& srcloc = std::source_location::current()) : \
    m_srcloc{ srcloc }, m_message{ message }, m_result{ result } { }\
    inline oberon::cstring type() const noexcept override { return #name; } \
    inline oberon::cstring what() const noexcept override { return std::data(m_message); } \
    inline oberon::cstring message() const noexcept override { return std::data(m_message); } \
    inline oberon::i32 result() const noexcept override { return m_result; } \
    inline const std::source_location& location() const noexcept override { return m_srcloc; } \
  }

// Disable to skip invariant checks. Risky!
#if !defined(OBERON_INVARIANTS_ENABLED)
  #define OBERON_INVARIANTS_ENABLED 1
#endif

#if OBERON_INVARIANTS_ENABLED
  #define OBERON_INVARIANT_ERROR(exp, error) \
    do \
    { \
      if (!(exp)) \
      { \
        throw oberon::invariant_violated_error{ "Invariant \'" #exp "\' was violated.", (error) }; \
      } \
    } \
    while (0)
#else
  #define OBERON_INVARIANT_ERROR(exp, error) ((void) (exp))
#endif

#define OBERON_INVARIANT(exp) OBERON_INVARIANT_ERROR(exp, 1)

namespace oberon {

  class error : public std::exception {
  public:
    virtual ~error() noexcept = default;

    virtual cstring type() const noexcept = 0;
    virtual cstring message() const noexcept = 0;
    virtual i32 result() const noexcept = 0;
    virtual const std::source_location& location() const noexcept = 0;
  };

  // Static errors
  OBERON_STATIC_EXCEPTION_TYPE(not_implemented_error, "Functionality not implemented.", 1);

  // Dynamic errors
  OBERON_DYNAMIC_EXCEPTION_TYPE(runtime_error);
  OBERON_DYNAMIC_EXCEPTION_TYPE(invariant_violated_error);
  OBERON_DYNAMIC_EXCEPTION_TYPE(x11_error);
  OBERON_DYNAMIC_EXCEPTION_TYPE(vulkan_error);

}

#endif
