#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include <unordered_set>

#include "memory.hpp"
#include "rects.hpp"
#include "events.hpp"
#include "keys.hpp"
#include "mouse.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(window_impl);

}

namespace oberon {

  class graphics_device;

  enum class display_style {
    windowed = 0,
    fullscreen_composited,
    fullscreen_bypass_compositor
  };

  // These values must be kept inline with VkPresentModeKHR.
  enum class presentation_mode {
    immediate = 0,
    mailbox = 1,
    fifo = 2,
    fifo_relaxed = 3,
    shared_demand_refresh = 1000111000,
    shared_continuous_refresh = 1000111001
  };

  class window final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::window_impl);
  public:
    using implementation_type = internal::base::window_impl;

    window(graphics_device& device, const std::string& title, const rect_2d& bounds);
    window(const window& other) = delete;
    window(window&& other) = delete;

    ~window() noexcept = default;

    window& operator=(const window& rhs) = delete;
    window& operator=(window&& rhs) = delete;

    implementation_type& implementation();

    u32 id() const;
    void show();
    void hide();
    bool is_shown() const;
    bool is_minimized() const;
    void change_display_style(const display_style style);
    display_style current_display_style() const;
    void resize(const extent_2d& extent);
    void move_to(const offset_2d& offset);
    rect_2d current_drawable_rect() const;
    rect_2d current_rect() const;
    void change_title(const std::string& title);
    std::string title() const;
    event poll_events();
    key translate_keycode(const u32 code) const;
    bool is_key_pressed(const key k) const;
    bool is_key_echoing(const key k) const;
    bool is_key_just_pressed(const key k) const;
    bool is_mouse_button_pressed(const mouse_button mb) const;
    mouse_button translate_mouse_buttoncode(const u32 code) const;
    bool is_modifier_pressed(const modifier_key modifier) const;
    const std::unordered_set<presentation_mode>& available_presentation_modes() const;
    void request_presentation_mode(const presentation_mode mode);
    presentation_mode current_presentation_mode() const;
  };

  OBERON_ENFORCE_CONCEPT(concepts::has_internal_implementation, window);

}

#endif
