/**
 * @file environment.hpp
 * @brief Linux implementation of the enviroment class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_ENVIRONMENT_HPP
#define OBERON_LINUX_ENVIRONMENT_HPP

#include "../memory.hpp"
#include "../environment.hpp"

#include "x11.hpp"

namespace oberon::linux {

  class system;
  class input;
  class window;

  class environment final : public oberon::environment {
  private:
    ptr<class system> m_system{ };
    ptr<class input> m_input{ };
    ptr<class window> m_window{ };

    std::function<key_event_callback> m_key_event_cb{ };

    void handle_x_error(const ptr<xcb_generic_error_t> err);
    void handle_x_event(const u8 response_type, const ptr<xcb_generic_event_t> ev);
  public:
    /**
     * @brief Create a new environment object.
     * @param sys The underlying system object.
     * @param inpt The underlying input object.
     * @param win The underlying window object.
     */
    environment(class system& sys, class input& inpt, class window& win);

    /// @cond
    environment(const environment& other) = delete;
    environment(environment&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy an environment object.
     */
    ~environment() noexcept = default;

    /// @cond
    environment& operator=(const environment& rhs) = delete;
    environment& operator=(environment&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the current system handle.
     * @return A reference to the system object.
     */
    oberon::system& system() override;

    /**
     * @brief Retrieve the current input handle.
     * @return A reference to the input object.
     */
    oberon::input& input() override;

    /**
     * @brief Retrieve the current window handle.
     * @return A reference to the window object.
     */
    oberon::window& window() override;


    /**
     * @brief Attach a new key event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     */
    void attach_key_event_callback(const std::function<key_event_callback>& fn) override;

    /**
     * @brief Detach the currently attached key event callback.
     */
    void detach_key_event_callback() override;

    /**
     * @brief Poll the system event queue until no more events are found.
     * @details This empties the system event queue and dispatches events to their corresponding subsystems.
     */
    void drain_event_queue() override;
  };

}

#endif
