#ifndef OBERON_INTERNAL_WSI_GRAPHICS_DEVICE_HPP
#define OBERON_INTERNAL_WSI_GRAPHICS_DEVICE_HPP

#ifdef MESON_SYSTEM_LINUX
  #include "linux/wsi_graphics_device.hpp"
#else
  #error The current platform does not support WSI capable graphics devices.
#endif

#endif
