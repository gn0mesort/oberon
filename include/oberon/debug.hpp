#ifndef OBERON_DEBUG_HPP
#define OBERON_DEBUG_HPP

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

#if !defined(NDEBUG)
  #if !defined(OBERON_BYPASS_ASSERTIONS)
    #define OBERON_BYPASS_ASSERTIONS 0
  #endif
#else
  #if !defined(OBERON_BYPASS_ASSERTIONS)
    #define OBERON_BYPASS_ASSERTIONS 1
  #endif
#endif

#if OBERON_BYPASS_ASSERTIONS
  #define OBERON_ASSERT(x) ((void) (x))

  #define OBERON_ASSERT_MSG(x, msg, ...) ((void) (x))
#else
  #define OBERON_ASSERT(x) \
    oberon::assert(std::source_location::current(), (x), (#x))

  #define OBERON_ASSERT_MSG(x, msg, ...) \
    oberon::assert(std::source_location::current(), (x), (msg) __VA_OPT__(,) __VA_ARGS__)
#endif

#define OBERON_PRECONDITION(x) \
  OBERON_ASSERT(x)

#define OBERON_POSTCONDITION(x) \
  OBERON_ASSERT(x)

#define OBERON_PRECONDITION_MSG(x, msg, ...) \
  OBERON_ASSERT_MSG(x, msg __VA_OPT__(,) __VA_ARGS__)

#define OBERON_POSTCONDITION_MSG(x, msg, ...) \
  OBERON_ASSERT_MSG(x, msg, __VA_OPT__(,) __VA_ARGS__)

namespace oberon {

  void assert(const std::source_location& location, const bool condition, const std::string_view message_format, ...);

}

#endif
