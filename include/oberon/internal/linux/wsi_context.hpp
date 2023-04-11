#ifndef OBERON_INTERNAL_LINUX_WSI_CONTEXT_HPP
#define OBERON_INTERNAL_LINUX_WSI_CONTEXT_HPP

#ifndef MESON_SYSTEM_LINUX
  #error linux/wsi_context.hpp can only be built on Linux platforms.
#endif

#ifdef MESON_SYSTEM_LINUX_WSI_TYPE_X11
  #include "xcb/wsi_context.hpp"
#else
  #error At least one window system integration must be enabled to build with WSI objects.
#endif

#endif
