/**
 * @file debug.hpp
 * @brief Debugging utilities.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2022
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_DEBUG_HPP
#define OBERON_DEBUG_HPP

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

#include "memory.hpp"

#if !defined(OBERON_ASSERTIONS_ENABLED)
  #if !defined(NDEBUG)
    /**
     * @def OBERON_ASSERTIONS_ENABLED
     * @brief If defined to 0 this disables assertions. Otherwise assertions are enabled.
     */
    #define OBERON_ASSERTIONS_ENABLED 1
  #else
    #define OBERON_ASSERTIONS_ENABLED 0
  #endif
#endif

#if OBERON_ASSERTIONS_ENABLED
  /**
   * @def OBERON_ASSERT(x)
   * @brief If assertions are enabled check the condition `x` and abort if `x` is `false`.
   * @param x A condition to check.
   */
  #define OBERON_ASSERT(x) \
    (oberon::debug_assert(std::source_location::current(), (x), (#x)))

  /**
   * @def OBERON_ASSERT_MSG(x, msg, ...)
   * @brief If assertions are enabled check the condition `x` and abort with a custom message if `x` is `false`.
   * @param x A condition to check.
   * @param msg A custom message to print if the assertion fails. This is a `std::printf`-style format string.
   * @param ... 0 or more format arguments corresponding to the format specifiers in `msg`.
   */
  #define OBERON_ASSERT_MSG(x, msg, ...) \
    (oberon::debug_assert(std::source_location::current(), (x), (msg) __VA_OPT__(,) __VA_ARGS__))
#else
  #define OBERON_ASSERT(x) ((void) (x))

  #define OBERON_ASSERT_MSG(x, msg, ...) ((void) (x))
#endif

/**
 * @def OBERON_PRECONDITION(x)
 * @brief If assertions are enabled check the condition `x` and abort if `x` is `false`.
 * @param x A condition to check.
 */
#define OBERON_PRECONDITION(x) \
  (OBERON_ASSERT(x))

/**
 * @def OBERON_POSTCONDITION(x)
 * @brief If assertions are enabled check the condition `x` and abort if `x` is `false`.
 * @param x A condition to check.
 */
#define OBERON_POSTCONDITION(x) \
  (OBERON_ASSERT(x))

/**
 * @def OBERON_PRECONDITION_MSG(x, msg, ...)
 * @brief If assertions are enabled check the condition `x` and abort with a custom message if `x` is `false`.
 * @param x A condition to check.
 * @param msg A custom message to print if the assertion fails. This is a `std::printf`-style format string.
 * @param ... 0 or more format arguments corresponding to the format specifiers in `msg`.
 */
#define OBERON_PRECONDITION_MSG(x, msg, ...) \
  (OBERON_ASSERT_MSG(x, msg __VA_OPT__(,) __VA_ARGS__))

/**
 * @def OBERON_POSTCONDITION_MSG(x, msg, ...)
 * @brief If assertions are enabled check the condition `x` and abort with a custom message if `x` is `false`.
 * @param x A condition to check.
 * @param msg A custom message to print if the assertion fails. This is a `std::printf`-style format string.
 * @param ... 0 or more format arguments corresponding to the format specifiers in `msg`.
 */
#define OBERON_POSTCONDITION_MSG(x, msg, ...) \
  (OBERON_ASSERT_MSG(x, msg, __VA_OPT__(,) __VA_ARGS__))

namespace oberon {

  /**
   * @brief Query whether or not the current build is a debug build.
   * @details This is the same as checking for `NDEBUG`. This doesn't check whether, for example, the library
   *          was built with debugging enabled.
   * @return True if the current build is a debug build. Otherwise false.
   */
  consteval bool is_debug_build() {
#ifndef NDEBUG
    return true;
#else
    return false;
#endif
  }

  /**
   * @brief Debug assertion implementation.
   * @param location A `std::source_location` object holding the location that `oberon::debug_assert` was called from.
   * @param condition The condition to assert must be true.
   * @param message_format A custom message to print if the assertion fails. This is a `std::printf`-style format
   *        string.
   * @param ... 0 or more format arguments corresponding to the format specifiers in `message_format`.
   */
  void debug_assert(const std::source_location& location, const bool condition, const cstring message_format, ...);

}

#endif
