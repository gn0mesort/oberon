/**
 * @file window.hpp
 * @brief Window class.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_LINUX_WINDOW_HPP
#define OBERON_LINUX_WINDOW_HPP

#include "../window.hpp"

#include "x11.hpp"
#include "vk.hpp"

namespace oberon::linux {

  class system;

  /**
   * @brief The Linux implementation of the window object.
   */
  class window final : public oberon::window {
  private:
    ptr<system> m_parent{ };
    xcb_window_t m_window_id{ };
    display_style m_display_style{ };
    bool m_quit_requested{ };
    VkSurfaceKHR m_vk_surface{ };

    void wm_send_message(const xcb_atom_t atom, const std::array<u32, 5>& message);
    void wm_change_state(const wm_state_mode mode, const xcb_atom_t first, const xcb_atom_t second);
    void wm_change_compositor_mode(const compositor_mode mode);
    void wm_unlock_resize();
    void wm_lock_resize(const window_extent& size);
    void wm_set_title(const std::string& title);
  public:
    /**
     * @brief Create a new window object.
     * @param sys The parent system object used to create the window.
     */
    window(system& sys);

    /// @cond
    window(const window& other) = delete;
    window(window&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a window object.
     */
    ~window() noexcept;

    /// @cond
    window& operator=(const window& rhs) = delete;
    window& operator=(window&& rhs) = delete;
    /// @endcond

    /**
     * @brief Retrieve an integer ID uniquely identifying the window.
     * @return A non-zero ID representing the calling window.
     */
    id unique_id() const override;

    /**
     * @brief Change the window display style.
     * @param style The new display style to switch to.
     * @return A reference back to the calling window.
     */
    window& change_display_style(const display_style style) override;

    /**
     * @brief Retrieve the current display style.
     * @return The current display style (e.g., windowed).
     */
    display_style current_display_style() const override;

    /**
     * @brief Show (i.e., display) the window.
     * @return A reference back to the calling window.
     */
    window& show() override;

    /**
     * @brief Hide (i.e., do not display) the window.
     * @return A reference back to the calling window.
     */
    window& hide() override;

    /**
     * @brief Check whether or not the window is visible.
     * @return True if the window is shown. Otherwise false.
     */
    bool is_visible() const override;

    /**
     * @brief Resize the drawable area of the window.
     * @param size The new size of the drawable area.
     * @return A reference back to the calling window.
     */
    window& resize(const window_extent& size) override;

    /**
     * @brief Move the window to a specific position.
     * @param position The new position for the window.
     * @return A reference back to the calling window.
     */
    window& move_to(const window_offset& position) override;

    /**
     * @brief Retrieve the rectangle representing the drawable area of the window.
     * @return A rectangle in the form { { x, y }, { width, height } }.
     */
    window_rect current_drawable_rect() const override;

    /**
     * @brief Retrieve the rectangle representing the total area of the window.
     * @return A rectangle in the form { { x, y }, { width, height } }.
     */
    window_rect current_rect() const override;

    /**
     * @brief Change the window title.
     * @param title The new window title.
     * @return A reference back to the calling window.
     */
    window& change_title(const std::string& title) override;

    /**
     * @brief Retrieve the current window title.
     * @return The current window title.
     */
    std::string current_title() const override;

    /**
     * @brief Check whether or not the window has received a signal that if should quit.
     * @return True if a quit signal has been received. Otherwise false.
     */
    bool quit_requested() const override;

    /**
     * @brief Signal to the window that the application should quit.
     * @return A reference back to the calling window.
     */
    window& request_quit() override;

    /**
     * @brief Clear a pending window quit signal.
     * @return A reference back to the calling window.
     */
    window& clear_quit_request() override;
  };

}

#endif
