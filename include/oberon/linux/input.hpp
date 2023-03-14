/**
 * @file input.hpp
 * @brief Input class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_INPUT_HPP
#define OBERON_LINUX_INPUT_HPP

#include <array>
#include <vector>

#include "../input.hpp"

#include "x11.hpp"

namespace oberon::linux {

  class system;

  /**
   * @brief The Linux implementation of the input object.
   */
  class input final : public oberon::input {
  private:
    ptr<system> m_parent{ };

    ptr<xkb_keymap> m_keymap{ };
    ptr<xkb_state> m_state{ };
    mouse_offset m_mouse_window_coord{ };
    mouse_offset m_mouse_screen_coord{ };

    std::array<xcb_keycode_t, OBERON_LINUX_X_KEY_MAX> m_to_keycode{ };
    std::array<oberon::key, MAX_KEY_COUNT> m_to_external_key{ };
    std::array<xkb_mod_index_t, OBERON_LINUX_X_MODIFIER_KEY_MAX> m_to_mod_index{ };
    std::array<key_state, MAX_KEY_COUNT> m_key_states{ };
    std::vector<button_state> m_button_states{ };

    void initialize();
    void deinitialize() noexcept;
  public:
    /**
     * @brief Construct a new input object using the given parent system.
     * @param sys The parent system. The lifetime of the referenced object must be longer than the created input
     *            object.
     */
    input(system& sys);

    /// @cond
    input(const input& other) = delete;
    input(input&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy an input object.
     */
    ~input() noexcept;

    /// @cond
    input& operator=(const input& rhs) = delete;
    input& operator=(input&& rhs) = delete;
    /// @endcond

    /**
     * @brief Translate a numeric keycode (as-if emitted by pressing a key on a keyboard) into an oberon::key value.
     * @param k The keycode to translate.
     * @return The corresponding oberon::key value. If no corresponding key exists then oberon::key::none is returned.
     */
    key translate_keycode(const u32 k) const override;

    /**
     * @brief Translate an oberon::key value into a input device specific keycode.
     * @param k The oberon::key to translate.
     * @return The corresponding keycode if one exists. Otherwise 0.
     */
    u32 translate_key(const key k) const override;

    /**
     * @brief Translate a numeric buttoncode (as-if emitted by pressing a button on a mouse) into an
     *        oberon::mouse_button value.
     * @param mb The buttoncode to translate.
     * @return The corresponding oberon::mouse_button value. If no corresponding mouse_button exists then
     *         oberon::mouse_button::none is returned.
     */
    mouse_button translate_buttoncode(const u32 mb) const override;

    /**
     * @brief Translate an oberon::mouse_button value into an input device specific buttoncode.
     * @param mb The oberon::mouse_button to translate.
     * @return The corresponding buttoncode if one exists. Otherwise 0.
     */
    u32 translate_button(const mouse_button mb) const override;

    /**
     * @brief Check if a key is pressed.
     * @param k The key to check the state of.
     * @return True if the key is currently pressed. Otherwise false.
     */
    bool key_is_pressed(const key k) const override;

    /**
     * @brief Check if a pressed key is sending echo key press events.
     * @details On some platforms it may not be possible to detect if a key is echoing. Further, it may not be
     *          possible to detect echoing on certain keys (e.g., left_control).
     * @param k The key to check the state of.
     * @return True if the key is currently pressed and sending echo press events. Otherwise false.
     */
    bool key_is_echoing(const key k) const override;

    /**
     * @brief Check if a key is pressed and not echoing.
     * @details On some platforms a pressed key may always be "just pressed" because it does not emit echo events.
     * @param k The key check the state of.
     * @return True if the key is pressed and not echoing. Otherwise false.
     */
    bool key_is_just_pressed(const key k) const override;

    /**
     * @brief Check if a modifier key is active.
     * @details Modifier keys are distinct from physical keys (as represented by oberon::key). For example,
     *          oberon::modifier_key::alt represents both the left and right alt keys. Modifier keys may only be
     *          triggered in platform specific conditions. Due to latching and locking of modifiers, an active
     *          modifier key does not explicitly indicate any key is pressed.
     * @param mk The modifier_key to check the state of.
     * @return True if the modifier_key is active. Otherwise false.
     */
    bool modifier_key_is_active(const modifier_key k) const override;

    /**
     * @brief Deinitialize and then initialize the keyboard state again.
     */
    void reinitialize_keyboard();

    /**
     * @brief Update the keyboard modifier state.
     * @param base_mods The set of depressed modifiers.
     * @param latched_mods The set of latched modifiers.
     * @param locked_mods The set of locked modifiers.
     * @param base_group The group the depressed mods belong to.
     * @param latched_group The group the latchedmodifiers belong to.
     * @param locked_group The group the locked modifiers belong to.
     */
    void update_keyboard_state(const u8 base_mods, const u8 latched_mods, const u8 locked_mods, const u16 base_group,
                               const u16 latched_group, const u16 locked_group);

    /**
     * @brief Process a key press or key release event.
     * @param key An xcb_keycode_t representing the pressed key.
     * @param pressed If the key is pressed or released.
     * @param echoing If the key is echoing or not.
     */
    void update_key(const xcb_keycode_t key, const bool pressed, const bool echoing);

    /**
     * @brief Process a mouse movement event.
     * @param screen_position The position of the mouse pointer relative to the screen origin.
     * @param window_position The position of the mouse pointer relative to the window origin.
     */
    void update_mouse_position(const mouse_offset& screen_position, const mouse_offset& window_position);

    /**
     * @brief Process a mouse button press or mouse button release event.
     * @param button An xcb_button_t representing the button that was pressed.
     * @param pressed If the button is pressed or released.
     */
    void update_mouse_button(const xcb_button_t button, const bool pressed);

    /**
     * @brief Retrieve the current position of the mouse pointer relative to the screen origin.
     * @return A mouse_offset indicating the position of the mouse pointer relative to the upper left corner of the
     *         screen.
     */
    mouse_offset mouse_screen_position() const override;

    /**
     * @brief Retrieve the current position of the mouse pointer relative to the window origin.
     * @return A mouse_offset indicating the position of the mouse pointer relative to the upper left corner of the
     *         window.
     */
    mouse_offset mouse_window_position() const override;

    /**
     * @brief Check if a mouse button is pressed.
     * @param mb The mouse_button to check the state of.
     * @return True if the mouse_button is pressed. Otherwise false.
     */
    bool mouse_button_is_pressed(const mouse_button mb) const override;
  };

}

#endif
