/**
 * @file input.hpp
 * @brief Input class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INPUT_HPP
#define OBERON_INPUT_HPP

#include "types.hpp"
#include "keys.hpp"
#include "mouse.hpp"

namespace oberon {

  /**
   * @brief A class representing input devices (e.g., keyboards and mice) attached to the system.
   */
  class input {
  public:
    /**
     * @brief Constructs a new input object.
     */
    input() = default;

    /**
     * @brief Copy an input object.
     * @param other The input object to copy.
     */
    input(const input& other) = default;

    /**
     * @brief Move an input object.
     * @param other The input object to move.
     */
    input(input&& other) = default;

    /**
     * @brief Destroy an input object.
     */
    inline virtual ~input() noexcept = 0;

    /**
     * @brief Copy an input object.
     * @param rhs The input object to copy.
     * @return A reference to the assigned object.
     */
    input& operator=(const input& rhs) = default;

    /**
     * @brief Move an input object.
     * @param rhs The input object to move.
     * @return A reference to the assigned object.
     */
    input& operator=(input&& rhs) = default;

    /**
     * @brief Translate a numeric keycode (as-if emitted by pressing a key on a keyboard) into an oberon::key value.
     * @param k The keycode to translate.
     * @return The corresponding oberon::key value. If no corresponding key exists then oberon::key::none is returned.
     */
    virtual key translate_keycode(const u32 k) const = 0;

    /**
     * @brief Translate an oberon::key value into an input device specific keycode.
     * @param k The oberon::key to translate.
     * @return The corresponding keycode if one exists. Otherwise 0.
     */
    virtual u32 translate_key(const key k) const = 0;

    /**
     * @brief Translate a numeric buttoncode (as-if emitted by pressing a button on a mouse) into an
     *        oberon::mouse_button value.
     * @param mb The buttoncode to translate.
     * @return The corresponding oberon::mouse_button value. If no corresponding mouse_button exists then
     *         oberon::mouse_button::none is returned.
     */
    virtual mouse_button translate_buttoncode(const u32 mb) const = 0;

    /**
     * @brief Translate an oberon::mouse_button value into an input device specific buttoncode.
     * @param mb The oberon::mouse_button to translate.
     * @return The corresponding buttoncode if one exists. Otherwise 0.
     */
    virtual u32 translate_button(const mouse_button mb) const = 0;

    /**
     * @brief Check if a key is pressed.
     * @param k The key to check the state of.
     * @return True if the key is currently pressed. Otherwise false.
     */
    virtual bool key_is_pressed(const key k) const = 0;

    /**
     * @brief Check if a pressed key is sending echo key press events.
     * @details On some platforms it may not be possible to detect if a key is echoing. Further, it may not be
     *          possible to detect echoing on certain keys (e.g., left_control).
     * @param k The key to check the state of.
     * @return True if the key is currently pressed and sending echo press events. Otherwise false.
     */
    virtual bool key_is_echoing(const key k) const = 0;

    /**
     * @brief Check if a key is pressed and not echoing.
     * @details On some platforms a pressed key may always be "just pressed" because it does not emit echo events.
     * @param k The key check the state of.
     * @return True if the key is pressed and not echoing. Otherwise false.
     */
    virtual bool key_is_just_pressed(const key k) const = 0;

    /**
     * @brief Check if a modifier key is active.
     * @details Modifier keys are distinct from physical keys (as represented by oberon::key). For example,
     *          oberon::modifier_key::alt represents both the left and right alt keys. Modifier keys may only be
     *          triggered in platform specific conditions. Due to latching and locking of modifiers, an active
     *          modifier key does not explicitly indicate any key is pressed.
     * @param mk The modifier_key to check the state of.
     * @return True if the modifier_key is active. Otherwise false.
     */
    virtual bool modifier_key_is_active(const modifier_key mk) const = 0;

    /**
     * @brief Retrieve the current position of the mouse pointer relative to the screen origin.
     * @return A mouse_offset indicating the position of the mouse pointer relative to the upper left corner of the
     *         screen.
     */
    virtual mouse_offset mouse_screen_position() const = 0;

    /**
     * @brief Retrieve the current position of the mouse pointer relative to the window origin.
     * @return A mouse_offset indicating the position of the mouse pointer relative to the upper left corner of the
     *         window.
     */
    virtual mouse_offset mouse_window_position() const = 0;

    /**
     * @brief Check if a mouse button is pressed.
     * @param mb The mouse_button to check the state of.
     * @return True if the mouse_button is pressed. Otherwise false.
     */
    virtual bool mouse_button_is_pressed(const mouse_button mb) const = 0;
  };

  input::~input() noexcept { }

}

#endif
