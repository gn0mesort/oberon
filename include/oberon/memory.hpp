#ifndef OBERON_MEMORY_HPP
#define OBERON_MEMORY_HPP

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
}

#endif
