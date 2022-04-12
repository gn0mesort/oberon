#ifndef OBERON_MEMORY_HPP
#define OBERON_MEMORY_HPP

#include <memory>

#include "types.hpp"

#define OBERON_PIMPL_FWD(type) \
  struct type##_impl;\
  struct type##_impl_dtor final {\
    void operator()(const ptr<type##_impl> p) const noexcept;\
  }

#define OBERON_PIMPL_PTR(ns, type) \
  std::unique_ptr<ns::type##_impl, ns::type##_impl_dtor> m_impl{ }

#define OBERON_NON_OWNING_PIMPL_FWD(type) struct type##_impl

#define OBERON_NON_OWNING_PIMPL_PTR(ns, type) oberon::ptr<ns::type##_impl> m_impl{ }

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
