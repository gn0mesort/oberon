/**
 * @file system.hpp
 * @brief System class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

#include <functional>

#include "types.hpp"
#include "keys.hpp"

namespace oberon {

  struct environment;

  /**
   * @brief A class representing basic system functionality.
   */
  class system {
  public:
    /**
     * @brief A function to be called when key press or key release events occur.
     */
    using key_event_callback = void(environment&);

    /**
     * @brief Construct a new system object.
     */
    system() = default;

    /**
     * @brief Copy a system object.
     * @param other The system object to copy.
     */
    system(const system& other) = default;

    /**
     * @brief Move a system object.
     * @param other The system object to move.
     */
    system(system&& other) = default;

    /**
     * @brief Destroy a system object.
     */
    inline virtual ~system() noexcept = 0;

    /**
     * @brief Copy a system object.
     * @param rhs The system object to copy.
     * @return A reference to the assigned object.
     */
    system& operator=(const system& rhs) = default;

    /**
     * @brief Move a system object.
     * @param rhs The system object to move.
     * @return A reference to the assigned object.
     */
    system& operator=(system&& rhs) = default;

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

  system::~system() noexcept { }

}

#endif
