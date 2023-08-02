/**
 * @file utility.hpp
 * @brief Utility functions.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_UTILITY_HPP
#define OBERON_UTILITY_HPP

#include <cmath>

#include <concepts>
#include <algorithm>

#include "types.hpp"


namespace oberon {

  /**
   * @brief Convert a linear color value to sRGB.
   * @tparam Float A floating-point type.
   * @param color A color value in linear space.
   * @return The color value converted into sRGB.
   */
  template <std::floating_point Float>
  constexpr Float to_srgb_color(Float&& color) {
    if (color <= 0.0031308)
    {
      return 12.92 * color;
    }
    else
    {
      return (1.055 * std::pow(color, 1.0 / 2.4)) - 0.055;
    }
  }

  /**
   * @brief Convert an sRGB color to linear space.
   * @tparam Float A floating-point type.
   * @param color An sRGB color value.
   * @return The color value converted into linear space.
   */
  template <std::floating_point Float>
  constexpr Float to_linear_color(Float&& color) {
    if (color <= 0.04045)
    {
      return color / 12.92;
    }
    else
    {
      return std::pow((color + 0.055) / 1.055, 2.4);
    }
  }

  /**
   * @brief Find the maximum size of a set of types.
   * @tparam Type The first type in the set.
   * @tparam Types Zero or more further types to compare to `Type`.
   * @return The maximum size (in `char`s) of all types in the input set.
   */
  template <typename Type, typename... Types>
  consteval usize max_sizeof() {
    if constexpr (sizeof...(Types))
    {
      return std::max(sizeof(Type), max_sizeof<Types...>());
    }
    return sizeof(Type);
  }

  /**
   * @brief Find the minimum size of a set of types.
   * @tparam Type The first type in the set.
   * @tparam Types Zero or more further types to compare to `Type`.
   * @return The minimum size (in `char`s) of all types in the input set.
   */
  template <typename Type, typename... Types>
  consteval usize min_sizeof() {
    if constexpr (sizeof...(Types))
    {
      return std::min(sizeof(Type), min_sizeof<Types...>());
    }
    return sizeof(Type);
  }

  /**
   * @brief Find the maximum alignment of a set of types.
   * @tparam Type The first type in the set.
   * @tparam Types Zero or more further types to compare to `Type`.
   * @return The maximum alignment (in `char`s) of all types in the input set.
   */
  template <typename Type, typename... Types>
  consteval usize max_alignof() {
    if constexpr (sizeof...(Types))
    {
      return std::max(alignof(Type), max_sizeof<Types...>());
    }
    return alignof(Type);
  }

  /**
   * @brief Find the minimum alignment of a set of types.
   * @tparam Type The first type in the set.
   * @tparam Types Zero or more further types to compare to `Type`.
   * @return The minimum alignment (in `char`s) of all types in the input set.
   */
  template <typename Type, typename... Types>
  consteval usize min_alignof() {
    if constexpr (sizeof...(Types))
    {
      return std::min(alignof(Type), min_sizeof<Types...>());
    }
    return alignof(Type);
  }

  /**
   * @brief Determine if all types in a set are derived from the same base type.
   * @tparam Base The base type from which all other types in the input set must be derived.
   * @tparam Derived The first potentially derived type to test against `Base`.
   * @tparam Types Zero or more remaining types to test against `Base`.
   * @return True if `Derived` and `Types` are derived from `Base`. In all other cases, false is returned.
   */
  template <typename Base, typename Derived, typename... Types>
  consteval bool all_derived() {
    if constexpr (sizeof...(Types))
    {
      return std::is_base_of_v<Base, Derived> && all_derived<Base, Types...>();
    }
    return std::is_base_of_v<Base, Derived>;
  }

  /**
   * @brief Determine if an empty set of types is derived from a base type.
   * @details This is called by the variadic `all_derived<Base, Types...>()` function. It provides a base case for
   *          the recursion. There's no real utility to calling it on its own.
   * @tparam Base The base type.
   * @return True is returned in every case.
   */
  template <typename Base>
  consteval bool all_derived() {
    return true;
  }

  /**
   * @brief Determine if an input type is a member of a set of types.
   * @tparam FirstType The type to check for inclusion in the input set.
   * @tparam SecondType The first type of the input set.
   * @tparam Types Zero or more additional types in the input set.
   * @return True if `FirstType` is the same as `SecondType` or any member of `Types`. False is returned in all other
   *         cases.
   */
  template <typename FirstType, typename SecondType, typename... Types>
  consteval bool is_one_of() {
    if constexpr (sizeof...(Types))
    {
      return std::is_same_v<FirstType, SecondType> || is_one_of<FirstType, Types...>();
    }
    return std::is_same_v<FirstType, SecondType>;
  }

  /**
   * @brief Combine two hash values.
   * @param a The first hash value.
   * @param b The second hash value.
   * @return A new hash value that is a combination of `a` and `b`.
   */
  constexpr usize hash_combine(const usize a, const usize b) {
    if constexpr (sizeof(usize) == 8)
    {
      return a ^ (b + 0x517cc1b727220a95 + (a << 6) + (a >> 2));
    }
    else
    {
      return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }
  }

}

#endif
