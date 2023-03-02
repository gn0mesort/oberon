/**
 * @file system.hpp
 * @brief System class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

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
  };

  system::~system() noexcept { }

}

#endif
