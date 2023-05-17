#include "oberon/stopwatch.hpp"

namespace oberon {

  stopwatch::time_point stopwatch::start() const {
    return m_start;
  }

  stopwatch::duration stopwatch::current() const {
    return std::chrono::steady_clock::now() - m_start;
  }

  stopwatch::duration stopwatch::reset() {
    const auto start = m_start;
    m_start = std::chrono::steady_clock::now();
    return std::chrono::steady_clock::now() - start;
  }

}
