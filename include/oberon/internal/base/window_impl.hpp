/**
 * @file window_impl.hpp
 * @brief Internal window API.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#ifndef OBERON_INTERNAL_BASE_WINDOW_IMPL_HPP
#define OBERON_INTERNAL_BASE_WINDOW_IMPL_HPP

#include <vector>

#include "../../window.hpp"

#include "vulkan.hpp"


namespace oberon::internal::base {

  /**
   * @class window_impl
   * @brief The base window implementation.
   */
  class window_impl {
  protected:
    window_impl() = default;
  public:
    /// @cond
    window_impl(const window_impl& other) = delete;
    window_impl(window_impl&& other) = delete;
    /// @endcond

    /**
     * @brief Destroy a `window_impl`.
     */
    virtual ~window_impl() noexcept = default;

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
    virtual u32 acquire_next_image(const VkSemaphore acquired) = 0;

    /**
     * @brief Retrieve the list of swapchain images.
     * @return A reference to a list of `VkImage`s that belong to the `window`'s `VkSwapchainKHR`.
     */
    virtual const std::vector<VkImage>& swapchain_images() = 0;

    /**
     * @brief Retrieve the format of the `window`'s `VkSwapchainKHR`.
     * @return The current `VkFormat` of the swapchain.
     */
    virtual VkFormat surface_format() const = 0;

    /**
     * @brief Retrieve the 2D extent of the `window`'s `VkSwapchainKHR`.
     * @return The extent of the swapchain.
     */
    virtual VkExtent2D swapchain_extent() const = 0;

    /**
     * @brief Present the `VkImage` with the given index to the `window`.
     * @param index The index of the `VkImage` to present in the `VkSwapchainKHR`.
     * @param ready_to_present A `VkSemaphore` that should be waited upon before presenting.
     */
    virtual void present_image(const u32 index, const VkSemaphore ready_to_present) = 0;

    /**
     * @brief Retrieve the `window`'s unique id.
     * @return A unique 32-bit integer representing the `window`.
     */
    virtual u32 id() const = 0;

    /**
     * @brief Show the `window`.
     */
    virtual void show() = 0;

    /**
     * @brief Hide the `window`.
     */
    virtual void hide() = 0;

    /**
     * @brief Determine if the `window` is shown or hidden.
     * @return True if the `window` is shown. False if the `window` is hidden.
     */
    virtual bool is_shown() const = 0;

    /**
     * @brief Determine if the `window` is shown but minimized.
     * @detail This is slightly different from the `window` being shown. A shown `window` is potentially visible on
     *         the screen but is definitely known to the window manager.
     * @return True if the `window` is minimized. False if the `window` is not minimized.
     */
    virtual bool is_minimized() const = 0;

    /**
     * @brief Change the current `display_style`.
     * @param style The new `display_style`.
     */
    virtual void display_style(const display_style style) = 0;

    /**
     * @brief Retrieve the current `display_style`.
     * @return The current `display_style`.
     */
    virtual enum display_style display_style() const = 0;

    /**
     * @brief Retrieve the geometry of the `window`'s drawable region.
     * @return A `rect_2d` describing the drawable region.
     */
    virtual rect_2d drawable_rect() const = 0;

    /**
     * @brief Retrieve the geometry of the `window`'s total screen region.
     * @return A `rect_2d` describing the screen region.
     */
    virtual rect_2d screen_rect() const = 0;

    /**
     * @brief Change the `window`'s title.
     * @param title The new `window` title.
     */
    virtual void title(const std::string& title) = 0;

    /**
     * @brief Retrieve the `window`'s title.
     * @return The `window`'s title.
     */
    virtual std::string title() const = 0;

    /**
     * @brief Resize the `window`.
     * @param extent The new `window` size.
     */
    virtual void resize(const extent_2d& extent) = 0;

    /**
     * @brief Move the `window`.
     * @param offset The new position of the `window`.
     */
    virtual void move_to(const offset_2d& offset) = 0;

    /**
     * @brief Poll for window system events.
     * @detail If there are no pending events this returns immediately.
     * @return The next available `window` event or an empty event if none are available.
     */
    virtual event poll_events() = 0;

    /**
     * @brief Determine whether or not the given `key` is pressed.
     * @param k The `key` to determine the state of.
     * @return True if the `key` is pressed. False if the `key` is not pressed.
     */
    virtual bool is_key_pressed(const key k) const = 0;

    /**
     * @brief Determine whether or not the given `key` is sending echo press events.
     * @param k The `key` to determine the state of.
     * @return True if the `key` is echoing. False if the `key` is not echoing.
     */
    virtual bool is_key_echoing(const key k) const = 0;

    /**
     * @brief Determine whether or not a `mouse_button` is pressed.
     * @param mb The `mouse_button` to determine the state of.
     * @return True if the `mouse_button` is pressed. False if the `mouse_button` is not pressed.
     */
    virtual bool is_mouse_button_pressed(const mouse_button mb) const = 0;

    /**
     * @brief Translate a keycode into a `key` value.
     * @param code The keycode to translate.
     * @return The corresponding `key` value.
     */
    virtual key translate_keycode(const u32 code) const = 0;

    /**
     * @brief Translate a buttoncode into a `mouse_button`.
     * @param code The code to translate.
     * @return The corresponding `mouse_button`.
     */
    virtual mouse_button translate_mouse_buttoncode(const u32 code) const = 0;

    /**
     * @brief Determine if a `modifier_key` is active.
     * @param modifier The `modifier_key` to determine the state of.
     * @return True if the `modifier_key` is active (i.e., pressed, locked, or latched). False if the `modifer_key` is
     *         not active.
     */
    virtual bool is_modifier_key_active(const modifier_key modifier) const = 0;

    /**
     * @brief Retrieve a set of `presentation_mode`s available to the `window`.
     * @return A set of `presentation_mode`s available to the `window`.
     */
    virtual const std::unordered_set<enum presentation_mode>& available_presentation_modes() const = 0;

    /**
     * @brief Request a change in the `window`'s `presentation_mode`.
     * @detail A `window` may reject the client's chosen mode. In this case the mode will be changed to
     *        `presentation_mode::fifo`.
     * @param mode The `presentation_mode` to request.
     */
    virtual void presentation_mode(const enum presentation_mode mode) = 0;

    /**
     * @brief Retrieve the current `presentation_mode` of the `window`.
     * @return The current `presentation_mode`.
     */
    virtual enum presentation_mode presentation_mode() const = 0;
  };

}

#endif
