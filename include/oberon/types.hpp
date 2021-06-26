#ifndef OBERON_TYPES_HPP
#define OBERON_TYPES_HPP

#include <cinttypes>
#include <cstddef>
#include <climits>

#include <type_traits>

// Check if an iresult is an error (e.g. less than 0)
#define OBERON_IS_IERROR(x) ((x) < 0)
// Check if an iresult is a status (e.g. greater than 0)
#define OBERON_IS_ISTATUS(x) ((x) > 0)
// Check if an iresult is a passing return (e.g. equal to 0)
#define OBERON_IS_IPASS(x) ((x) == 0)

namespace oberon {

  using  u8 = std::uint8_t;
  using u16 = std::uint16_t;
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;

  using  i8 = std::int8_t;
  using i16 = std::int16_t;
  using i32 = std::int32_t;
  using i64 = std::int64_t;

  using  least_u8 = std::uint_least8_t;
  using least_u16 = std::uint_least16_t;
  using least_u32 = std::uint_least32_t;
  using least_u64 = std::uint_least64_t;

  using  least_i8 = std::int_least8_t;
  using least_i16 = std::int_least16_t;
  using least_i32 = std::int_least32_t;
  using least_i64 = std::int_least64_t;

  using  fast_u8 = std::uint_fast8_t;
  using fast_u16 = std::uint_fast16_t;
  using fast_u32 = std::uint_fast32_t;
  using fast_u64 = std::uint_fast64_t;

  using  fast_i8 = std::int_fast8_t;
  using fast_i16 = std::int_fast16_t;
  using fast_i32 = std::int_fast32_t;
  using fast_i64 = std::int_fast64_t;

  using umax = std::uintmax_t;
  using imax = std::intmax_t;

  using f32 = float;
  using f64 = double;

  using uchar = unsigned char;
  using ichar = signed char;
  using wchar = wchar_t;

  using usize = std::size_t;
  using isize = std::make_signed_t<std::size_t>;

  using uptr = std::uintptr_t;
  using iptr = std::intptr_t;

  // C-Style integer errors
  using iresult = imax;

}

#endif
