#ifndef OBERON_ENVIRONMENT_HPP
#define OBERON_ENVIRONMENT_HPP

namespace oberon {

  class system;
  class input;
  class window;

  /**
   * @brief The runtime environment of an Oberon application.
   */
  struct environment final {
    /**
     * @brief The current system handle.
     */
    class system& system;

    /**
     * @brief The current input handle.
     */
    class input& input;

    /**
     * @brief The current window handle.
     */
    class window& window;
  };

}

#endif
