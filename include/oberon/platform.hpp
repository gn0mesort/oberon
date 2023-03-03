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

namespace oberon {

  class system;
  class input;
  class window;

  /**
   * @brief The runtime platform of an Oberon application.
   */
  class platform {
  public:
    /**
     * @brief A function to be called when key press or key release events occur.
     */
    using key_event_callback = void(platform&);

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


    /**
     * @brief Attach a new key event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     */
    virtual void attach_key_event_callback(const std::function<key_event_callback>& fn) = 0;

    /**
     * @brief Detach the currently attached key event callback.
     */
    virtual void detach_key_event_callback() = 0;

    /**
     * @brief Poll the platform event queue until no more events are found.
     * @details This empties the platform event queue and dispatches events to their corresponding subsystems.
     */
    virtual void drain_event_queue() = 0;
  };

  platform::~platform() noexcept { }

}

#endif
