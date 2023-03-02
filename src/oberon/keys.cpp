/**
 * @file keys.cpp
 * @brief Key enumeration utility implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/keys.hpp"

namespace oberon {

#define OBERON_KEY(name) case key::name: return #name;
  std::string to_string(const key k) {
    switch (k)
    {
    OBERON_KEYS
    default:
      return "none";
    }
  }
#undef OBERON_KEY

#define OBERON_MODIFIER_KEY(name) case modifier_key::name: return #name;
  std::string to_string(const modifier_key k) {
    switch (k)
    {
    OBERON_MODIFIER_KEYS
    default:
      return "none";
    }
  }
#undef OBERON_MODIFIER_KEY

}
