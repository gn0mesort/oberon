/**
 * @file system.hpp
 * @brief System class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_SYSTEM_HPP
#define OBERON_SYSTEM_HPP

#include <filesystem>
#include <string>
#include <unordered_set>

#include "types.hpp"
#include "keys.hpp"

namespace oberon {

  enum class default_file_location {
    none,
    home,
    immutable_data,
    mutable_data,
    cache,
    configuration
  };

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

    virtual void add_additional_search_path(const std::filesystem::path& path) = 0;
    virtual void remove_additional_search_path(const std::filesystem::path& path) = 0;
    virtual const std::unordered_set<std::filesystem::path>& additional_search_paths() const = 0;
    virtual std::filesystem::path home_directory() const = 0;
    virtual std::filesystem::path executable_path() const = 0;
    virtual std::filesystem::path executable_directory() const = 0;
    virtual std::filesystem::path immutable_data_directory() const = 0;
    virtual std::filesystem::path mutable_data_directory() const = 0;
    virtual std::filesystem::path cache_directory() const = 0;
    virtual std::filesystem::path configuration_directory() const = 0;
    virtual std::filesystem::path find_file(const std::string& search, const std::string& name) const = 0;
    virtual std::filesystem::path find_file(const default_file_location location, const std::string& name) const = 0;
  };

  system::~system() noexcept { }

}

#endif
