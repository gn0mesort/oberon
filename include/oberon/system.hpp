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

  /**
   * @brief An enumeration of common file locations.
   */
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

    /**
     * @brief Add a path to the set of additional search paths.
     * @param path The path to add.
     */
    virtual void add_additional_search_path(const std::filesystem::path& path) = 0;

    /**
     * @brief Remove a path from the set of additional search paths.
     * @param path The path to remove.
     */
    virtual void remove_additional_search_path(const std::filesystem::path& path) = 0;

    /**
     * @brief Retrieve the set of additional search paths.
     * @return A set of additional search paths used by the system.
     */
    virtual const std::unordered_set<std::filesystem::path>& additional_search_paths() const = 0;

    /**
     * @brief Retrieve the system home directory.
     * @return A path to an implementation defined directory.
     */
    virtual std::filesystem::path home_directory() const = 0;

    /**
     * @brief Retrieve the path of the currently executing binary.
     * @return A path to the currently executing binary.
     */
    virtual std::filesystem::path executable_path() const = 0;

    /**
     * @brief Retrieve the path to the directory in which the currently executing binary resides.
     * @return A path to the directory in which the currently executing binary can be found.
     */
    virtual std::filesystem::path executable_directory() const = 0;

    /**
     * @brief Retrieve the system immutable data directory.
     * @return A path to an implementation defined directory. This directory does not require write access.
     */
    virtual std::filesystem::path immutable_data_directory() const = 0;

    /**
     * @brief Retrieve the system mutable data directory.
     * @return A path to an implementation defined directory.
     */
    virtual std::filesystem::path mutable_data_directory() const = 0;

    /**
     * @brief Retrieve the system cache directory.
     * @return A path to an implementation defined directory.
     */
    virtual std::filesystem::path cache_directory() const = 0;

    /**
     * @brief Retrieve the system configuration directory.
     * @return A path to an implementation defined directory.
     */
    virtual std::filesystem::path configuration_directory() const = 0;

    /**
     * @brief Find a file by searching a set of paths.
     * @details This searches for the file named by name in any of the paths found in search. Implementations must
     *          make it clear what character will be used to separate strings in search. POSIX systems commonly use
     *          ':' and Windows commonly uses ';'. In any case, once separated searching proceeds from the first path
     *          until the file is found or the set of paths is exhausted.
     * @param search A set of search paths. Each path must be separated by an implementation specific character.
     * @param name The name of the file to find. This can contain additional path elements.
     */
    virtual std::filesystem::path find_file(const std::string& search, const std::string& name) const = 0;

    /**
     * @brief Find a file by searching using a preset configuration.
     * @details This behaves like find_file(const std::string&, const std::string&) const but instead of using an
     *          arbitrary set of search paths it uses specific implementation defined paths. The search order is
     *          always as follows:
     *            1. All paths in additional_search_paths().
     *            2. A set of implementation defined paths.
     *            3. executable_directory().
     *
     * @param location The location to search or default_file_location::none to use only the additional paths.
     * @param name The name of the file to find. This can contain additional path elements.
     */
    virtual std::filesystem::path find_file(const default_file_location location, const std::string& name) const = 0;
  };

  /// @cond
  system::~system() noexcept { }
  /// @endcond

}

#endif
