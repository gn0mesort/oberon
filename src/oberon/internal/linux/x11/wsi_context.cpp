/**
 * @file wsi_context.cpp
 * @brief Internal Linux+X11 wsi_context object implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/internal/linux/x11/wsi_context.hpp"

#include <cstdlib>
#include <cstring>

#include "oberon/errors.hpp"
#include "oberon/debug.hpp"

#include "oberon/internal/linux/x11/wsi_worker.hpp"

#define XCB_SEND_REQUEST(request, connection, ...) \
  OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST(request, connection __VA_OPT__(, __VA_ARGS__))
#define XCB_AWAIT_REPLY(request, connection, cookie, error) \
  OBERON_INTERNAL_LINUX_X11_XCB_AWAIT_REPLY(request, connection, cookie, error)
#define XCB_HANDLE_ERROR(reply, error, msg) \
  OBERON_INTERNAL_LINUX_X11_XCB_HANDLE_ERROR(reply, error, msg)
#define XCB_SEND_REQUEST_SYNC(reply, request, connection, ...) \
  OBERON_INTERNAL_LINUX_X11_XCB_SEND_REQUEST_SYNC(reply, request, connection __VA_OPT__(, __VA_ARGS__))

namespace oberon::internal::linux::x11 {

  wsi_context::wsi_context() : wsi_context{ nullptr, "" } { }

  wsi_context::wsi_context(const std::string& instance) : wsi_context{ nullptr, instance } { }

  wsi_context::wsi_context(const cstring display, const std::string& instance) {
    // Select an instance name.
    m_instance_name = !instance.empty() ? instance : m_application_name;
    auto screenp = int{ };
    m_connection = xcb_connect(display, &screenp);
    OBERON_CHECK_ERROR_MSG(!xcb_connection_has_error(m_connection), 1, "Failed to connect to the X11 server.");
    m_setup = xcb_get_setup(m_connection);
    // Select the default X11 screen.
    for (auto roots = xcb_setup_roots_iterator(m_setup); roots.rem; xcb_screen_next(&roots))
    {
      if (!(screenp--))
      {
        m_default_screen = roots.data;
      }
    }
    // Initialize the XInput2 extension.
    // This is unrelated to the similarly name Microsoft Xbox Controller specification.
    {
      auto xinput = xcb_get_extension_data(m_connection, &xcb_input_id);
      OBERON_CHECK_ERROR_MSG(xinput->present, 1, "The XInput2 extension is not available.");
      // Ensure proper support for XInput2.
      {
        auto reply = ptr<xcb_input_xi_query_version_reply_t>{ };
        XCB_SEND_REQUEST_SYNC(reply, xcb_input_xi_query_version, m_connection, 2, 0);
        auto major = reply->major_version;
        auto minor = reply->minor_version;
        std::free(reply);
        OBERON_CHECK_ERROR_MSG(major == 2 && minor >= 0, 1, "XInput2 is available but version %u.%u is unsupported.",
                               major, minor);
      }
      m_xi_major_opcode = xinput->major_opcode;
      // Find the master keyboard and the master pointer.
      {
        auto reply = ptr<xcb_input_xi_query_device_reply_t>{ };
        XCB_SEND_REQUEST_SYNC(reply, xcb_input_xi_query_device, m_connection, XCB_INPUT_DEVICE_ALL_MASTER);
        auto kbd_found = false;
        auto ptr_found = false;
        for (auto itr = xcb_input_xi_query_device_infos_iterator(reply); itr.rem; xcb_input_xi_device_info_next(&itr))
        {
          if (!kbd_found && itr.data->enabled && itr.data->type == XCB_INPUT_DEVICE_TYPE_MASTER_KEYBOARD)
          {
            m_xi_master_keyboard_id = itr.data->deviceid;
          }
          if (!ptr_found && itr.data->enabled && itr.data->type == XCB_INPUT_DEVICE_TYPE_MASTER_POINTER)
          {
            m_xi_master_pointer_id = itr.data->deviceid;
          }
        }
        std::free(reply);
        OBERON_CHECK_ERROR_MSG(m_xi_master_keyboard_id > 1, 1, "The XInput2 master keyboard device was not found.");
        OBERON_CHECK_ERROR_MSG(m_xi_master_pointer_id > 1, 1, "The XInput2 master pointer device was not found.");
      }
    }
    // Initialize the XKB extension and libxkbcommon structures.
    {
      auto res = int{ 0 };
      res = xkb_x11_setup_xkb_extension(m_connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                        XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, &m_xkb_first_event,
                                        nullptr);
      OBERON_CHECK_ERROR_MSG(res, 1, "The XKB extension failed to initialize.");
      // No XKB events are actually selected at this point.
      // Inexplicable code per libxkbcommon
      // (https://github.com/xkbcommon/libxkbcommon/blob/master/tools/interactive-x11.c)
      constexpr const auto required_events = XCB_XKB_EVENT_TYPE_MAP_NOTIFY | XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
                                             XCB_XKB_EVENT_TYPE_STATE_NOTIFY;
      constexpr const auto map_parts = XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
                                       XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                                       XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_VIRTUAL_MODS |
                                       XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
      constexpr const auto state_details = XCB_XKB_STATE_PART_MODIFIER_BASE | XCB_XKB_STATE_PART_MODIFIER_LATCH |
                                           XCB_XKB_STATE_PART_MODIFIER_LOCK | XCB_XKB_STATE_PART_GROUP_BASE |
                                           XCB_XKB_STATE_PART_GROUP_LATCH | XCB_XKB_STATE_PART_GROUP_LOCK;
      auto details = xcb_xkb_select_events_details_t{ };
      details.affectNewKeyboard = XCB_XKB_NKN_DETAIL_KEYCODES;
      details.newKeyboardDetails = XCB_XKB_NKN_DETAIL_KEYCODES;
      details.affectState = state_details;
      details.stateDetails = state_details;
      // This returns xcb_void_cookie_t and requires no check.
      xcb_xkb_select_events(m_connection, m_xi_master_keyboard_id, required_events, 0, 0, map_parts, map_parts,
                            &details);
      m_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
      OBERON_CHECK_ERROR_MSG(m_xkb_context, 1, "Failed to create a valid XKB context.");
      constexpr const auto mask = XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT;
      auto reply = ptr<xcb_xkb_per_client_flags_reply_t>{ };
      XCB_SEND_REQUEST_SYNC(reply, xcb_xkb_per_client_flags, m_connection, m_xi_master_keyboard_id, mask, mask, 0, 0,
                            0);
      auto supported = reply->supported & mask;
      auto value = reply->value & mask;
      std::free(reply);
      OBERON_CHECK_ERROR_MSG(supported && value, 1, "Failed to set XKB per-client flags for the master keyboard.");
    }
    // Initialize the required X11 atoms.
    {
#define OBERON_INTERNAL_LINUX_X11_ATOM_NAME(name, str) (str),
      auto atom_strs = std::array<cstring, MAX_ATOM>{ OBERON_INTERNAL_LINUX_X11_ATOMS };
#undef OBERON_INTERNAL_LINUX_X11_ATOM_NAME
      // Dispatching independent requests in sequence like this is more optimal, generally, than using
      // XCB_SEND_REQUEST_SYNC.
      auto atom_cookies = std::array<xcb_intern_atom_cookie_t, MAX_ATOM>{ };
      {
        auto cur = atom_cookies.begin();
        for (const auto atom_str : atom_strs)
        {
          *(cur++) = XCB_SEND_REQUEST(xcb_intern_atom, m_connection, false, std::strlen(atom_str), atom_str);
        }
      }
      {
        auto cur = m_atoms.begin();
        auto reply = ptr<xcb_intern_atom_reply_t>{ };
        auto error = ptr<xcb_generic_error_t>{ };
        for (const auto cookie : atom_cookies)
        {
          reply = XCB_AWAIT_REPLY(xcb_intern_atom, m_connection, cookie, &error);
          XCB_HANDLE_ERROR(reply, error, "Failed to intern X11 atom.");
          *(cur++) = reply->atom;
          std::free(reply);
        }
      }
    }
    // Initialize the client leader window.
    {
      m_client_leader = xcb_generate_id(m_connection);
      OBERON_CHECK_ERROR_MSG(m_client_leader, 1, "Failed to allocate an XID for the system client leader.");
      constexpr const auto value_mask = XCB_CW_EVENT_MASK;
      constexpr const auto event_mask = u32{ XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY };
      xcb_create_window(m_connection, 0, m_client_leader, m_default_screen->root, 0, 0, 64, 64, 0,
                        XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, value_mask, &event_mask);
      xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_client_leader, m_atoms[WM_CLIENT_LEADER_ATOM],
                          XCB_ATOM_WINDOW, 32, 1, &m_client_leader);
      auto hints = xcb_hints_t{ };
      hints.flags = hint_flag_bits::window_group_hint_bit;
      hints.window_group = m_client_leader;
      xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_client_leader, m_atoms[WM_HINTS_ATOM],
                          XCB_ATOM_WM_HINTS, 32, sizeof(xcb_hints_t) >> 1, &hints);
    }
    // Start worker thread.
    OBERON_CHECK_ERROR_MSG(!pthread_create(&m_wsi_pub_worker, nullptr, wsi_pub_worker,
                                           new wsi_pub_worker_params{ m_connection, m_default_screen, m_client_leader,
                                                                      m_xi_major_opcode, m_xkb_first_event,
                                                                      m_atoms }),
                           1, "The system could not start the WSI event publisher thread.");
    OBERON_CHECK_ERROR_MSG(!pthread_setname_np(m_wsi_pub_worker, "wsi-pub-0"), 1, "The system could not set a thread "
                           "name.");
    xcb_flush(m_connection);
  }

  wsi_context::~wsi_context() noexcept {
    auto client_message = xcb_client_message_event_t{ };
    client_message.response_type = XCB_CLIENT_MESSAGE;
    client_message.type = m_atoms[OBERON_SYSTEM_SIGNAL_ATOM];
    client_message.window = m_client_leader;
    client_message.format = 32;
    client_message.data.data32[0] = QUIT_SYSTEM_SIGNAL;
    send_client_message(m_client_leader, client_message);
    xcb_flush(m_connection);
    auto params = ptr<wsi_pub_worker_params>{ };
    pthread_join(m_wsi_pub_worker, reinterpret_cast<ptr<ptr<void>>>(&params));
    delete params;
    xcb_destroy_window(m_connection, m_client_leader);
    xkb_context_unref(m_xkb_context);
    xcb_disconnect(m_connection);
  }

  void wsi_context::send_client_message(const xcb_window_t window, const xcb_client_message_event_t& message) {
    constexpr const auto MASK = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    auto ev = xcb_generic_event_t{ };
    std::memset(&ev, 0, sizeof(xcb_generic_event_t));
    std::memcpy(&ev, &message, sizeof(xcb_client_message_event_t));
    xcb_send_event(m_connection, false, window, MASK, reinterpret_cast<cstring>(&ev));
  }

  ptr<xcb_connection_t> wsi_context::connection() {
    return m_connection;
  }

  ptr<xcb_screen_t> wsi_context::default_screen() {
    return m_default_screen;
  }

  xcb_atom_t wsi_context::atom_by_name(const atom name) const {
    return m_atoms[name];
  }

  const std::string& wsi_context::instance_name() const {
    return m_instance_name;
  }

  const std::string& wsi_context::application_name() const {
    return m_application_name;
  }

  xcb_window_t wsi_context::leader() {
    return m_client_leader;
  }

  ptr<xkb_context> wsi_context::keyboard_context() {
    return m_xkb_context;
  }

  xcb_input_device_id_t wsi_context::keyboard() {
    return m_xi_master_keyboard_id;
  }

  xcb_input_device_id_t wsi_context::pointer() {
    return m_xi_master_pointer_id;
  }

  bool wsi_context::is_keyboard_event(const u8 type) const {
    return type == m_xkb_first_event;
  }

}
