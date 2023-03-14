/**
 * @file platform.hpp
 * @brief Platform class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_PLATFORM_HPP
#define OBERON_PLATFORM_HPP

#include <functional>

#include "types.hpp"
#include "keys.hpp"
#include "mouse.hpp"

namespace oberon {

  class system;
  class input;
  class window;
  class graphics;

  struct window_offset;
  struct window_extent;

  /**
   * @brief The runtime platform of an Oberon application.
   */
  class platform {
  public:
    /**
     * @brief A function to be called when key press events occur.
     */
    using key_press_event_callback = void(platform& plt, const u32 code, const key k, const bool echoing);

    /**
     * @brief A std::function wrapper for key press event callbacks.
     */
    using key_press_event_fn = std::function<key_press_event_callback>;

    /**
     * @brief A function to be called when key release events occur.
     */
    using key_release_event_callback = void(platform& plt, const u32 code, const key k);

    /**
     * @brief A std::function wrapper for key release event callbacks.
     */
    using key_release_event_fn = std::function<key_release_event_callback>;

    /**
     * @brief A function to be called when mouse movement events occur.
     */
    using mouse_movement_event_callback = void(platform& plt, const mouse_offset& screen_position,
                                               const mouse_offset& window_position);

    /**
     * @brief A std::function wrapper for mouse movement event callbacks.
     */
    using mouse_movement_event_fn = std::function<mouse_movement_event_callback>;

    /**
     * @brief A function to be called when mouse button press events occur.
     */
    using mouse_button_press_event_callback = void(platform& plt, const u32 code, const mouse_button mb);

    /**
     * @brief A std::function wrapper for mouse button press events.
     */
    using mouse_button_press_event_fn = std::function<mouse_button_press_event_callback>;

    /**
     * @brief A function to be called when mouse button release events occur.
     */
    using mouse_button_release_event_callback = void(platform& plt, const u32 code, const mouse_button mb);

    /**
     * @brief A std::function wrapper for mouse button release events.
     */
    using mouse_button_release_event_fn = std::function<mouse_button_release_event_callback>;

    /**
     * @brief A function to be called when window move events occur.
     */
    using window_move_event_callback = void(platform& plt, const window_offset& offset);

    /**
     * @brief A std::function wrapper for window move events.
     */
    using window_move_event_fn = std::function<window_move_event_callback>;

    /**
     * @brief A function to be called when window resize events occur.
     */
    using window_resize_event_callback = void(platform& plt, const window_extent& extent);

    /**
     * @brief A std::function wrapper for window resize events.
     */
    using window_resize_event_fn = std::function<window_resize_event_callback>;

    /**
     * @brief Create a new platform object.
     */
    platform() = default;

    /**
     * @brief Copy an platform object.
     * @param other The platform to copy.
     */
    platform(const platform& other) = default;

    /**
     * @brief Move a platform object.
     * @param other The environement to move.
     */
    platform(platform&& other) = default;

    /**
     * @brief Destroy a platform object.
     */
    inline virtual ~platform() noexcept = 0;

    /**
     * @brief Copy a platform object.
     * @param rhs The platform to copy.
     * @return A reference to the assigned object.
     */
    platform& operator=(const platform& rhs) = default;

    /**
     * @brief Move a platform object.
     * @param rhs The platform to move.
     * @return A reference to the assigned object.
     */
    platform& operator=(platform& rhs) = default;

    /**
     * @brief Retrieve the current system handle.
     * @return A reference to the system object.
     */
    virtual class system& system() = 0;

    /**
     * @brief Retrieve the current input handle.
     * @return A reference to the input object.
     */
    virtual class input& input() = 0;

    /**
     * @brief Retrieve the current window handle.
     * @return A reference to the window object.
     */
    virtual class window& window() = 0;

    virtual class graphics& graphics() = 0;

    /**
     * @brief Attach a new key press event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    virtual void attach_key_press_event_callback(const key_press_event_fn& fn) = 0;

    /**
     * @brief Detach the currently attached key press event callback.
     */
    virtual void detach_key_press_event_callback() = 0;

    /**
     * @brief Attach a new key release event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    virtual void attach_key_release_event_callback(const key_release_event_fn& fn) = 0;

    /**
     * @brief Detach the currently attached key release event callback.
     */
    virtual void detach_key_release_event_callback() = 0;

    /**
     * @brief Attach a new mouse movement event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    virtual void attach_mouse_movement_event_callback(const mouse_movement_event_fn& fn) = 0;


    /**
     * @brief Detach the currently attached mouse movement event callback.
     */
    virtual void detach_mouse_movement_event_callback() = 0;

    /**
     * @brief Attach a new mouse button press event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    virtual void attach_mouse_button_press_event_callback(const mouse_button_press_event_fn& fn) = 0;

    /**
     * @brief Detach the currently attached mouse button press event callback.
     */
    virtual void detach_mouse_button_press_event_callback() = 0;

    /**
     * @brief Attach a new mouse button release event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    virtual void attach_mouse_button_release_event_callback(const mouse_button_release_event_fn& fn) = 0;

    /**
     * @brief Detach the currently attached mouse button release event callback.
     */
    virtual void detach_mouse_button_release_event_callback() = 0;

    /**
     * @brief Attach a new window move event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    virtual void attach_window_move_event_callback(const window_move_event_fn& fn) = 0;

    /**
     * @brief Detach the currently attached window move event callback.
     */
    virtual void detach_window_move_event_callback() = 0;

    /**
     * @brief Attach a new window resize event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    virtual void attach_window_resize_event_callback(const window_resize_event_fn& fn) = 0;

    /**
     * @brief Detach the currently attached window resize event callback.
     */
    virtual void detach_window_resize_event_callback() = 0;

    /**
     * @brief Poll the platform event queue until no more events are found.
     * @details This empties the platform event queue and dispatches events to their corresponding subsystems.
     */
    virtual void drain_event_queue() = 0;
  };

  platform::~platform() noexcept { }

}

#endif
