/**
 * @file dump_buttons.cpp
 * @brief Simple X11 pointer button dumping tool.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include <cstdio>
#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <limits>
#include <array>

#include <xcb/xcb.h>

using u8 = std::uint8_t;
using usize = std::size_t;

int main() {
  auto c = xcb_connect(nullptr, nullptr);
  if (xcb_connection_has_error(c))
  {
    return 1;
  }
  {
    auto request = xcb_get_pointer_mapping(c);
    auto err = static_cast<xcb_generic_error_t*>(nullptr);
    auto reply = xcb_get_pointer_mapping_reply(c, request, &err);
    if (!reply)
    {
      auto code = err->error_code;
      std::free(err);
      std::fprintf(stderr, "X11 error: %u\n", code);
      return code;
    }
    auto map_sz = static_cast<usize>(xcb_get_pointer_mapping_map_length(reply));
    auto map = xcb_get_pointer_mapping_map(reply);
    for (auto i = usize{ 0 }; i < map_sz; ++i)
    {
      std::printf("Button mapping: %zu -> %zu\n", i, static_cast<usize>(map[i]));
    }
    std::free(reply);
  }
  xcb_disconnect(c);
  return 0;
}
