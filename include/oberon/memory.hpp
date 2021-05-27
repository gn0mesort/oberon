#ifndef OBERON_MEMORY_HPP
#define OBERON_MEMORY_HPP

#include <memory>

#if __has_include(<propagate_const>)
  #include <propagate_const>
  #define OBERON_PROPAGATE_CONST std::propagate_const
#elif __has_include(<experimental/propagate_const>)
  #include <experimental/propagate_const>
  #define OBERON_PROPAGATE_CONST std::experimental::propagate_const
#endif

#include "types.hpp"

namespace oberon {
  template <typename Type>
  using ptr = Type*;

  template <typename Type>
  using readonly_ptr = const Type*;

  template <typename CharType>
  using basic_cstring = readonly_ptr<CharType>;

  using cstring = basic_cstring<char>;
  using wcstring = basic_cstring<wchar>;

#if defined(__WIN32)
  using tcstring = basic_cstring<TCHAR>;
#endif

#if defined(OBERON_PROPAGATE_CONST)
  template <typename Type, typename Deleter = std::default_delete<Type>>
  using d_ptr = OBERON_PROPAGATE_CONST<std::unique_ptr<Type, Deleter>>;
#else
  template <typename Type, typename Deleter = std::defaukt_delete>
  using d_ptr = std::unique_ptr<Type, Deleter>;
#endif
}

#if defined(OBERON_PROPAGATE_CONST)
  #undef OBERON_PROPAGATE_CONST
#endif

#endif
