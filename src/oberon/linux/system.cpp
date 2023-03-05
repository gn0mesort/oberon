/**
 * @file system.cpp
 * @brief System class implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/linux/system.hpp"

#include <cstring>
#include <cstdlib>

#include "oberon/debug.hpp"

#include "oberon/linux/input.hpp"
#include "oberon/linux/window.hpp"

#define OBERON_SYSTEM_PRECONDITIONS \
  OBERON_PRECONDITION(m_x_display); \
  OBERON_PRECONDITION(m_x_connection); \
  OBERON_PRECONDITION(m_x_screen)

namespace oberon::linux {

  system::system(const std::string& instance_name, const std::string& application_name) :
  m_instance_name{ instance_name },
  m_application_name{ application_name } {
    // Initialize X11
    {
      XInitThreads();
      m_x_display = XOpenDisplay(nullptr);
      XSetEventQueueOwner(m_x_display, XCBOwnsEventQueue);
      m_x_connection = XGetXCBConnection(m_x_display);
      OBERON_CHECK(!xcb_connection_has_error(m_x_connection));
      auto screenp = XDefaultScreen(m_x_display);
      auto setup = xcb_get_setup(m_x_connection);
      for (auto roots = xcb_setup_roots_iterator(setup); roots.rem; xcb_screen_next(&roots))
      {
        if (!(screenp--))
        {
          m_x_screen = roots.data;
        }
      }
      // Initialize XInput
      // Not the Microsoft gamepad thing. The X11 extension
      {
        auto xinput = xcb_get_extension_data(m_x_connection, &xcb_input_id);
        OBERON_CHECK_ERROR_MSG(xinput->present, 1, "The XInput extension is not available.");
        {
          auto request = xcb_input_xi_query_version(m_x_connection, 2, 0);
          auto reply = ptr<xcb_input_xi_query_version_reply_t>{ };
          OBERON_LINUX_X_SUCCEEDS(reply, xcb_input_xi_query_version_reply(m_x_connection, request, err));
          auto major = reply->major_version;
          auto minor = reply->minor_version;
          std::free(reply);
          OBERON_CHECK_ERROR_MSG(major == 2 && minor >= 0, 1, "XInput 2 is not supported.");
        }
        m_xi_major_opcode = xinput->major_opcode;
        {
          auto request = xcb_input_xi_query_device(m_x_connection, XCB_INPUT_DEVICE_ALL_MASTER);
          auto reply = ptr<xcb_input_xi_query_device_reply_t>{ };
          OBERON_LINUX_X_SUCCEEDS(reply, xcb_input_xi_query_device_reply(m_x_connection, request, err));
          auto kbd_found = false;
          auto ptr_found = false;
          for (auto itr = xcb_input_xi_query_device_infos_iterator(reply); itr.rem;
               xcb_input_xi_device_info_next(&itr))
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
        }
      }
      {
        xkb_x11_setup_xkb_extension(m_x_connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                    XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, &m_xkb_first_event,
                                    nullptr);
        // m_xkb_keyboard = xkb_x11_get_core_keyboard_device_id(m_x_connection);
        // std::printf("XKB Keyboard = %hu\nXI Keyboard = %hu\n", m_xkb_keyboard, m_xi_master_keyboard_id);
        OBERON_CHECK(m_xi_master_keyboard_id >= 0);
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
        xcb_xkb_select_events(m_x_connection, m_xi_master_keyboard_id, required_events, 0, 0, map_parts, map_parts,
                                  &details);
        m_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        {
          constexpr const auto mask = XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT;
          auto request = xcb_xkb_per_client_flags(m_x_connection, m_xi_master_keyboard_id, mask, mask, 0, 0, 0);
          auto reply = ptr<xcb_xkb_per_client_flags_reply_t>{ };
          OBERON_LINUX_X_SUCCEEDS(reply, xcb_xkb_per_client_flags_reply(m_x_connection, request, err));
          OBERON_CHECK(reply->supported & mask);
          OBERON_CHECK(reply->value & mask);
          std::free(reply);
        }
      }
    }
    OBERON_ASSERT(m_x_screen);
    OBERON_ASSERT(m_x_connection);
    OBERON_ASSERT(m_x_display);
    // Intern X11 atoms
    {
#define OBERON_LINUX_X_ATOM_NAME(name, str) (str),
      auto atom_names = std::array<cstring, OBERON_LINUX_X_ATOM_MAX>{ OBERON_LINUX_X_ATOMS };
#undef OBERON_LINUX_X_ATOM_NAME
      auto atom_requests = std::array<xcb_intern_atom_cookie_t, OBERON_LINUX_X_ATOM_MAX>{ };
      {
        auto cur = atom_requests.begin();
        for (const auto& atom_name : atom_names)
        {
          *(cur++) = begin_intern_atom(atom_name);
        }
      }
      // TODO: Vulkan instance initialization
      {
        auto cur = m_x_atoms.begin();
        for (const auto& request : atom_requests)
        {
          *(cur++) = end_intern_atom(request);
        }
      }
    }
  }

  xcb_intern_atom_cookie_t system::begin_intern_atom(const cstring name) {
    OBERON_SYSTEM_PRECONDITIONS;
    OBERON_PRECONDITION(name);
    return xcb_intern_atom(m_x_connection, false, std::strlen(name), name);
  }

  xcb_atom_t system::end_intern_atom(const xcb_intern_atom_cookie_t request) {
    OBERON_SYSTEM_PRECONDITIONS;
    auto reply = ptr<xcb_intern_atom_reply_t>{ };
    OBERON_LINUX_X_SUCCEEDS(reply, xcb_intern_atom_reply(m_x_connection, request, err));
    auto result = reply->atom;
    std::free(reply);
    return result;
  }


  system::~system() noexcept {
    OBERON_SYSTEM_PRECONDITIONS;
    xkb_context_unref(m_xkb_context);
    XCloseDisplay(m_x_display);
  }

  std::string system::instance_name() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_instance_name;
  }

  std::string system::application_name() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_application_name;
  }

  ptr<xcb_connection_t> system::connection() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_connection;
  }

  ptr<xcb_screen_t> system::default_screen() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_screen;
  }

  xcb_window_t system::root_window_id() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_screen->root;
  }

  xcb_atom_t system::atom_from_name(const x_atom_name name) const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_x_atoms[name];
  }


  ptr<xkb_context> system::keyboard_context() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xkb_context;
  }

  xcb_input_device_id_t system::keyboard() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xi_master_keyboard_id;
  }

  xcb_input_device_id_t system::pointer() {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xi_master_pointer_id;
  }

  u8 system::xkb_event_code() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xkb_first_event;
  }

  u8 system::xi_major_opcode() const {
    OBERON_SYSTEM_PRECONDITIONS;
    return m_xi_major_opcode;
  }

}
