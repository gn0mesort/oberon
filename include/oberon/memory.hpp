#ifndef OBERON_MEMORY_HPP
#define OBERON_MEMORY_HPP

#include <memory>

#include "macros.hpp"
#include "types.hpp"

namespace oberon {
OBERON_INLINE_V0_0 namespace v0_0 {

  template <typename Type>
  using ptr = Type*;

  template <typename Type, typename Deleter = std::default_delete<Type>>
  using unique = std::unique_ptr<Type, Deleter>;

  template <typename Type>
  using shared = std::shared_ptr<Type>;

  template <typename Type>
  using readonly_ptr = const Type*;

  template <typename CharType>
  using basic_cstring = readonly_ptr<CharType>;

  using      cstring = basic_cstring<char>;
  using     wcstring = basic_cstring<wchar>;
  using  utf8_string = basic_cstring<utf8>;
  using utf16_string = basic_cstring<utf16>;
  using utf32_string = basic_cstring<utf32>;

}
}

#endif
