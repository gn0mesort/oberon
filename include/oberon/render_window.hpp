#ifndef OBERON_RENDER_WINDOW_HPP
#define OBERON_RENDER_WINDOW_HPP

#include <string>

#include "memory.hpp"
#include "rects.hpp"
#include "events.hpp"
#include "keys.hpp"
#include "mouse.hpp"

#include "concepts/has_internal_implementation.hpp"

namespace oberon::internal::base {

  OBERON_OPAQUE_BASE_FWD(render_window_impl);

}

namespace oberon {

  enum class display_style {
    windowed = 0,
    fullscreen_composited,
    fullscreen_bypass_compositor
  };

  class graphics_device;

  class render_window final {
  private:
    OBERON_OPAQUE_BASE_PTR(internal::base::render_window_impl);
  public:
    using implementation_type = internal::base::render_window_impl;

    render_window(graphics_device& device, const std::string& title, const rect_2d& bounds);
    render_window(const render_window& other) = delete;
    render_window(render_window&& other) = default;

    ~render_window() noexcept = default;

    render_window& operator=(const render_window& rhs) = delete;
    render_window& operator=(render_window&& rhs) = default;

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
  };

  OBERON_ENFORCE_CONCEPT(has_internal_implementation, render_window);

}

#endif
