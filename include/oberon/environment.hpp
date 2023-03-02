/**
 * @file environment.hpp
 * @brief Enviroment class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_ENVIRONMENT_HPP
#define OBERON_ENVIRONMENT_HPP

#include <functional>

namespace oberon {

  class system;
  class input;
  class window;

  /**
   * @brief The runtime environment of an Oberon application.
   */
  class environment {
  public:
    /**
     * @brief A function to be called when key press or key release events occur.
     */
    using key_event_callback = void(environment&);

    /**
     * @brief Create a new environment object.
     */
    environment() = default;

    /**
     * @brief Copy an environment object.
     * @param other The environment to copy.
     */
    environment(const environment& other) = default;

    /**
     * @brief Move a environment object.
     * @param other The environement to move.
     */
    environment(environment&& other) = default;

    /**
     * @brief Destroy an environment object.
     */
    inline virtual ~environment() noexcept = 0;

    /**
     * @brief Copy an environment object.
     * @param rhs The environment to copy.
     * @return A reference to the assigned object.
     */
    environment& operator=(const environment& rhs) = default;

    /**
     * @brief Move an environment object.
     * @param rhs The environment to move.
     * @return A reference to the assigned object.
     */
    environment& operator=(environment& rhs) = default;

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
     * @brief Poll the system event queue until no more events are found.
     * @details This empties the system event queue and dispatches events to their corresponding subsystems.
     */
    virtual void drain_event_queue() = 0;
  };

  environment::~environment() noexcept { }

}

#endif
