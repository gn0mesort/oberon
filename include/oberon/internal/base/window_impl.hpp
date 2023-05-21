#ifndef OBERON_INTERNAL_BASE_WINDOW_IMPL_HPP
#define OBERON_INTERNAL_BASE_WINDOW_IMPL_HPP

#include <vector>

#include "../../window.hpp"

#include "vulkan.hpp"


namespace oberon::internal::base {

  class window_impl {
  protected:
    window_impl() = default;
  public:
    window_impl(const window_impl& other) = delete;
    window_impl(window_impl&& other) = delete;

    virtual ~window_impl() noexcept = default;

    window_impl& operator=(const window_impl& other) = delete;
    window_impl& operator=(window_impl&& other) = delete;

    virtual u32 acquire_next_image(const VkSemaphore acquired) = 0;
    virtual const std::vector<VkImage>& swapchain_images() = 0;
    virtual VkFormat surface_format() const = 0;
    virtual VkExtent2D swapchain_extent() const = 0;
    virtual void present_image(const u32 index, const VkSemaphore ready_to_present) = 0;
    virtual u32 id() const = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual bool is_shown() const = 0;
    virtual bool is_minimized() const = 0;
    virtual void display_style(const display_style style) = 0;
    virtual enum display_style display_style() const = 0;
    virtual rect_2d drawable_rect() const = 0;
    virtual rect_2d screen_rect() const = 0;
    virtual void title(const std::string& title) = 0;
    virtual std::string title() const = 0;
    virtual void resize(const extent_2d& extent) = 0;
    virtual void move_to(const offset_2d& offset) = 0;
    virtual event poll_events() = 0;
    virtual bool is_key_pressed(const key k) const = 0;
    virtual bool is_key_echoing(const key k) const = 0;
    virtual bool is_mouse_button_pressed(const mouse_button mb) const = 0;
    virtual key translate_keycode(const u32 code) const = 0;
    virtual mouse_button translate_mouse_buttoncode(const u32 code) const = 0;
    virtual bool is_modifier_key_active(const modifier_key modifier) const = 0;
    virtual const std::unordered_set<enum presentation_mode>& available_presentation_modes() const = 0;
    virtual void presentation_mode(const enum presentation_mode mode) = 0;
    virtual enum presentation_mode presentation_mode() const = 0;
  };

}

#endif
