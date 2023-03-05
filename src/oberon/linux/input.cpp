/**
 * @file input.cpp
 * @brief Input class implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */

#include "oberon/linux/input.hpp"

#include "oberon/debug.hpp"

#include "oberon/linux/system.hpp"

#define OBERON_INPUT_PRECONDITIONS \
  OBERON_PRECONDITION(m_keymap); \
  OBERON_PRECONDITION(m_state)

#define OBERON_INPUT_POSTCONDITIONS \
  OBERON_POSTCONDITION(m_state); \
  OBERON_POSTCONDITION(m_keymap)

namespace oberon::linux {

  input::input(system& sys) : m_parent{ &sys } {
    initialize();
    OBERON_INPUT_POSTCONDITIONS;
  }

  void input::initialize() {
    OBERON_PRECONDITION(!m_keymap);
    OBERON_PRECONDITION(!m_state);
    m_keymap = xkb_x11_keymap_new_from_device(m_parent->keyboard_context(), m_parent->connection(),
                                              m_parent->keyboard(), XKB_KEYMAP_COMPILE_NO_FLAGS);
    m_state = xkb_x11_state_new_from_device(m_keymap, m_parent->connection(), m_parent->keyboard());
    // Map key name strings to keycodes.
    {
#define OBERON_LINUX_X_KEYCODE_MAPPING(name, str) (str),
      auto keys_names = std::array<cstring, OBERON_LINUX_X_KEY_MAX>{ OBERON_LINUX_X_KEYCODE_MAP };
#undef OBERON_LINUX_X_KEYCODE_MAPPING
      auto cur = m_to_keycode.begin();
      for (const auto name : keys_names)
      {
        *(cur++) = xkb_keymap_key_by_name(m_keymap, name);
      }
    }
    // Map key codes to oberon::keys.
    {
      auto i = 1;
      for (const auto keycode : m_to_keycode)
      {
        m_to_external_key[keycode] = static_cast<key>(i++);
      }
    }
    // Map modifier name strings to modifier indices.
    {
#define OBERON_LINUX_X_MODIFIER_KEY_MAPPING(name, str) (str),
      auto modifier_names = std::array<cstring, OBERON_LINUX_X_MODIFIER_KEY_MAX>{ OBERON_LINUX_X_MODIFIER_KEY_MAP };
#undef OBERON_LINUX_X_MODIFIER_KEY_MAPPING
      auto cur = m_to_mod_index.begin();
      for (const auto name : modifier_names)
      {
        *(cur++) = xkb_keymap_mod_get_index(m_keymap, name);
      }
    }
    // Get pointer map.
    // This doesn't seem to be allowed to change in current servers for "security" reasons.
    // I'm only using this to discover the actual number of pointer buttons.
    // My system has 20 but I know mice with more buttons exist and X11 just says, "pointer buttons are always
    // numbered starting at 1."
    // see: https://www.x.org/releases/current/doc/xproto/x11protocol.html#Pointers
    {
      auto request = xcb_get_pointer_mapping(m_parent->connection());
      auto reply = ptr<xcb_get_pointer_mapping_reply_t>{ };
      OBERON_LINUX_X_SUCCEEDS(reply, xcb_get_pointer_mapping_reply(m_parent->connection(), request, err));
      m_button_states.resize(xcb_get_pointer_mapping_map_length(reply));
      std::free(reply);
    }
    OBERON_INPUT_POSTCONDITIONS;
  }

  void input::deinitialize() noexcept {
    OBERON_INPUT_PRECONDITIONS;
    xkb_state_unref(m_state);
    m_state = nullptr;
    xkb_keymap_unref(m_keymap);
    m_keymap = nullptr;
    OBERON_POSTCONDITION(!m_state);
    OBERON_POSTCONDITION(!m_keymap);
  }

  input::~input() noexcept {
    OBERON_INPUT_PRECONDITIONS;
    deinitialize();
  }

  bool input::key_is_pressed(const key k) const {
    OBERON_INPUT_PRECONDITIONS;
    if (k == oberon::key::none)
    {
      return false;
    }
    return m_key_states[m_to_keycode[static_cast<usize>(k) - 1]].pressed;
  }

  bool input::key_is_echoing(const key k) const {
    OBERON_INPUT_PRECONDITIONS;
    if (k == oberon::key::none)
    {
      return false;
    }
    return m_key_states[m_to_keycode[static_cast<usize>(k) - 1]].echo;
  }

