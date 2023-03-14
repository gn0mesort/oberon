/**
 * @file platform.hpp
 * @brief Linux implementation of the platform class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_PLATFORM_HPP
#define OBERON_LINUX_PLATFORM_HPP

#include "../memory.hpp"
#include "../platform.hpp"

#include "x11.hpp"

namespace oberon::linux {

  class system;
  class input;
  class window;
  class graphics;

  class platform final : public oberon::platform {
  private:
    using event_handler = void(platform&, const u8, const ptr<xcb_generic_event_t>);
    using xi_event_handler = void(platform&, const u16, const ptr<xcb_ge_generic_event_t>);
    using xkb_event_handler = void(platform&, const u8, const ptr<xcb_xkb_generic_event_t>);

    static void handle_error(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev);
    static void handle_client_message(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev);
    static void handle_configure_notify(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev);
    static void handle_ge_generic_event(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev);
    static void handle_xi_key_press_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev);
    static void handle_xi_key_release_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev);
    static void handle_xi_button_press_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev);
    static void handle_xi_button_release_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev);
    static void handle_xi_motion_event(platform& plt, const u16 type, const ptr<xcb_ge_generic_event_t> ev);
    static void handle_xkb_event(platform& plt, const u8 type, const ptr<xcb_generic_event_t> ev);
    static void handle_xkb_state_notify(platform& plt, const u8 type, const ptr<xcb_xkb_generic_event_t> ev);
    static void handle_xkb_new_keyboard_notify_and_map_notify(platform& plt, const u8 type,
                                                              const ptr<xcb_xkb_generic_event_t> ev);

    ptr<class system> m_system{ };
    ptr<class input> m_input{ };
    ptr<class window> m_window{ };
    ptr<class graphics> m_graphics{ };

    key_press_event_fn m_key_press_event_cb{ };
    key_release_event_fn m_key_release_event_cb{ };
    mouse_movement_event_fn m_mouse_movement_event_cb{ };
    mouse_button_press_event_fn m_mouse_button_press_event_cb{ };
    mouse_button_release_event_fn m_mouse_button_release_event_cb{ };
    window_move_event_fn m_window_move_event_cb{ };
    window_resize_event_fn m_window_resize_event_cb{ };

    std::array<ptr<event_handler>, OBERON_LINUX_X_EVENT_MAX> m_event_handlers{ };
    std::array<ptr<xi_event_handler>, OBERON_LINUX_X_XI_EVENT_MAX> m_xi_event_handlers{ };
    std::array<ptr<xkb_event_handler>, OBERON_LINUX_X_XKB_EVENT_MAX> m_xkb_event_handlers{ };
  public:
    /**
     * @brief Create a new platform object.
     * @param sys The underlying system object.
     * @param inpt The underlying input object.
     * @param win The underlying window object.
     */
    platform(class system& sys, class input& inpt, class window& win, class graphics& gfx);

    /// @cond
    platform(const platform& other) = delete;
    platform(platform&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a platform object.
     */
    ~platform() noexcept = default;

    /// @cond
    platform& operator=(const platform& rhs) = delete;
    platform& operator=(platform&& rhs) = delete;
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

    oberon::graphics& graphics() override;

    /**
     * @brief Attach a new key press event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    void attach_key_press_event_callback(const key_press_event_fn& fn) override;

    /**
     * @brief Detach the currently attached key press event callback.
     */
    void detach_key_press_event_callback() override;

    /**
     * @brief Attach a new key release event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    void attach_key_release_event_callback(const key_release_event_fn& fn) override;

    /**
     * @brief Detach the currently attached key release event callback.
     */
    void detach_key_release_event_callback() override;

    /**
     * @brief Attach a new mouse movement event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    void attach_mouse_movement_event_callback(const mouse_movement_event_fn& fn) override;

    /**
     * @brief Detach the currently attached mouse movement event callback.
     */
    void detach_mouse_movement_event_callback() override;

    /**
     * @brief Attach a new mouse button press event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    void attach_mouse_button_press_event_callback(const mouse_button_press_event_fn& fn) override;

    /**
     * @brief Detach the currently attached mouse button press event callback.
     */
    void detach_mouse_button_press_event_callback() override;

    /**
     * @brief Attach a new mouse button release event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    void attach_mouse_button_release_event_callback(const mouse_button_release_event_fn& fn) override;

    /**
     * @brief Detach the currently attached mouse button release event callback.
     */
    void detach_mouse_button_release_event_callback() override;

    /**
     * @brief Attach a new window move event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    void attach_window_move_event_callback(const window_move_event_fn& fn) override;

    /**
     * @brief Detach the currently attached window move event callback.
     */
    void detach_window_move_event_callback() override;

    /**
     * @brief Attach a new window resize event callback.
     * @details This will override the currently attached callback (if any callback is attached). Callbacks are not
     *          handled as a list. There can only be one callback.
     * @param fn The callback to attach.
     */
    void attach_window_resize_event_callback(const window_resize_event_fn& fn) override;

    /**
     * @brief Detach the currently attached window resize event callback.
     */
    void detach_window_resize_event_callback() override;

    /**
     * @brief Poll the platform event queue until no more events are found.
     * @details This empties the platform event queue and dispatches events to their corresponding subsystems.
     */
    void drain_event_queue() override;
  };

}

#endif
