#ifndef OBERON_LINUX_XCB_XCB_HPP
#define OBERON_LINUX_XCB_XCB_HPP

#include <xcb/xcb.h>
#include <xcb/xinput.h>
// Don't use C++ keywords in C headers please!
// GCC doesn't emit a warning for this currently.
#if defined(USING_CLANG)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define explicit xcb_xkb_explict
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#undef explicit
#if defined(USING_CLANG)
  #pragma clang diagnostic pop
#endif

#endif
