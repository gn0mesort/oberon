/**
 * @file wsi_worker.hpp
 * @brief Internal Linux+X11 WSI worker thread related symbols.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_LINUX_X11_WSI_WORKER_HPP
#define OBERON_INTERNAL_LINUX_X11_WSI_WORKER_HPP

#include <array>

#include "../../../types.hpp"
#include "../../../memory.hpp"
#include "../../../events.hpp"

#include "xcb.hpp"
#include "atoms.hpp"

namespace oberon::internal::linux::x11 {

  /**
   * @brief The URI representing the WSI worker thread.
   */
  constexpr const cstring WSI_WORKER_ENDPOINT{ "inproc://wsi-worker" };

  /**
   * @class wsi_pub_worker_params
   * @brief A structure used to send parameters to the WSI worker thread.
   */
  struct wsi_pub_worker_params final {
    ptr<xcb_connection_t> connection{ };
    ptr<xcb_screen_t> default_screen{ };
    xcb_window_t leader{ };
    u8 xi_major_opcode{ };
    u8 xkb_first_event{ };
    std::array<xcb_atom_t, MAX_ATOM> atoms{ };
  };

  /**
   * @class wsi_event_message
   * @brief A structure representing a message between the WSI worker thread and the system.
   */
  struct wsi_event_message final {
    xcb_window_t window{ };
    event data{ };
  };

  /**
   * @brief The actual WSI worker thread entry point.
   * @param arg A pointer to a `wsi_pub_worker_params` structure. The worker takes ownership of the pointer.
   * @return The same pointer passed as `arg`. Upon returning the worker thread releases control of the pointer.
   */
  ptr<void> wsi_pub_worker(ptr<void> arg);

}

#endif
