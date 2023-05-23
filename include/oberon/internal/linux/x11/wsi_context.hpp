/**
 * @file wsi_context.hpp
 * @brief Internal Linux+X11 wsi_context object.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_LINUX_X11_WSI_CONTEXT_HPP
#define OBERON_INTERNAL_LINUX_X11_WSI_CONTEXT_HPP

#include <array>

#include <pthread.h>

#include "../../../types.hpp"
#include "../../../memory.hpp"


#include "xcb.hpp"
#include "atoms.hpp"

namespace oberon::internal::linux::x11 {


  /**
   * @class wsi_context
   * @brief An object representing the system-wide WSI state.
   */
  class wsi_context final {
  private:
    std::string m_instance_name{ };
    std::string m_application_name{ "oberon" };
    ptr<xcb_connection_t> m_connection{ };
    readonly_ptr<xcb_setup_t> m_setup{ };
    ptr<xcb_screen_t> m_default_screen{ };
    u8 m_xi_major_opcode{ };
    u8 m_xkb_first_event{ };
    xcb_input_device_id_t m_xi_master_keyboard_id{ };
    xcb_input_device_id_t m_xi_master_pointer_id{ };
    ptr<xkb_context> m_xkb_context{ };
    std::array<xcb_atom_t, MAX_ATOM> m_atoms{ };
    pthread_t m_wsi_pub_worker{ };
    xcb_window_t m_client_leader{ };

    void send_client_message(const xcb_window_t window, const xcb_client_message_event_t& message);
  public:
    /**
     * @brief Create a `wsi_context`.
     */
    wsi_context();

    /**
     * @brief Create a `wsi_context`.
     * @param instance The name of the application instance.
     */
    explicit wsi_context(const std::string& instance);

    /**
     * @brief Create a `wsi_context`.
     * @param display The X11 connection string.
     * @param instance The name of the application instance.
     */
    wsi_context(const cstring display, const std::string& instance);

    /// @cond
    wsi_context(const wsi_context& other) = delete;
    wsi_context(wsi_context&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `wsi_context`.
     */
    ~wsi_context() noexcept;

    /// @cond
    wsi_context& operator=(const wsi_context& rhs) = delete;
    wsi_context& operator=(wsi_context&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve the X11 connection.
     * @return A pointer to the X11 connection.
     */
    ptr<xcb_connection_t> connection();

    /**
     * @brief Retrieve the default X11 screen.
     * @return The default X11 screen.
     */
    ptr<xcb_screen_t> default_screen();

    /**
     * @brief Retrieve an X11 atom using its symbolic name.
     * @param name The symbolic name of the atom.
     * @return The X11 atom corresponding to the input name.
     */
    xcb_atom_t atom_by_name(const atom name) const;

    /**
     * @brief Retrieve the name of the application instance.
     * @return The name of the application instance.
     */
    const std::string& instance_name() const;

    /**
     * @brief Retrieve the name of the application.
     * @return The name of the application.
     */
    const std::string& application_name() const;

    /**
     * @brief Retrieve the client leader window.
     * @return The X11 window id of the client leader.
     */
    xcb_window_t leader();

    /**
     * @brief Retrieve the XKB keyboard context.
     * @return The XKB keyboard context.
     */
    ptr<xkb_context> keyboard_context();

    /**
     * @brief Retrieve the XInput device ID of the master keyboard.
     * @return The master keyboard's device ID.
     */
    xcb_input_device_id_t keyboard();

    /**
     * @brief Retrieve the XInput device ID of the master pointer.
     * @return The master pointer's device ID.
     */
    xcb_input_device_id_t pointer();

    /**
     * @brief Determine whether an event type is an XKB event.
     * @param type The 8-bit event type.
     * @return True if the type was an XKB event. False in all other cases.
     */
    bool is_keyboard_event(const u8 type) const;
  };

}

#endif
