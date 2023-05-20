#include "oberon/window.hpp"

#include "configuration.hpp"

#if defined(CONFIGURATION_OPERATING_SYSTEM_LINUX) && defined(CONFIGURATION_WINDOW_SYSTEM_X11)
  #include "oberon/internal/linux/x11/window_impl.hpp"

  #define WINDOW_PTR internal::linux::x11::window_impl{ device, title, bounds }
#else
  #error A complete configuration requires window support.
#endif

namespace oberon {

  window::window(graphics_device& device, const std::string& title, const rect_2d& bounds) :
  m_impl{ new WINDOW_PTR } { }

  window::implementation_type& window::implementation() {
    return *m_impl;
  }

  u32 window::id() const {
    return m_impl->id();
  }

  void window::show() {
    m_impl->show();
  }

  void window::hide() {
    m_impl->hide();
  }

  bool window::is_shown() const {
    return m_impl->is_shown();
  }

  bool window::is_minimized() const {
    return m_impl->is_minimized();
  }

  void window::change_display_style(const display_style style) {
    return m_impl->change_display_style(style);
  }

  display_style window::current_display_style() const {
    return m_impl->current_display_style();
  }

  rect_2d window::current_drawable_rect() const {
    return m_impl->current_drawable_rect();
  }

  rect_2d window::current_rect() const {
    return m_impl->current_rect();
  }

  void window::move_to(const offset_2d& offset) {
    m_impl->move_to(offset);
  }

  void window::resize(const extent_2d& extent) {
    m_impl->resize(extent);
  }

  void window::change_title(const std::string& title) {
    m_impl->change_title(title);
  }

  std::string window::title() const {
    return m_impl->title();
  }

  event window::poll_events() {
    return m_impl->poll_events();
  }

  key window::translate_keycode(const u32 code) const {
    return m_impl->translate_keycode(code);
  }

  mouse_button window::translate_mouse_buttoncode(const u32 code) const {
    return m_impl->translate_mouse_buttoncode(code);
  }

  bool window::is_modifier_pressed(const modifier_key modifier) const {
    return m_impl->is_modifier_pressed(modifier);
  }

  bool window::is_key_pressed(const key k) const {
    return m_impl->is_key_pressed(k);
  }

  bool window::is_key_echoing(const key k) const {
    return m_impl->is_key_echoing(k);
  }

  bool window::is_key_just_pressed(const key k) const {
    return is_key_pressed(k) && !is_key_echoing(k);
  }

  bool window::is_mouse_button_pressed(const mouse_button mb) const {
    return m_impl->is_mouse_button_pressed(mb);
  }

  const std::unordered_set<presentation_mode>& window::available_presentation_modes() const {
    return m_impl->available_presentation_modes();
  }

  void window::request_presentation_mode(const presentation_mode mode) {
    m_impl->request_presentation_mode(mode);
  }

  presentation_mode window::current_presentation_mode() const {
    return m_impl->current_presentation_mode();
  }

}
