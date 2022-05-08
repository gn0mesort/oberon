#ifndef OBERON_MEMORY_HPP
#define OBERON_MEMORY_HPP

#include <memory>

#include "types.hpp"

#define OBERON_OPAQUE_BASE_FWD(type) \
  namespace oberon::detail { \
    class type##_impl;\
    struct type##_impl_dtor final {\
      void operator()(const ptr<type##_impl> p) const noexcept;\
    };\
  }\
  static_assert(true)

#define OBERON_OPAQUE_BASE_PTR(type) \
  std::unique_ptr<oberon::detail::type##_impl, oberon::detail::type##_impl_dtor> m_impl{ }

namespace oberon {

  template <typename Type>
  using ptr = Type*;

  template <typename Type>
  using readonly_ptr = const Type*;

  template <typename CharType>
  using basic_cstring = readonly_ptr<CharType>;

  using       cstring = basic_cstring<char>;
  using  utf8_cstring = basic_cstring<utf8>;
  using utf16_cstring = basic_cstring<utf16>;
  using utf32_cstring = basic_cstring<utf32>;
  using      wcstring = basic_cstring<wchar>;

}

#endif
