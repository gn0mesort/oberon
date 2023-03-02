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

  void input::update_key(const xcb_keycode_t key, const bool pressed) {
    OBERON_INPUT_PRECONDITIONS;
    auto& state = m_key_states[key];
    if (state.pressed)
    {
      state.echo = pressed;
    }
    state.pressed = pressed;
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

}
