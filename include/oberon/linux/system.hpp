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
    std::unordered_set<std::filesystem::path> m_search_paths{ };

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
     * @param additional_search_paths A set of 0 or more additional search paths to use with find_file.
     * @param desired_layers A list of 0 or more desired Vulkan instance layers.
     */
    system(const std::string& instance_name, const std::string& application_name,
           const std::unordered_set<std::filesystem::path>& additional_search_paths,
           const std::unordered_set<std::string>& desired_layers);

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


    /**
     * @brief Add a path to the set of additional search paths.
     * @param path The path to add.
     */
    void add_additional_search_path(const std::filesystem::path& path) override;

    /**
     * @brief Remove a path from the set of additional search paths.
     * @param path The path to remove.
     */
    void remove_additional_search_path(const std::filesystem::path& path) override;

    /**
     * @brief Retrieve the set of additional search paths.
     * @return A set of additional search paths used by the system.
     */
    const std::unordered_set<std::filesystem::path>& additional_search_paths() const override;

    /**
     * @brief Retrieve the system home directory.
     * @return $HOME or the user's home directory from the passwd file if $HOME is undefined.
     */
    std::filesystem::path home_directory() const override;

    /**
     * @brief Retrieve the path of the currently executing binary.
     * @return A path to the currently executing binary.
     */
    std::filesystem::path executable_path() const override;

    /**
     * @brief Retrieve the path to the directory in which the currently executing binary resides.
     * @return A path to the directory in which the currently executing binary can be found.
     */
    std::filesystem::path executable_directory() const override;

    /**
     * @brief Retrieve the system immutable data directory.
     * @return The value of executable_directory().parent_path() / "share" / application_name(). This directory does
     *         not require write access.
     */
    std::filesystem::path immutable_data_directory() const override;

    /**
     * @brief Retrieve the system mutable data directory.
     * @return The value of $XDG_DATA_HOME or "$HOME/.local/share" / application_name() if $XDG_DATA_HOME is
     *         undefined.
     */
    std::filesystem::path mutable_data_directory() const override;

    /**
     * @brief Retrieve the system cache directory.
     * @return The value of $XDG_CACHE_HOME or "$HOME/.cache" / application_name() if $XDG_CACHE_HOME is undefined.
     */
    std::filesystem::path cache_directory() const override;

    /**
     * @brief Retrieve the system configuration directory.
     * @return The value of $XDG_CONFIG_HOME or "$HOME/.config" / application_name() if $XDG_CONFIG_HOME is undefined.
     */
    std::filesystem::path configuration_directory() const override;

    /**
     * @brief Find a file by searching a set of paths.
     * @details This searches for the file named by name in any of the paths found in search. Implementations must
     *          make it clear what character will be used to separate strings in search. POSIX systems commonly use
     *          ':' and Windows commonly uses ';'. In any case, once separated searching proceeds from the first path
     *          until the file is found or the set of paths is exhausted.
     * @param search A set of ':' separated search paths.
     * @param name The name of the file to find. This can contain additional path elements.
     */
    std::filesystem::path find_file(const std::string& search, const std::string& name) const override;

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
    std::filesystem::path find_file(const default_file_location location, const std::string& name) const override;

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
     * @brief Retrieve the master keyboard ID.
     * @return The master keyboard ID provided by the system.
     */
    xcb_input_device_id_t keyboard();

    /**
     * @brief Retrieve the master pointer ID.
     * @return The master pointer ID provided by the system.
     */
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

    /**
     * @brief Retrieve the system Vulkan instance.
     * @return The system Vulkan instance handle.
     */
    VkInstance instance();

    /**
     * @brief Retrieve the system Vulkan function table.
     * @return a mutable reference to the Vulkan function table.
     */
    vkfl::loader& vk_dl();
  };

}

#endif
