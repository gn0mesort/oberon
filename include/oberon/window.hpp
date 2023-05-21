/**
 * @file window.hpp
 * @brief Window system window objects.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <unordered_set>

#include "memory.hpp"
#include "rects.hpp"
#include "events.hpp"
#include "keys.hpp"
#include "mouse.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(window_impl);

}

namespace oberon {

  class graphics_device;

  /**
   * @enum display_style
   * @brief An enumeration of available `window` display styles.
   */
  enum class display_style {
    windowed = 0,
    fullscreen_composited,
    fullscreen_bypass_compositor
  };

  /**
   * @enum presentation_mode
   * @brief An enumeration of potentially available image presentation modes.
   * @detail These values must be kept in sync with `VkPresentModeKHR`
   */
  enum class presentation_mode {
    immediate = 0,
    mailbox = 1,
    fifo = 2,
    fifo_relaxed = 3,
    shared_demand_refresh = 1000111000,
    shared_continuous_refresh = 1000111001
  };

  /**
   * @class window
   * @brief An object representing a window in the host machine's window system.
   */
  class window final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::window_impl);
  public:
    using implementation_type = internal::base::window_impl;

    /**
     * @brief Create a `window`.
     * @param device The `graphics_device` that the `window` will be based on.
     * @param title The title of the `window`.
     * @param bounds The initial bounds of the `window`.
     */
    window(graphics_device& device, const std::string& title, const rect_2d& bounds);

    /// @cond
    window(const window& other) = delete;
    window(window&& other) = delete;
    /// @endcond

    ~window() noexcept = default;

    /// @cond
    window& operator=(const window& rhs) = delete;
    window& operator=(window&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the internal `window` implementation.
     * @return A reference to the `window`'s implementation object.
     */
    implementation_type& implementation();

    /**
     * @brief Retrieve the `window`'s unique id.
     * @return A unique 32-bit integer representing the `window`.
     */
    u32 id() const;

    /**
     * @brief Show the `window`.
     */
    void show();

    /**
     * @brief Hide the `window`.
     */
    void hide();

    /**
     * @brief Determine if the `window` is shown or hidden.
     * @return True if the `window` is shown. False if the `window` is hidden.
     */
    bool is_shown() const;

    /**
     * @brief Determine if the `window` is shown but minimized.
     * @detail This is slightly different from the `window` being shown. A shown `window` is potentially visible on
     *         the screen but is definitely known to the window manager.
     * @return True if the `window` is minimized. False if the `window` is not minimized.
     */
    bool is_minimized() const;

    /**
     * @brief Change the current `display_style`.
     * @param style The new `display_style`.
     */
    void display_style(const display_style style);

    /**
     * @brief Retrieve the current `display_style`.
     * @return The current `display_style`.
     */
    enum display_style display_style() const;

    /**
     * @brief Resize the `window`.
     * @param extent The new `window` size.
     */
    void resize(const extent_2d& extent);


    /**
     * @brief Move the `window`.
     * @param offset The new position of the `window`.
     */
    void move_to(const offset_2d& offset);

    /**
     * @brief Retrieve the geometry of the `window`'s drawable region.
     * @return A `rect_2d` describing the drawable region.
     */
    rect_2d drawable_rect() const;

    /**
     * @brief Retrieve the geometry of the `window`'s total screen region.
     * @return A `rect_2d` describing the screen region.
     */
    rect_2d screen_rect() const;

    /**
     * @brief Change the `window`'s title.
     * @param title The new `window` title.
     */
    void title(const std::string& title);

    /**
     * @brief Retrieve the `window`'s title.
     * @return The `window`'s title.
     */
    std::string title() const;

    /**
     * @brief Poll for window system events.
     * @detail If there are no pending events this returns immediately.
     * @return The next available `window` event or an empty event if none are available.
     */
    event poll_events();

    /**
     * @brief Translate a keycode into a `key` value.
     * @param code The keycode to translate.
     * @return The corresponding `key` value.
     */
    key translate_keycode(const u32 code) const;

    /**
     * @brief Determine whether or not the given `key` is pressed.
     * @param k The `key` to determine the state of.
     * @return True if the `key` is pressed. False if the `key` is not pressed.
     */
    bool is_key_pressed(const key k) const;

    /**
     * @brief Determine whether or not the given `key` is sending echo press events.
     * @param k The `key` to determine the state of.
     * @return True if the `key` is echoing. False if the `key` is not echoing.
     */
    bool is_key_echoing(const key k) const;

    /**
     * @brief Determine whether or not the given `key` was just pressed.
     * @detail A `key` that is "just pressed" is one that has not started to echo.
     * @param k The `key` to determine the state of.
     * @return True if the `key` is pressed but not echoing. False if the `key` is not pressed or is echoing.
     */
    bool is_key_just_pressed(const key k) const;

    /**
     * @brief Determine whether or not a `mouse_button` is pressed.
     * @param mb The `mouse_button` to determine the state of.
     * @return True if the `mouse_button` is pressed. False if the `mouse_button` is not pressed.
     */
    bool is_mouse_button_pressed(const mouse_button mb) const;

    /**
     * @brief Translate a buttoncode into a `mouse_button`.
     * @param code The code to translate.
     * @return The corresponding `mouse_button`.
     */
    mouse_button translate_mouse_buttoncode(const u32 code) const;


    /**
     * @brief Determine if a `modifier_key` is active.
     * @param modifier The `modifier_key` to determine the state of.
     * @return True if the `modifier_key` is active (i.e., pressed, locked, or latched). False if the `modifer_key` is
     *         not active.
     */
    bool is_modifier_key_active(const modifier_key modifier) const;

    /**
     * @brief Retrieve a set of `presentation_mode`s available to the `window`.
     * @return A set of `presentation_mode`s available to the `window`.
     */
    const std::unordered_set<enum presentation_mode>& available_presentation_modes() const;

    /**
     * @brief Request a change in the `window`'s `presentation_mode`.
     * @detail A `window` may reject the client's chosen mode. In this case the mode will be changed to
     *        `presentation_mode::fifo`.
     * @param mode The `presentation_mode` to request.
     */
    void presentation_mode(const enum presentation_mode mode);

    /**
     * @brief Retrieve the current `presentation_mode` of the `window`.
     * @return The current `presentation_mode`.
     */
    enum presentation_mode presentation_mode() const;
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, window);

}

#endif
