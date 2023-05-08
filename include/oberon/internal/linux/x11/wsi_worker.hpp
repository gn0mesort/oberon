#ifndef OBERON_INTERNAL_LINUX_X11_WSI_WORKER_HPP
#define OBERON_INTERNAL_LINUX_X11_WSI_WORKER_HPP

#include <array>

#include "../../../types.hpp"
#include "../../../memory.hpp"
#include "../../../events.hpp"

#include "xcb.hpp"
#include "atoms.hpp"

namespace oberon::internal::linux::x11 {

  constexpr const cstring WSI_WORKER_ENDPOINT{ "inproc://wsi-worker" };

  struct wsi_pub_worker_params final {
    ptr<xcb_connection_t> connection{ };
    ptr<xcb_screen_t> default_screen{ };
    xcb_window_t leader{ };
    u8 xi_major_opcode{ };
    u8 xkb_first_event{ };
    std::array<xcb_atom_t, MAX_ATOM> atoms{ };
  };

  struct wsi_event_message final {
    xcb_window_t window{ };
    event data{ };
  };

  ptr<void> wsi_pub_worker(ptr<void> arg);

}

#endif
