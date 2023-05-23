/**
 * @file window_impl.hpp
 * @brief Internal Linux+X11 window API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_LINUX_X11_WINDOW_IMPL_HPP
#define OBERON_INTERNAL_LINUX_X11_WINDOW_IMPL_HPP

#include <array>
#include <vector>

#include <nng/nng.h>

#include "../../../types.hpp"

#include "../../base/vulkan.hpp"
#include "../../base/graphics_device_impl.hpp"
#include "../../base/window_impl.hpp"

#include "xcb.hpp"
#include "keys.hpp"

namespace oberon {

  class graphics_device;

}

namespace oberon::internal::linux::x11 {

  class graphics_device_impl;

namespace query_visibility_flag_bits {

  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(mapped, 0);
  OBERON_DEFINE_BIT(iconic, 1);
  OBERON_DEFINE_BIT(hidden, 2);

}

namespace renderer_status_flag_bits {

  OBERON_DEFINE_ZERO_BIT(none);
  OBERON_DEFINE_BIT(dirty, 0);

}

  /**
   * @class window_impl
   * @brief The Linux+X11 window implementation.
   */
  class window_impl final : public base::window_impl {
  private:
    struct key_state final {
      bool pressed{ };
      bool echoing{ };
    };

    // I know this is just 1 bool.
    // std::vector<bool> has weird behavior.
    struct mouse_button_state final {
      bool pressed{ };
    };

    ptr<graphics_device_impl> m_parent{ };
    xcb_window_t m_window{ };
    ptr<xkb_keymap> m_keyboard_map{ };
    ptr<xkb_state> m_keyboard_state{ };
    std::array<xcb_keycode_t, MAX_KEY> m_to_keycode{ };
    // This is a little weird. The number of xcb_keycode_t -> oberon::key mappings is the number of unique
    // xcb_keycode_t values that can be represented (i.e., 256 and not 255). The reason this isn't MAX_KEY is that
    // MAX_KEY is the number of valid usize -> xcb_keycode_t mappings (i.e., a number closer to 128).
    std::array<oberon::key, std::numeric_limits<xcb_keycode_t>::max() + 1> m_to_external_key{ };
    std::array<xkb_mod_index_t, MAX_MODIFIER_KEY> m_to_modifier_index{ };
    std::array<key_state, MAX_KEY> m_key_states{ };
    std::vector<mouse_button_state> m_mouse_button_states{ };

    nng_socket m_socket{ };
    nng_dialer m_dialer{ };

    VkSurfaceKHR m_surface{ };
    std::unordered_set<enum presentation_mode> m_presentation_modes{ };
    VkPresentModeKHR m_swapchain_present_mode{ VK_PRESENT_MODE_FIFO_KHR };
    VkExtent2D m_swapchain_extent{ };
    VkSurfaceFormatKHR m_swapchain_surface_format{ };
    VkSwapchainKHR m_swapchain{ };
    std::vector<VkImage> m_swapchain_images{ };
    std::vector<VkImageView> m_swapchain_image_views{ };
    bitmask m_status{ };


    void send_client_message(const xcb_window_t destination, const xcb_generic_event_t& message);
    void change_size_hints(const xcb_size_hints_t& hints);
    void initialize_keyboard();
    void deinitialize_keyboard();
    bitmask query_visibility() const;
    void change_ewmh_states(const ewmh_state_action action, const xcb_atom_t first, const xcb_atom_t second);
    void change_compositor_mode(const compositor_mode mode);
    void initialize_swapchain(const VkSwapchainKHR old);
  public:
    /**
     * @brief Create a `window_impl`.
     * @param device The `graphics_device` on which the `window` will be based.
     * @param title  The title of the `window`.
     * @param bounds The bounds of the `window`.
     */
    window_impl(graphics_device& device, const std::string& title, const rect_2d& bounds);

    /// @cond
    window_impl(const window_impl& other) = delete;
    window_impl(window_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `window_impl`.
     */
    ~window_impl() noexcept;

    /// @cond
    window_impl& operator=(const window_impl& other) = delete;
    window_impl& operator=(window_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Acquire a `VkImage` from the `window`'s `VkSwapchainKHR`.
     * @details Although an image is acquired it may still be pending use (i.e., reads) so it is important to await
     *          the signalling of the `acquired` semaphore before writing to the acquired image.
     * @param acquired A `VkSemaphore` to signal when the acquired `VkImage` is ready to render to.
     * @return The 32-bit index of the newly acquired image.
     */
    u32 acquire_next_image(const VkSemaphore acquired) override;

    /**
     * @brief Retrieve the list of swapchain images.
     * @return A reference to a list of `VkImage`s that belong to the `window`'s `VkSwapchainKHR`.
     */
    const std::vector<VkImage>& swapchain_images() override;

    /**
     * @brief Retrieve the format of the `window`'s `VkSwapchainKHR`.
     * @return The current `VkFormat` of the swapchain.
     */
    VkFormat surface_format() const override;

    /**
     * @brief Retrieve the 2D extent of the `window`'s `VkSwapchainKHR`.
     * @return The extent of the swapchain.
     */
    VkExtent2D swapchain_extent() const override;

    /**
     * @brief Present the `VkImage` with the given index to the `window`.
     * @param index The index of the `VkImage` to present in the `VkSwapchainKHR`.
     * @param ready_to_present A `VkSemaphore` that should be waited upon before presenting.
     */
    void present_image(const u32 index, const VkSemaphore ready_to_present) override;

    /**
     * @brief Retrieve the `window`'s unique id.
     * @return A unique 32-bit integer representing the `window`.
     */
    u32 id() const override;

    /**
     * @brief Show the `window`.
     */
    void show() override;

    /**
     * @brief Hide the `window`.
     */
    void hide() override;

    /**
     * @brief Determine if the `window` is shown or hidden.
     * @return True if the `window` is shown. False if the `window` is hidden.
     */
    bool is_shown() const override;

    /**
     * @brief Determine if the `window` is shown but minimized.
     * @detail This is slightly different from the `window` being shown. A shown `window` is potentially visible on
     *         the screen but is definitely known to the window manager.
     * @return True if the `window` is minimized. False if the `window` is not minimized.
     */
    bool is_minimized() const override;

    /**
     * @brief Change the current `display_style`.
     * @param style The new `display_style`.
     */
    void display_style(const enum display_style style) override;

    /**
     * @brief Retrieve the current `display_style`.
     * @return The current `display_style`.
     */
    enum display_style display_style() const override;

    /**
     * @brief Retrieve the geometry of the `window`'s drawable region.
     * @return A `rect_2d` describing the drawable region.
     */
    rect_2d drawable_rect() const override;

    /**
     * @brief Retrieve the geometry of the `window`'s total screen region.
     * @return A `rect_2d` describing the screen region.
     */
    rect_2d screen_rect() const override;

    /**
     * @brief Change the `window`'s title.
     * @param title The new `window` title.
     */
    void title(const std::string& title) override;

    /**
     * @brief Retrieve the `window`'s title.
     * @return The `window`'s title.
     */
    std::string title() const override;

    /**
     * @brief Resize the `window`.
     * @param extent The new `window` size.
     */
    void resize(const extent_2d& extent) override;

    /**
     * @brief Move the `window`.
     * @param offset The new position of the `window`.
     */
    void move_to(const offset_2d& offset) override;

    /**
     * @brief Poll for window system events.
     * @detail If there are no pending events this returns immediately.
     * @return The next available `window` event or an empty event if none are available.
     */
    event poll_events() override;

    /**
     * @brief Translate a keycode into a `key` value.
     * @param code The keycode to translate.
     * @return The corresponding `key` value.
     */
    oberon::key translate_keycode(const u32 code) const override;

    /**
     * @brief Translate a buttoncode into a `mouse_button`.
     * @param code The code to translate.
     * @return The corresponding `mouse_button`.
     */
    oberon::mouse_button translate_mouse_buttoncode(const u32 code) const override;

    /**
     * @brief Determine if a `modifier_key` is active.
     * @param modifier The `modifier_key` to determine the state of.
     * @return True if the `modifier_key` is active (i.e., pressed, locked, or latched). False if the `modifer_key` is
     *         not active.
     */
    bool is_modifier_key_active(const oberon::modifier_key modifier) const override;

    /**
     * @brief Determine whether or not the given `key` is pressed.
     * @param k The `key` to determine the state of.
     * @return True if the `key` is pressed. False if the `key` is not pressed.
     */
    bool is_key_pressed(const oberon::key k) const override;

    /**
     * @brief Determine whether or not the given `key` is sending echo press events.
     * @param k The `key` to determine the state of.
     * @return True if the `key` is echoing. False if the `key` is not echoing.
     */
    bool is_key_echoing(const oberon::key k) const override;

    /**
     * @brief Determine whether or not a `mouse_button` is pressed.
     * @param mb The `mouse_button` to determine the state of.
     * @return True if the `mouse_button` is pressed. False if the `mouse_button` is not pressed.
     */
    bool is_mouse_button_pressed(const oberon::mouse_button mb) const override;

    /**
     * @brief Retrieve a set of `presentation_mode`s available to the `window`.
     * @return A set of `presentation_mode`s available to the `window`.
     */
    const std::unordered_set<enum presentation_mode>& available_presentation_modes() const override;

    /**
     * @brief Request a change in the `window`'s `presentation_mode`.
     * @detail A `window` may reject the client's chosen mode. In this case the mode will be changed to
     *        `presentation_mode::fifo`.
     * @param mode The `presentation_mode` to request.
     */
    void presentation_mode(const enum presentation_mode mode) override;

    /**
     * @brief Retrieve the current `presentation_mode` of the `window`.
     * @return The current `presentation_mode`.
     */
    enum presentation_mode presentation_mode() const override;
  };

}

#endif
