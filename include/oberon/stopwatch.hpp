/**
 * @file stopwatch.hpp
 * @brief Stopwatch objects.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_STOPWATCH_HPP
#define OBERON_STOPWATCH_HPP

#include <chrono>

namespace oberon {

  /**
   * @class stopwatch
   * @brief An object representing a simple stopwatch.
   * @details A `stopwatch` is a simple timing object which measures the number of seconds elapsed since the last
   *          reset of the `stopwatch`. Seconds are returned as a floating point duration. `stopwatch`es are not
   *          timers. No events are triggered by a `stopwatch`. `stopwatch`es measure monotonic time so they aren't
   *          affected by changes in the wall-clock time of the host machine.
   */
  class stopwatch final {
  public:
    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::duration<float>;
  private:
    time_point m_start{ std::chrono::steady_clock::now() };
  public:
    /**
     * @brief Create a `stopwatch`.
     */
    stopwatch() = default;

    /**
     * @brief Create a `stopwatch` as a copy of another `stopwatch`.
     * @param other The `stopwatch` to copy.
     */
    stopwatch(const stopwatch& other) = default;

    /**
     * @brief Create a `stopwatch` by moving another `stopwatch`.
     * @param other The `stopwatch` to move.
     */
    stopwatch(stopwatch&& other) = default;

    /**
     * @brief Destroy a `stopwatch`.
     */
    ~stopwatch() noexcept = default;

    /**
     * @brief Assign a `stopwatch` to copy another `stopwatch`.
     * @param rhs The `stopwatch` to copy.
     */
    stopwatch& operator=(const stopwatch& rhs) = default;

    /**
     * @brief Assign a `stopwatch` by moving another `stopwatch`.
     * @param rhs The `stopwatch` to move.
     */
    stopwatch& operator=(stopwatch&& rhs) = default;

    /**
     * @brief Retrieve time of the last reset.
     * @return A `time_point` representing the time at which the `stopwatch` was reset.
     */
    time_point start_time() const;

    /**
     * @brief Retrieve the currently elapsed time without reseting the `stopwatch`.
     * @return The `duration` that the `stopwatch` has been running for.
     */
    duration current() const;

    /**
     * @brief Retrieve the currently elapsed time and reset the `stopwatch`.
     * @return The `duration` that elapsed before the reset.
     */
    duration reset();
  };

}

#endif
