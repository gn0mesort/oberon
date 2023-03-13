#ifndef OBERON_APPLICATION_HPP
#define OBERON_APPLICATION_HPP

#include <functional>
#include <filesystem>
#include <sstream>

#include "memory.hpp"

namespace oberon {

  class platform;

  class application final {
  private:
    std::ostringstream m_additional_search_paths{ };
  public:
    /**
     * @brief Platform generic entry point procedure.
     */
    using entry_point = int(const int argc, const ptr<csequence> argv, platform&);

    /**
     * @brief Create a new application object.
     */
    application() = default;

    /**
     * @brief Copy an application.
     * @param other The application to copy.
     */
    application(const application& other) = delete;

    /**
     * @brief Move an application.
     * @param other The application to move.
     */
    application(application&& other) = default;

    /**
     * @brief Destroy and application.
     */
    ~application() noexcept = default;

    /**
     * @brief Copy an application.
     * @param rhs The application to copy.
     * @return A reference to the assigned application.
     */
    application& operator=(const application& rhs) = delete;

    /**
     * @brief Move an application.
     * @param rhs The application to move.
     * @return A reference to the assigned application.
     */
    application& operator=(application&& rhs) = default;

    void add_search_path(const std::filesystem::path& search);

    /**
     * @brief Execute the given entry point.
     * @details This initializes the application, selects platform specific implementations of various subsystems, and
     *          executes the entry point procedure. When the entry point exits (whether successfully or not) the
     *          result is passed back up to the client application.
     * @param fn The main procedure to execute for the application.
     * @param argc The number of arguments in the succeeding argv sequence. This may be the same argc parameter passed
     *             to the main function of the program.
     * @param argv The argument vector. This may be the same argv that was passed to the main function. Like the argv
     *             pointer that is passed to main, this must contain argc + 1 pointers and argv[argc] must be null.
     *             Like the argv pointer passed to main, if argv[0] is not null (or argc > 0) then the value of
     *             argv[0] must be a C-style string representing the name used to invoke the program.
     * @return The result of the entry point procedure. If an error occurred than the result & 0xff will be non-zero.
     * @throws check_failed_error If argv[argc] is not null.
     */
    int run(const std::function<entry_point>& fn, const int argc, const ptr<csequence> argv);
  };

}

#endif