  void input::reinitialize_keyboard() {
    OBERON_INPUT_PRECONDITIONS;
    deinitialize();
    initialize();
  }

  void input::update_keyboard_state(const u8 base_mods, const u8 latched_mods, const u8 locked_mods,
                                    const u16 base_group, const u16 latched_group, const u16 locked_group) {
    xkb_state_update_mask(m_state, base_mods, latched_mods, locked_mods, base_group, latched_group, locked_group);
  }

  void input::update_keyboard(const ptr<xcb_generic_event_t> ev) {
    OBERON_INPUT_PRECONDITIONS;
    auto xkb_ev = reinterpret_cast<ptr<xcb_xkb_generic_event_t>>(ev);
    switch (xkb_ev->xkb_code)
    {
    // The keyboard was attached/detached or the key map changed.
    case XCB_XKB_NEW_KEYBOARD_NOTIFY:
    case XCB_XKB_MAP_NOTIFY:
      deinitialize();
      initialize();
      break;
    // A modifier state has changed.
    case XCB_XKB_STATE_NOTIFY:
      {
        // This updates the state of modifiers on the keyboard.
        // Base modifiers are the set of "pressed" modifiers.
        auto state_notify = reinterpret_cast<ptr<xcb_xkb_state_notify_event_t>>(xkb_ev);
        xkb_state_update_mask(m_state, state_notify->baseMods, state_notify->latchedMods, state_notify->lockedMods,
                              state_notify->baseGroup, state_notify->latchedGroup, state_notify->lockedGroup);
      }
      break;
    }
  }

  void input::update_key(const xcb_keycode_t key, const bool pressed, const bool echoing) {
    OBERON_INPUT_PRECONDITIONS;
    auto& state = m_key_states[key];
    state.pressed = pressed;
    state.echo = echoing;
  }

  key input::translate_keycode(const u32 k) const {
    OBERON_INPUT_PRECONDITIONS;
    if (k >= m_to_external_key.size())
    {
      return oberon::key::none;
    }
    return m_to_external_key[k];
  }

  u32 input::translate_key(const key k) const {
    OBERON_INPUT_PRECONDITIONS;
    if (k == oberon::key::none)
    {
      return 0;
    }
    return m_to_keycode[static_cast<usize>(k) - 1];
  }

  mouse_button input::translate_buttoncode(const u32 mb) const {
    OBERON_INPUT_PRECONDITIONS;
    // TODO: this should return mouse_button::none if mb is out of range.
    return static_cast<mouse_button>(mb);
  }

  u32 input::translate_button(const mouse_button mb) const {
    OBERON_INPUT_PRECONDITIONS;
    return static_cast<u32>(mb);
  }

  bool input::modifier_key_is_active(const modifier_key k) const {
    OBERON_INPUT_PRECONDITIONS;
    if (k == modifier_key::none)
    {
      return false;
    }
    const auto idx = m_to_mod_index[static_cast<usize>(k) - 1];
    return xkb_state_mod_index_is_active(m_state, idx, XKB_STATE_MODS_EFFECTIVE);
  }

  bool input::key_is_just_pressed(const key k) const {
    OBERON_INPUT_PRECONDITIONS;
    return key_is_pressed(k) && !key_is_echoing(k);
  }

  void input::update_mouse_position(const mouse_offset& screen_coord, const mouse_offset& window_coord) {
    OBERON_INPUT_PRECONDITIONS;
    m_mouse_screen_coord = screen_coord;
    m_mouse_window_coord = window_coord;
  }

  void input::update_mouse_button(const xcb_button_t button, const bool pressed) {
    OBERON_INPUT_PRECONDITIONS;
    OBERON_PRECONDITION(button > 0 && button < m_button_states.size());
    m_button_states[button - 1].pressed = pressed;
  }

  bool input::mouse_button_is_pressed(const mouse_button mb) const {
    OBERON_INPUT_PRECONDITIONS;
    return m_button_states[static_cast<usize>(mb) - 1].pressed;
  }

  mouse_offset input::mouse_screen_position() const {
    OBERON_INPUT_PRECONDITIONS;
    return m_mouse_screen_coord;
  }

  mouse_offset input::mouse_window_position() const {
    OBERON_INPUT_PRECONDITIONS;
    return m_mouse_window_coord;
  }

}
