/**
 * @file system.hpp
 * @brief System class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_SYSTEM_HPP
#define OBERON_LINUX_SYSTEM_HPP

#include <array>
#include <string>
#include <vector>

#include "../system.hpp"
#include "../memory.hpp"
#include "../errors.hpp"
#include "../keys.hpp"

#include "x11.hpp"
#include "vk.hpp"

namespace oberon::linux {

  class input;
  class window;

  /**
   * @brief The Linux implementation of the system object.
   */
  class system final : public oberon::system {
  private:
    std::string m_instance_name{ };
    std::string m_application_name{ };

    ptr<Display> m_x_display{ };
    ptr<xcb_connection_t> m_x_connection{ };
    ptr<xcb_screen_t> m_x_screen{ };
    ptr<xkb_context> m_xkb_context{ };
    u8 m_xkb_first_event{ };
    u8 m_xi_major_opcode{ };
    xcb_input_device_id_t m_xi_master_keyboard_id{ };
    xcb_input_device_id_t m_xi_master_pointer_id{ };
    std::array<xcb_atom_t, OBERON_LINUX_X_ATOM_MAX> m_x_atoms{ };
    vkfl::loader m_vkdl{ vkGetInstanceProcAddr };
    VkInstance m_vk_instance{ };
    VkDebugUtilsMessengerEXT m_vk_debug_messenger{ };

    xcb_intern_atom_cookie_t begin_intern_atom(const cstring name);
    xcb_atom_t end_intern_atom(const xcb_intern_atom_cookie_t request);
  public:
    /**
     * @brief Create a new system object.
     * @param instance_name The name of the specific instance of the application. This should be acquired via the
     *                      "-name" program option, the "RESOURCE_NAME" environment variable, or the first value in
     *                      the "argv" array passed to the main function. When multiple values are supplied the
     *                      precedence must be the value provided with "-name", the value of "RESOURCE_NAME",
     *                      and finally argv[0].
     * @param application_name The canonical name of the application.
     * @param desired_layers A list of 0 or more desired Vulkan instance layers.
     */
    system(const std::string& instance_name, const std::string& application_name,
           const std::vector<std::string>& desired_layers);

    /// @cond
    system(const system& other) = delete;
    system(system&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a system object.
     */
    ~system() noexcept;

    /// @cond
    system& operator=(const system& rhs) = delete;
    system& operator=(system& rhs) = delete;
    /// @endcond


    std::filesystem::path executable_path() const override;

    /**
     * @brief Retrieve the instance name associated with the application.
     * @return The value of the instance name as-if retrieved from the WM_CLASS window property.
     */
    std::string instance_name() const;

    /**
     * @brief Retrieve the application name associated with the application.
     * @return The value of the application name as-if retrieved from the WM_CLASS window property.
     */
    std::string application_name() const;

    /**
     * @brief Retrieve the XCB connection.
     * @return A pointer to the underlying XCB connection object.
     */
    ptr<xcb_connection_t> connection();

    /**
     * @brief Retrieve the default XCB screen.
     * @return A pointer to the default XCB screen selected during initialization.
     */
    ptr<xcb_screen_t> default_screen();

    /**
     * @brief Retrieve the unique integer ID corresponding to the root window.
     * @return An XCB window ID that can be used to reference the root window of the X server.
     */
    xcb_window_t root_window_id();

    /**
     * @brief Retrieve the X Atom with the given name.
     * @return A valid Atom XID or XCB_NONE (0) if the name was not found.
     */
    xcb_atom_t atom_from_name(const x_atom_name name) const;

    /**
     * @brief Retrieve the current XKB keyboard context.
     * @return A pointer to the XKB keyboard context.
     */
    ptr<xkb_context> keyboard_context();

    /**
     * @brief Retrieve the XKB keyboard ID.
     * @return The core keyboard ID provided by the system.
     */
    xcb_input_device_id_t keyboard();
    xcb_input_device_id_t pointer();

    /**
     * @brief Retrieve the event code indicating an XKB event.
     * @return An 8-bit value indicating when an XKB event has been received.
     */
    u8 xkb_event_code() const;

    /**
     * @brief Retrieve the major opcode indicating XInput 2 events.
     * @return An 8-bit major opcode.
     */
    u8 xi_major_opcode() const;

    VkInstance instance();
    vkfl::loader& vk_dl();
  };

}

#endif
