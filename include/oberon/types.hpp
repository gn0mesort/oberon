/**
 * @file types.hpp
 * @brief Primitive type aliases and support macros.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2022
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_TYPES_HPP
#define OBERON_TYPES_HPP

#include <cinttypes>
#include <cstddef>
#include <climits>

#include <limits>
#include <type_traits>
#include <concepts>
#include <utility>

/**
 * @def OBERON_ENFORCE_CONCEPT(concept, type)
 * @brief Require that `type` fulfills `concept` or generate a compiler error.
 */
#define OBERON_ENFORCE_CONCEPT(concept, type) static_assert(requires { requires concept<type>; })

/**
 * @def OBERON_DEFINE_ZERO_BIT(name)
 * @brief Defines a new constant representing the value 0.
 * @detail This is intended to be used as a base value in a list of OBERON_DEFINE_BIT declarations.
 * @param name The name of the constant to define.
 */
#define OBERON_DEFINE_ZERO_BIT(name) \
  constexpr const oberon::bitmask name##_bit = 0; \
  static_assert(name##_bit == 0)

/**
 * @def OBERON_DEFINE_BIT(name, bit)
 * @brief Defines a new constant representing the value 2**n.
 * @param name The name of the constant to define.
 * @param bit The power of 2 to define the new constant to.
 */
#define OBERON_DEFINE_BIT(name, bit) \
  constexpr const oberon::bitmask name##_bit = (oberon::bitmask{ 1 } << oberon::bitmask{ bit }); \
  static_assert(bit < std::numeric_limits<oberon::bitmask>::digits && name##_bit == (oberon::bitmask{ 1 } << oberon::bitmask{ bit }))

namespace oberon::inline fundamental_types {

  /**
   * @brief Exactly 8-bit unsigned integer type.
   */
  using u8 = std::uint8_t;

  /**
   * @brief Exactly 16-bit unsigned integer type.
   */
  using u16 = std::uint16_t;

  /**
   * @brief Exactly 32-bit unsigned integer type.
   */
  using u32 = std::uint32_t;

  /**
   * @brief Exactly 64-bit unsigned integer type.
   */
  using u64 = std::uint64_t;

  /**
   * @brief Exactly 8-bit signed integer type.
   */
  using i8 = std::int8_t;

  /**
   * @brief Exactly 16-bit signed integer type.
   */
  using i16 = std::int16_t;

  /**
   * @brief Exactly 32-bit signed integer type.
   */
  using i32 = std::int32_t;

  /**
   * @brief Exactly 64-bit signed integer type.
   */
  using i64 = std::int64_t;

  /**
   * @brief  8-bit wide unsigned integer type.
   */
  using least_u8 = std::uint_least8_t;

  /**
   * @brief At least 16-bit wide unsigned integer type.
   */
  using least_u16 = std::uint_least16_t;

  /**
   * @brief At least 32-bit wide unsigned integer type.
   */
  using least_u32 = std::uint_least32_t;

  /**
   * @brief At least 64-bit wide unsigned integer type.
   */
  using least_u64 = std::uint_least64_t;

  /**
   * @brief At least 8-bit wide signed integer type.
   */
  using least_i8 = std::int_least8_t;

  /**
   * @brief At least 16-bit wide signed integer type.
   */
  using least_i16 = std::int_least16_t;

  /**
   * @brief At least 32-bit wide signed integer type.
   */
  using least_i32 = std::int_least32_t;

  /**
   * @brief At least 64-bit wide signed integer type.
   */
  using least_i64 = std::int_least64_t;

  /**
   * @brief Fastest unsigned integer type with at least 8 bits.
   */
  using fast_u8 = std::uint_fast8_t;

  /**
   * @brief Fastest unsigned integer type with at least 16 bits.
   */
  using fast_u16 = std::uint_fast16_t;

  /**
   * @brief Fastest unsigned integer type with at least 32 bits.
   */
  using fast_u32 = std::uint_fast32_t;

  /**
   * @brief Fastest unsigned integer type with at least 64 bits.
   */
  using fast_u64 = std::uint_fast64_t;

  /**
   * @brief Fastest signed integer type with at least 8 bits.
   */
  using fast_i8 = std::int_fast8_t;

  /**
   * @brief Fastest signed integer type with at least 16 bits.
   */
  using fast_i16 = std::int_fast16_t;

  /**
   * @brief Fastest signed integer type with at least 32 bits.
   */
  using fast_i32 = std::int_fast32_t;

  /**
   * @brief Fastest signed integer type with at least 64 bits.
   */
  using fast_i64 = std::int_fast64_t;

  /**
   * @brief Maximum width unsigned integer type.
   */
  using umax = std::uintmax_t;

  /**
   * @brief Maximum width signed integer type.
   */
  using imax = std::intmax_t;

  /**
   * @brief Exactly 32-bit floating point type.
   */
  using f32 = float;

  /**
   * @brief Exactly 64-bit floating point type.
   */
  using f64 = double;

  /**
   * @brief Unsigned integer type suitable for basic system character representation.
   */
  using uchar = unsigned char;

  /**
   * @brief Signed integer type suitable for basic system character representation.
   */
  using ichar = signed char;

  /**
   * @brief Integer type suitable for UTF-8 character representation.
   */
  using utf8 = char8_t;

  /**
   * @brief Integer type suitable for UTF-16 character representation.
   */
  using utf16 = char16_t;

  /**
   * @brief Integer type suitable for UTF-32 character representation.
   */
  using utf32 = char32_t;

  /**
   * @brief Integer type suitable for wide system character representation.
   */
  using wchar = wchar_t;

  /**
   * @brief Unsigned integer type suitable for representing machine sizes.
   */
  using usize = std::size_t;

  /**
   * @brief Signed integer type suitable for representing machine sizes.
   */
  using isize = std::make_signed_t<std::size_t>;

  /**
   * @brief Unsigned integer type suitable for representing pointers.
   */
  using uptr = std::uintptr_t;

  /**
   * @brief Signed integer type suitable for representing pointers.
   */
  using iptr = std::intptr_t;

  /**
   * @brief Unsigned integer type suitable for representing bitmasks.
   */
  using bitmask = u64;

  /**
   * @brief A type representing a namespaced integer handle.
   * @detail Handles are values that work *like* pointers. They are arbitrary unsigned 64-bit integers wrapped in a
   *         type that prevents them from being treated as regular unsigned integers. Handles can be compared to other
   *         handles within the same space. Handles can be implicitly created from integer values. However, handles
   *         cannot be implicitly converted back to integers.
   * @tparam Spaces A set of namespace types that differentiate handles from eachother. Handles with the same `Spaces`
   *         value can be compared. Handles with different `Spaces` values cannot be compared. Essentially:
   *
   *         auto hnd_1 = basic_handle<space_a>{ 1 };
   *         auto hnd_2 = basic_handle<space_b>{ 1 };
   *         if (hnd_1 == hnd_2)
   *         {
   *          // Do something.
   *         }
   *
   *         should not compile even though the two handles use the same underlying data type.
   */
  template <typename... Spaces>
  class basic_handle final {
  public:
    /**
     * @brief The handle's value type. An unsigned 64-bit integer.
     */
    using value_type = u64;
  private:
    value_type m_handle{ 0 };
  public:
    /**
     * @brief Initialize a default handle.
     */
    basic_handle() noexcept = default;

    /**
     * @brief Initialize a handle with a specific underlying value.
     * @param handle The value of the created handle.
     */
    basic_handle(const value_type handle) noexcept : m_handle{ handle } { }

    /**
     * @brief Copy a basic_handle.
     * @param other The handle to copy.
     */
    basic_handle(const basic_handle& other) noexcept = default;

    /**
     * @brief Move a basic_handle.
     * @param other The handle to move.
     */
    basic_handle(basic_handle&& other) noexcept = default;

    /**
     * @brief Destroy a basic_handle.
     */
    ~basic_handle() noexcept = default;

    /**
     * @brief Assign the basic_handle by copying from another basic_handle.
     * @param rhs The basic_handle to copy.
     * @return A reference back to the basic_handle.
     */
    basic_handle& operator=(const basic_handle& rhs) noexcept = default;

    /**
     * @brief Assign the basic_handle by moving from another basic_handle.
     * @param rhs The basic_handle to move.
     * @return A reference back to the basic_handle.
     */
    basic_handle& operator=(basic_handle& rhs) noexcept = default;

    /**
     * @brief Convert a basic_handle to its underlying integer value.
     * @return The underlying unsigned 64-bit integer representing the handle.
     */
    explicit operator value_type() const noexcept { return m_handle; }

    /**
     * @brief Compare two basic_handles for equality.
     * @param rhs The handle to compare to the current handle.
     * @return True if the two handles are the same. Otherwise the result is always false.
     */
    bool operator==(const basic_handle& rhs) const noexcept { return m_handle == rhs.m_handle; }

    /**
     * @brief Compare two basic_handles for inequality.
     * @param rhs The handle to compare to the current handle.
     * @return True if the two handles are the different. Otherwise the result is always false.
     */
    bool operator!=(const basic_handle& rhs) const noexcept { return !(m_handle == rhs.m_handle); }
  };

  /**
   * @brief A read-only type representing a namespaced integer handle.
   * @detail Handles are values that work *like* pointers. They are arbitrary unsigned 64-bit integers wrapped in a
   *         type that prevents them from being treated as regular unsigned integers. Handles can be compared to other
   *         handles within the same space. Handles can be implicitly created from integer values. However, handles
   *         cannot be implicitly converted back to integers.
   * @tparam Spaces A set of namespace types that differentiate handles from eachother. Handles with the same `Spaces`
   *         value can be compared. Handles with different `Spaces` values cannot be compared. Essentially:
   *
   *         auto hnd_1 = basic_handle<space_a>{ 1 };
   *         auto hnd_2 = basic_handle<space_b>{ 1 };
   *         if (hnd_1 == hnd_2)
   *         {
   *          // Do something.
   *         }
   *
   *         should not compile even though the two handles use the same underlying data type.
   */
  template <typename... Spaces>
  class readonly_basic_handle final {
  public:
    /**
     * @brief The handle's value type. An unsigned 64-bit integer.
     */
    using value_type = u64;
  private:
    value_type m_handle{ 0 };
  public:
    /**
     * @brief Initialize a default handle.
     */
    readonly_basic_handle() noexcept = default;

    /**
     * @brief Initialize a handle with a specific underlying value.
     * @param handle The value of the created handle.
     */
    readonly_basic_handle(const value_type handle) noexcept : m_handle{ handle } { }

    /**
     * @brief Initialize a readonly_basic_handle from an compatible basic_handle.
     * @detail This is essentially the same as making the basic_handle read-only. The two handle types must share
     *         a `Spaces` value to be compatible.
     * @param handle The basic_handle to create a readonly_basic_handle from.
     */
    readonly_basic_handle(const basic_handle<Spaces...> handle) : m_handle{ static_cast<uptr>(handle) } { }

    /**
     * @brief Copy a readonly_basic_handle.
     * @param other The handle to copy.
     */
    readonly_basic_handle(const readonly_basic_handle& other) noexcept = default;

    /**
     * @brief Move a readonly_basic_handle.
     * @param other The handle to move.
     */
    readonly_basic_handle(readonly_basic_handle&& other) noexcept = default;

    /**
     * @brief Destroy a readonly_basic_handle.
     */
    ~readonly_basic_handle() noexcept = default;

    /**
     * @brief Assign the readonly_basic_handle by copying from another readonly_basic_handle.
     * @param rhs The readonly_basic_handle to copy.
     * @return A reference back to the readonly_basic_handle.
     */
    readonly_basic_handle& operator=(const readonly_basic_handle& rhs) noexcept = default;

    /**
     * @brief Assign the readonly_basic_handle by moving from another readonly_basic_handle.
     * @param rhs The readonly_basic_handle to move.
     * @return A reference back to the readonly_basic_handle.
     */
    readonly_basic_handle& operator=(readonly_basic_handle& rhs) noexcept = default;

    /**
     * @brief Convert a readonly_basic_handle to its underlying integer value.
     * @return The underlying unsigned 64-bit integer representing the handle.
     */
    explicit operator value_type() const noexcept { return m_handle; }

    /**
     * @brief Compare two readonly_basic_handles for equality.
     * @param rhs The handle to compare to the current handle.
     * @return True if the two handles are the same. Otherwise the result is always false.
     */
    bool operator==(const readonly_basic_handle& rhs) const noexcept { return m_handle == rhs.m_handle; }

    /**
     * @brief Compare two readonly_basic_handles for inequality.
     * @param rhs The handle to compare to the current handle.
     * @return True if the two handles are the different. Otherwise the result is always false.
     */
    bool operator!=(const readonly_basic_handle& rhs) const noexcept { return !(m_handle == rhs.m_handle); }
  };

}

#endif
