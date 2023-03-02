/**
 * @file memory.hpp
 * @brief Pointer type aliases and support macros.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2022
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_MEMORY_HPP
#define OBERON_MEMORY_HPP

#include <memory>

#include "types.hpp"

/**
 * @def OBERON_OPAQUE_BASE_FWD(type)
 * @brief Forward declares an opaque implementation type and a corresponding destructor functor type.
 * @param type The name of the type to forward declare.
 */
#define OBERON_OPAQUE_BASE_FWD(type) \
  class type; \
  struct type##_dtor final { \
    void operator()(const oberon::ptr<type> p) const noexcept; \
  }

/**
 * @def OBERON_OPAQUE_BASE_PTR(type)
 * @brief Declares a `std::unique_ptr` capable of holding a pointer to an opaque implementation type.
 * @param type The name of the type to declare a pointer to.
 * @attention `type` must name a type declared with `OBERON_OPAQUE_BASE_FWD`.
 */
#define OBERON_OPAQUE_BASE_PTR(type) \
  std::unique_ptr<type, type##_dtor> m_impl{ }

namespace oberon::inline pointer_types {

  /**
   * @brief Template for representing mutable pointers in a consistent way.
   * @details `ptr<T>` is equivalent to `T*`. `const ptr<T>` is equivalent to `T* const`.
   * @tparam Type The pointed-to type.
   */
  template <typename Type>
  using ptr = Type*;

  /**
   * @brief Template for representing constant pointers in a consistent way.
   * @details `readonly_ptr<T>` is equivalent to `const T*`. `const readonly_ptr<T>` is equivalent to
   *          `const T* const`.
   * @tparam Type The pointed-to type.
   */
  template <typename Type>
  using readonly_ptr = const Type*;

  /**
   * @brief Templated alias for a C-style string of `CharType` values.
   * @details C-style strings should always contain a terminating '\0' character.
   * @tparam CharType the character type for the aliased C-style string type.
   */
  template <typename CharType>
  using basic_cstring = readonly_ptr<CharType>;

  /**
   * @brief A C-style string of `char`s.
   */
  using cstring = basic_cstring<char>;

  /**
   * @brief A C-style string of `utf8`s.
   */
  using utf8_cstring = basic_cstring<utf8>;

  /**
   * @brief A C-style string of `utf16`s.
   */
  using utf16_cstring = basic_cstring<utf16>;

  /**
   * @brief A C-style string of `utf32`s.
   */
  using utf32_cstring = basic_cstring<utf32>;

  /**
   * @brief A C-style string of `wchar`s.
   */
  using wcstring = basic_cstring<wchar>;

}

#endif
