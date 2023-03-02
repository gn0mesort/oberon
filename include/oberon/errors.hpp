/**
 * @file errors.hpp
 * @brief Base error types and error declaration macros.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2022
 * @copyright AGPL-3.0+
 */
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

/**
 * @def OBERON_STATIC_EXCEPTION_TYPE(name, msg, res)
 * @brief Declares a new exception with properties known at compile time.
 * @param name The name of the new exception type. This will be returned by `type()`.
 * @param msg The message that will be returned by `what()` and `message()`.
 * @param res The result code that will be returned by `result()`.
 */
#define OBERON_STATIC_EXCEPTION_TYPE(name, msg, res) \
  class name final : public oberon::error { \
  private: \
    std::source_location m_srcloc{ }; \
    oberon::cstring m_message{ (msg) }; \
    oberon::i32 m_result{ (res) }; \
  public: \
    inline name(const std::source_location& srcloc = std::source_location::current()) : \
    m_srcloc{ srcloc } { } \
    inline name(const name& other) noexcept = default; \
    inline oberon::cstring type() const noexcept override { return #name; } \
    inline oberon::cstring what() const noexcept override { return m_message; } \
    inline oberon::cstring message() const noexcept override { return m_message; } \
    inline oberon::i32 result() const noexcept override { return m_result; } \
    inline const std::source_location& location() const noexcept override { return m_srcloc; } \
  }

/**
 * @def OBERON_DYNAMIC_EXCEPTION_TYPE(name)
 * @brief Declares a new exception with properties determined at runtime.
 * @param name The name of the new exception type. This will be returned by `type()`.
 */
#define OBERON_DYNAMIC_EXCEPTION_TYPE(name) \
  class name final : public oberon::error { \
  private: \
    std::source_location m_srcloc{ }; \
    std::string m_message{ }; \
    oberon::i32 m_result{ }; \
  public: \
    inline name(const std::string message, const oberon::i32 result, \
                const std::source_location& srcloc = std::source_location::current()) : \
    m_srcloc{ srcloc }, m_message{ message }, m_result{ result } { } \
    inline name(const name& other) noexcept = default; \
    inline oberon::cstring type() const noexcept override { return #name; } \
    inline oberon::cstring what() const noexcept override { return m_message.data(); } \
    inline oberon::cstring message() const noexcept override { return m_message.data(); } \
    inline oberon::i32 result() const noexcept override { return m_result; } \
    inline const std::source_location& location() const noexcept override { return m_srcloc; } \
  }

// Disable this to skip runtime checks. Risky!
#if !defined(OBERON_CHECKS_ENABLED)
  /**
   * @def OBERON_CHECKS_ENABLED
   * @brief If defined to 0 then `OBERON_CHECK_ERROR_MSG` and `OBERON_CHECK` will be disabled. Otherwise checks are
   *        enabled.
   */
  #define OBERON_CHECKS_ENABLED 1
#endif

#if OBERON_CHECKS_ENABLED
  /**
   * @def OBERON_CHECK_ERROR_MSG(x, error, msg, ...)
   * @brief If checks are enabled then throw an error if `x` is false.
   * @param x An expression to check.
   * @param error A 32-bit signed integer error code to returned when `x` is false.
   * @param msg An error message explaining the potential error. This is a `std::printf`-style format string.
   * @param ... 0 or more format arguments corresponding to the format specifiers in `msg`.
   */
  #define OBERON_CHECK_ERROR_MSG(x, error, msg, ...) \
    (oberon::check(std::source_location::current(), (x), (error), (msg) __VA_OPT__(,) __VA_ARGS__))
#else
  #define OBERON_CHECK_ERROR_MSG(x, error, msg, ...) ((void) (exp))
#endif

/**
 * @def OBERON_CHECK(x)
 * @brief If checks are enabled then throw an error if `x` is false.
 * @param x An expression to check.
 */
#define OBERON_CHECK(x) (OBERON_CHECK_ERROR_MSG((x), 1, (#x)))

namespace oberon {

  /**
   * @brief Abstract base error type.
   */
  class error : public std::exception {
  public:
    /**
     * @brief Construct an `error`.
     */
    error() noexcept = default;

    /**
     * @brief Copy an `error`.
     * @param other The `error` to copy.
     */
    error(const error& other) noexcept = default;

    /**
     * @brief Destroy an `error`.
     */
    virtual ~error() noexcept = default;

    /**
     * @brief Get the type name of an `error`.
     * @returns The type name of the derived `error`.
     */
    virtual cstring type() const noexcept = 0;

    /**
     * @brief Get the message associated with an `error`.
     * @returns a message indicating the cause of the `error`.
     */
    virtual cstring message() const noexcept = 0;

    /**
     * @brief Get the result code associated with an `error`.
     * @returns a 32-bit signed integer suitable to be returned by `main`.
     */
    virtual i32 result() const noexcept = 0;

    /**
     * @brief Get the `std::source_location` at which an `error` was generated.
     * @returns A `std::source_location` indicating where the corresponding `error` ocurred.
     */
    virtual const std::source_location& location() const noexcept = 0;
  };

  /**
   * @brief An error caused by attempting to invoke functionality that hasn't been implemented.
   */
  OBERON_STATIC_EXCEPTION_TYPE(not_implemented_error, "This functionality is not implemented.", 1);

  /**
   * @brief An error caused by failing a runtime check.
   */
  OBERON_DYNAMIC_EXCEPTION_TYPE(check_failed_error);

  /**
   * @brief Runtime error check implmentation.
   * @param location A `std::source_location` object holding the location that `oberon::check` was called from.
   * @param condition The condition to check is true.
   * @param message_format A custom message to provide when the check failes. This is a `std::printf`-style format
   *        string.
   * @param ... 0 or more format arguments corresponding to the format specifiers in `message_format`.
   */
  void check(const std::source_location& location, const bool condition, const i32 result,
             const cstring message_format, ...);

}

#endif
