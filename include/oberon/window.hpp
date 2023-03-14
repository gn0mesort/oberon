/**
 * @file window.hpp
 * @brief Window class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <string>

#include "types.hpp"

namespace oberon {

  struct window_system_message_event;

  /**
   * @brief A structure describing the extent (i.e., size) of a window.
   */
  struct window_extent final {
    /**
     * @brief The width of the window in pixels.
     */
    u16 width{ };
    /**
     * @brief The height of the window in pixels.
     */
    u16 height{ };
  };

  /**
   * @brief A structure describing the offset from the screen origin (i.e., position) of a window.
   */
  struct window_offset final {
    /**
     * @brief The x-axis position of the window in pixels.
     */
    i16 x{ };
    /**
     * @brief The y-axis position of the window in pixels.
     */
    i16 y{ };
  };

  /**
   * @brief A structure describing the position and size of a window.
   */
  struct window_rect final {
    /**
     * @brief The position of the window.
     */
    window_offset offset{ };
    /**
     * @brief The size of the window.
     */
    window_extent extent{ };
  };

  /**
   * @brief A class representing a window system window.
   */
  class window {
  public:
    /**
     * @brief An enumeration of possible window display styles.
     */
    enum class display_style {
      /**
       * @brief The windowed display style.
       * @details Windowed applications should be displayed within a frame and be managed by the window system.
       */
      windowed,

      /**
       * @brief The fullscreen display style with compositing.
       * @details Composited fullscreen windows should fill the entire screen but still be controlled by the
       *         window system. Compositing should remain enabled when a window is in this style.
       */
      fullscreen_composited,

      /**
       * @brief The fullscreen display style without compositing.
       * @details Uncomposited fullscreen widnows should fill the entire screen and may bypass the window
       *         system. Compositing should be disabled on the window.
       */
      fullscreen_uncomposited
    };

    /**
     * @brief Create a new window object.
     */
    window() = default;

    /**
     * @brief Copy a window object.
     * @param other The window to copy.
     */
    window(const window& other) = default;

    /**
     * @brief Move a window object.
     * @param other The window to move.
     */
    window(window&& other) = default;

    /**
     * @brief Destroy a window object.
     */
    inline virtual ~window() noexcept = 0;

    /**
     * @brief Copy a window object.
     * @param rhs The window to copy.
     * @return A reference to the assigned window object.
     */
    window& operator=(const window& rhs) = default;

    /**
     * @brief Move a window object.
     * @param rhs The window to move.
     * @return A reference to the assigned window object.
     */
    window& operator=(window&& rhs) = default;

    /**
     * @brief Retrieve an integer ID uniquely identifying the window.
     * @return A non-zero ID representing the calling window.
     */
    virtual u64 unique_id() const = 0;

    /**
     * @brief Change the window display style.
     * @param style The new display style to switch to.
     */
    virtual void change_display_style(const display_style style) = 0;

    /**
     * @brief Retrieve the current display style.
     * @return The current display style (e.g., windowed).
     */
    virtual display_style current_display_style() const = 0;

    /**
     * @brief Show (i.e., display) the window.
     */
    virtual void show() = 0;

    /**
     * @brief Hide (i.e., do not display) the window.
     */
    virtual void hide() = 0;

    /**
     * @brief Check whether or not the window is visible.
     * @return True if the window is shown. Otherwise false.
     */
    virtual bool is_visible() const = 0;

    /**
     * @brief Resize the drawable area of the window.
     * @param size The new size of the drawable area.
     */
    virtual void resize(const window_extent& size) = 0;

    /**
     * @brief Move the window to a specific position.
     * @param position The new position for the window.
     */
    virtual void move_to(const window_offset& position) = 0;

    /**
     * @brief Retrieve the rectangle representing the drawable area of the window.
     * @return A rectangle in the form { { x, y }, { width, height } }.
     */
    virtual window_rect current_drawable_rect() const = 0;

    /**
     * @brief Retrieve the rectangle representing the total area of the window.
     * @return A rectangle in the form { { x, y }, { width, height } }.
     */
    virtual window_rect current_rect() const = 0;

    /**
     * @brief Change the window title.
     * @param title The new window title.
     */
    virtual void change_title(const std::string& title) = 0;

    /**
     * @brief Retrieve the current window title.
     * @return The current window title.
     */
    virtual std::string current_title() const = 0;

    /**
     * @brief Check whether or not the window has received a signal that if should quit.
     * @return True if a quit signal has been received. Otherwise false.
     */
    virtual bool quit_requested() const = 0;

    /**
     * @brief Signal to the window that the application should quit.
     */
    virtual void request_quit() = 0;

    /**
     * @brief Clear a pending window quit signal.
     */
    virtual void clear_quit_request() = 0;
  };

  /// @cond
  window::~window() noexcept { }
  /// @endcond

}

#endif
