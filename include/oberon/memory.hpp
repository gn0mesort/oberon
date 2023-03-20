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
   * @brief Templated alias for a C-style pointer to an array of `Type` values.
   * @details C-style arrays, especially arrays of characters, may have terminators (of various types) but they are
   *          not guaranteed.
   * @tparam Type The data type of the pointed to sequence.
   */
  template <typename Type>
  using basic_sequence = ptr<Type>;

  /**
   * @brief A sequence of `char`s
   * @details csequences may or may not contain a null terminator.
   */
  using csequence = basic_sequence<char>;

  /**
   * @brief A sequence of `utf8`s
   * @details utf8_sequences may or may not contain a null terminator.
   */
  using utf8_sequence = basic_sequence<utf8>;

  /**
   * @brief A sequence of `utf16`s
   * @details utf16_sequences may or may not contain a null terminator.
   */
  using utf16_sequence = basic_sequence<utf16>;

  /**
   * @brief A sequence of `utf32`s
   * @details utf32_sequences may or may not contain a null terminator.
   */
  using utf32_sequence = basic_sequence<utf32>;

  /**
   * @brief Templated alias for a read-only C-style pointer to an array of `Type` values.
   * @details The same rules, regarding terminators, apply to basic_read_only_sequence as basic_sequence.
   */
  template <typename Type>
  using basic_readonly_sequence = readonly_ptr<Type>;

  /**
   * @brief A read-only sequence of `char`s.
   */
  using readonly_csequence = basic_readonly_sequence<char>;

  /**
   * @brief A read-only sequence of `utf8`s.
   */
  using readonly_utf8_sequence = basic_readonly_sequence<utf8>;

  /**
   * @brief A read-only sequence of `utf16`s.
   */
  using readonly_utf16_sequence = basic_readonly_sequence<utf16>;

  /**
   * @brief A read-only sequence of `utf32`s.
   */
  using readonly_utf32_sequence = basic_readonly_sequence<utf32>;

  /**
   * @brief Templated alias for a C-style string of `CharType` values.
   * @details C-style strings should always contain a terminating '\0' character.
   * @tparam CharType The character type for the aliased C-style string type.
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
