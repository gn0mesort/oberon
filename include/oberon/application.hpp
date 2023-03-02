#ifndef OBERON_APPLICATION_HPP
#define OBERON_APPLICATION_HPP

#include <functional>

namespace oberon {

  struct environment;

  class application final {
  public:
    /**
     * @brief Platform generic entry point procedure.
     */
    using entry_point = int(environment&);

    application() = default;
    application(const application& other) = default;
    application(application&& other) = default;

    ~application() noexcept = default;

    application& operator=(const application& rhs) = default;
    application& operator=(application&& rhs) = default;

    /**
     * @brief Execute the given entry point.
     * @details This initializes the application, selects platform specific implementations of various subsystems, and
     *          executes the entry point procedure. When the entry point exits (whether successfully or not) the
     *          result is passed back up to the client application.
     * @param fn The main procedure to execute for the application.
     * @return The result of the entry point procedure. If an error occurred than the result & 0xff will be non-zero.
     */
    int run(const std::function<entry_point>& fn);
  };

}

#endif
