#ifndef OBERON_DEBUG_HPP
#define OBERON_DEBUG_HPP

#include <string_view>

#if __has_include(<source_location>)
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
  #define OBERON_ASSERT(x) \
    oberon::assert(std::source_location::current(), (x), (#x "\n"))

  #define OBERON_ASSERT_MSG(x, msg, ...) \
    oberon::assert(std::source_location::current(), (x), (msg) __VA_OPT__(,) __VA_ARGS__)
#else
  #define OOBERON_ASSERT(x) (void) ((x))

  #define OBERON_ASSERT_MSG(x, msg, ...) (void) ((x))
#endif

#define OBERON_PRECONDITION(x) \
  OBERON_ASSERT(x)

#define OBERON_POSTCONDITION(x) \
  OBERON_ASSERT(x)

#define OBERON_PRECONDITION_MSG(x, msg, ...) \
  OBERON_ASSERT(x, msg __VA_OPT__(,) __VA_ARGS__)

#define OBERON_POSTCONDITION_MSG(x, msg, ...) \
  OBERON_ASSERT(x, msg, __VA_OPT__(,), __VA_ARGS__)

namespace oberon {

  void assert(const std::source_location& location, const bool condition, const std::string_view& message_format, ...);

}

#endif
