##
# @file meson.build
# @brief Meson build script for Linux specific tools.
# @author Alexander Rothman <gnomesort@megate.ch>
# @date 2023
# @copyright AGPL-3.0+
#
executable('dump_keycodes', files('dump_keycodes.cpp'),
           dependencies: [ dependency('xcb'), dependency('xkbcommon'), dependency('xkbcommon-x11') ])
executable('dump_mods', files('dump_mods.cpp'),
           dependencies: [ dependency('xcb'), dependency('xkbcommon'), dependency('xkbcommon-x11') ])
executable('dump_buttons', files('dump_buttons.cpp'), dependencies: [ dependency('xcb') ])
