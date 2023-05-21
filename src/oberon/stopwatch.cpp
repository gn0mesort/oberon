/**
 * @file stopwatch.cpp
 * @brief Stopwatch object implementation.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/stopwatch.hpp"

namespace oberon {

  stopwatch::time_point stopwatch::start_time() const {
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
