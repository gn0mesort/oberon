/**
 * @file dump_keycodes.cpp
 * @brief Simple X11 keycode dumping tool.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include <cstdio>
#include <cinttypes>
#include <cstddef>
#include <cstring>

#include <limits>
#include <array>

#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

using u8 = std::uint8_t;
using usize = std::size_t;

int main(int argc, char** argv) {
  auto prefix = "";
  if (argc > 1)
  {
    prefix = argv[1];
  }
  auto c = xcb_connect(nullptr, nullptr);
  if (xcb_connection_has_error(c))
  {
    return 1;
  }
  xkb_x11_setup_xkb_extension(c, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MAJOR_XKB_VERSION,
                              XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, nullptr, nullptr);
  auto ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  auto keyboard = xkb_x11_get_core_keyboard_device_id(c);
  auto keymap = xkb_x11_keymap_new_from_device(ctx, c, keyboard, XKB_KEYMAP_COMPILE_NO_FLAGS);
  auto names = std::array<const char*, 256>{ };
  for (auto i = usize{ std::numeric_limits<u8>::min() }; i <= std::numeric_limits<u8>::max(); ++i)
  {
    auto name = xkb_keymap_key_get_name(keymap, i);
    if (!name)
    {
      name = "";
    }
    names[i] = name;
    std::printf("\t%s(0x%02zx, \"%s\"), \\\n", prefix, i, name);
  }
  for (auto i = usize{ std::numeric_limits<u8>::min() }; i <= std::numeric_limits<u8>::max(); ++i)
  {
    if (!std::strcmp(names[i], ""))
    {
      continue;
    }
    auto code = xkb_keymap_key_by_name(keymap, names[i]);
    if (i != code)
    {
      std::fprintf(stderr, "Keycode mismatch.\n\tOriginal: %zu\n\tCurrent: %u\n", i, code);
    }
  }
  xkb_keymap_unref(keymap);
  xkb_context_unref(ctx);
  xcb_disconnect(c);
  return 0;
}