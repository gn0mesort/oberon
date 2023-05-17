#include "oberon/render_window.hpp"

#include "configuration.hpp"

#include "oberon/internal/linux/x11/render_window_impl.hpp"

namespace oberon {

  render_window::render_window(graphics_device& device, const std::string& title, const rect_2d& bounds) :
#if defined(CONFIGURATION_OPERATING_SYSTEM_LINUX) && defined(CONFIGURATION_WINDOW_SYSTEM_X11)
  m_impl{ new internal::linux::x11::render_window_impl{ device, title, bounds } }
#else
  #error A complete configuration requires render_window support.
#endif
  { }

  u32 render_window::id() const {
    return m_impl->id();
  }

  void render_window::show() {
    m_impl->show();
  }

  void render_window::hide() {
    m_impl->hide();
  }

  bool render_window::is_shown() const {
    return m_impl->is_shown();
  }

  bool render_window::is_minimized() const {
    return m_impl->is_minimized();
  }

  void render_window::change_display_style(const display_style style) {
    return m_impl->change_display_style(style);
  }

  display_style render_window::current_display_style() const {
    return m_impl->current_display_style();
  }

  rect_2d render_window::current_drawable_rect() const {
    return m_impl->current_drawable_rect();
  }

  rect_2d render_window::current_rect() const {
    return m_impl->current_rect();
  }

  void render_window::move_to(const offset_2d& offset) {
    m_impl->move_to(offset);
  }

  void render_window::resize(const extent_2d& extent) {
    m_impl->resize(extent);
  }

  void render_window::change_title(const std::string& title) {
    m_impl->change_title(title);
  }

  std::string render_window::title() const {
    return m_impl->title();
  }

  event render_window::poll_events() {
    return m_impl->poll_events();
  }

  key render_window::translate_keycode(const u32 code) const {
    return m_impl->translate_keycode(code);
  }

  mouse_button render_window::translate_mouse_buttoncode(const u32 code) const {
    return m_impl->translate_mouse_buttoncode(code);
  }

  bool render_window::is_modifier_pressed(const modifier_key modifier) const {
    return m_impl->is_modifier_pressed(modifier);
  }

  bool render_window::is_key_pressed(const key k) const {
    return m_impl->is_key_pressed(k);
  }

  bool render_window::is_key_echoing(const key k) const {
    return m_impl->is_key_echoing(k);
  }

  bool render_window::is_key_just_pressed(const key k) const {
    return is_key_pressed(k) && !is_key_echoing(k);
  }

  bool render_window::is_mouse_button_pressed(const mouse_button mb) const {
    return m_impl->is_mouse_button_pressed(mb);
  }

  void render_window::draw_test_image() {
    m_impl->draw_test_image();
  }

  void render_window::swap_buffers() {
    m_impl->swap_buffers();
  }

  const std::unordered_set<presentation_mode>& render_window::available_presentation_modes() const {
    return m_impl->available_presentation_modes();
  }

  void render_window::request_presentation_mode(const presentation_mode mode) {
    m_impl->request_presentation_mode(mode);
  }

  presentation_mode render_window::current_presentation_mode() const {
    return m_impl->current_presentation_mode();
  }

  void render_window::change_active_camera(camera& cam) {
    m_impl->change_active_camera(cam);
  }

  void render_window::draw(mesh& m) {
    m_impl->draw(m);
  }

}
