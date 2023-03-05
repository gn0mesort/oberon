/**
 * @file keys.hpp
 * @brief Keys and modifier names.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_KEY_HPP
#define OBERON_KEY_HPP

#include <string>

/**
 * @def OBERON_KEYS
 * @brief A list of physical keyboard keys.
 */
#define OBERON_KEYS \
  OBERON_KEY(escape) \
  OBERON_KEY(function_01) \
  OBERON_KEY(function_02) \
  OBERON_KEY(function_03) \
  OBERON_KEY(function_04) \
  OBERON_KEY(function_05) \
  OBERON_KEY(function_06) \
  OBERON_KEY(function_07) \
  OBERON_KEY(function_08) \
  OBERON_KEY(function_09) \
  OBERON_KEY(function_10) \
  OBERON_KEY(function_11) \
  OBERON_KEY(function_12) \
  OBERON_KEY(print_screen) \
  OBERON_KEY(scroll_lock) \
  OBERON_KEY(pause) \
  OBERON_KEY(character_tilde) \
  OBERON_KEY(character_1) \
  OBERON_KEY(character_2) \
  OBERON_KEY(character_3) \
  OBERON_KEY(character_4) \
  OBERON_KEY(character_5) \
  OBERON_KEY(character_6) \
  OBERON_KEY(character_7) \
  OBERON_KEY(character_8) \
  OBERON_KEY(character_9) \
  OBERON_KEY(character_0) \
  OBERON_KEY(character_minus) \
  OBERON_KEY(character_equal) \
  OBERON_KEY(backspace) \
  OBERON_KEY(tab) \
  OBERON_KEY(character_q) \
  OBERON_KEY(character_w) \
  OBERON_KEY(character_e) \
  OBERON_KEY(character_r) \
  OBERON_KEY(character_t) \
  OBERON_KEY(character_y) \
  OBERON_KEY(character_u) \
  OBERON_KEY(character_i) \
  OBERON_KEY(character_o) \
  OBERON_KEY(character_p) \
  OBERON_KEY(character_left_bracket) \
  OBERON_KEY(character_right_bracket) \
  OBERON_KEY(character_backward_slash) \
  OBERON_KEY(caps_lock) \
  OBERON_KEY(character_a) \
  OBERON_KEY(character_s) \
  OBERON_KEY(character_d) \
  OBERON_KEY(character_f) \
  OBERON_KEY(character_g) \
  OBERON_KEY(character_h) \
  OBERON_KEY(character_j) \
  OBERON_KEY(character_k) \
  OBERON_KEY(character_l) \
  OBERON_KEY(character_semicolon) \
  OBERON_KEY(character_apostrophe) \
  OBERON_KEY(enter) \
  OBERON_KEY(left_shift) \
  OBERON_KEY(character_z) \
  OBERON_KEY(character_x) \
  OBERON_KEY(character_c) \
  OBERON_KEY(character_v) \
  OBERON_KEY(character_b) \
  OBERON_KEY(character_n) \
  OBERON_KEY(character_m) \
  OBERON_KEY(character_comma) \
  OBERON_KEY(character_period) \
  OBERON_KEY(character_forward_slash) \
  OBERON_KEY(right_shift) \
  OBERON_KEY(left_control) \
  OBERON_KEY(left_window) \
  OBERON_KEY(left_alt) \
  OBERON_KEY(space) \
  OBERON_KEY(right_alt) \
  OBERON_KEY(right_window) \
  OBERON_KEY(menu) \
  OBERON_KEY(right_control) \
  OBERON_KEY(insert) \
  OBERON_KEY(home) \
  OBERON_KEY(page_up) \
  OBERON_KEY(rub_out) \
  OBERON_KEY(end) \
  OBERON_KEY(page_down) \
  OBERON_KEY(up) \
  OBERON_KEY(left) \
  OBERON_KEY(down) \
  OBERON_KEY(right) \
  OBERON_KEY(num_lock) \
  OBERON_KEY(key_pad_divide) \
  OBERON_KEY(key_pad_multiply) \
  OBERON_KEY(key_pad_subtract) \
  OBERON_KEY(key_pad_7) \
  OBERON_KEY(key_pad_8) \
  OBERON_KEY(key_pad_9) \
  OBERON_KEY(key_pad_4) \
  OBERON_KEY(key_pad_5) \
  OBERON_KEY(key_pad_6) \
  OBERON_KEY(key_pad_1) \
  OBERON_KEY(key_pad_2) \
  OBERON_KEY(key_pad_3) \
  OBERON_KEY(key_pad_0) \
  OBERON_KEY(key_pad_decimal) \
  OBERON_KEY(key_pad_add) \
  OBERON_KEY(key_pad_enter)

/**
 * @def OBERON_MODIFIER_KEYS
 * @brief A list of physical modifier keys.
 */
#define OBERON_MODIFIER_KEYS \
  OBERON_MODIFIER_KEY(shift) \
  OBERON_MODIFIER_KEY(caps_lock) \
  OBERON_MODIFIER_KEY(control) \
  OBERON_MODIFIER_KEY(alt) \
  OBERON_MODIFIER_KEY(num_lock) \
  OBERON_MODIFIER_KEY(window)

namespace oberon {

/// @cond
#define OBERON_KEY(name) name,
/// @endcond

  /**
   * @brief An enumeration of physical keyboard keys.
   * @details The key names correspond to the US layout.
   */
  enum class key {
    none,
    OBERON_KEYS
  };

/// @cond
#undef OBERON_KEY
/// @endcond

/// @cond
#define OBERON_MODIFIER_KEY(name) name,
/// @endcond

  /**
   * @brief An enumeration of logical modifier keys.
   * @details The key names correspond to the US modifier names.
   */
  enum class modifier_key {
    none,
    OBERON_MODIFIER_KEYS
  };

/// @cond
#undef OBERON_MODIFIER_KEY
/// @endcond

  /**
   * @brief Get a string representing the name of the given key.
   * @param k The key to retrieve the string representation of.
   * @return A string representing the given key.
   */
  std::string to_string(const key k);

  /**
   * @brief Get a string representing the name of the given modifier_key.
   * @param k The modifier_key to retrieve the string representation of.
   * @return A string representing the given modifier_key.
   */
  std::string to_string(const modifier_key k);

}

#endif
