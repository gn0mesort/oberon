#ifndef OBERON_INTERNAL_WSI_SYSTEM_HPP
#define OBERON_INTERNAL_WSI_SYSTEM_HPP

#ifdef MESON_SYSTEM_LINUX
  #include "linux/wsi_system.hpp"
#else
  #error The current platform does not support WSI objects.
#endif

#endif
