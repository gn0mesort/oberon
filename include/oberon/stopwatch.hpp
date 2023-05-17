#ifndef OBERON_STOPWATCH_HPP
#define OBERON_STOPWATCH_HPP

#include <chrono>

namespace oberon {

  class stopwatch final {
  public:
    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::duration<float>;
  private:
    time_point m_start{ std::chrono::steady_clock::now() };
  public:
    stopwatch() = default;
    stopwatch(const stopwatch& other) = default;
    stopwatch(stopwatch&& other) = default;

    ~stopwatch() noexcept = default;

    stopwatch& operator=(const stopwatch& rhs) = default;
    stopwatch& operator=(stopwatch&& rhs) = default;

    time_point start() const;
    duration current() const;
    duration reset();
  };

}

#endif
