/**
 * @file main.cpp
 * @brief "Test Image" example program.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/oberon.hpp"

void toggle_fullscreen(oberon::window& win) {
  if (win.current_display_style() == oberon::window::display_style::windowed)
  {
    win.change_display_style(oberon::window::display_style::fullscreen_composited);
  }
  else if (win.current_display_style() == oberon::window::display_style::fullscreen_composited)
  {
    win.change_display_style(oberon::window::display_style::windowed);
  }
}

void toggle_immediate_present(oberon::graphics& gfx) {
  if (gfx.current_presentation_mode() == oberon::presentation_mode::fifo)
  {
    gfx.request_presentation_mode(oberon::presentation_mode::immediate);
  }
  else
  {
    gfx.request_presentation_mode(oberon::presentation_mode::fifo);
  }
}

void on_key_press(oberon::platform& plt, const oberon::u32, const oberon::key, const bool) {
  auto& inpt = plt.input();
  auto& win = plt.window();
  auto& gfx = plt.graphics();
  if (inpt.key_is_pressed(oberon::key::escape))
  {
    win.request_quit();
  }
  if (inpt.key_is_just_pressed(oberon::key::enter) && inpt.modifier_key_is_active(oberon::modifier_key::alt))
  {
    toggle_fullscreen(win);
  }
  if (inpt.key_is_just_pressed(oberon::key::insert))
  {
    toggle_immediate_present(gfx);
  }
}

int app_run(const int, const oberon::ptr<oberon::csequence>, oberon::platform& plt) {
  auto& sys = plt.system();
  auto& win = plt.window();
  auto& gfx = plt.graphics();
  sys.add_additional_search_path(MESON_BUILD_DIRECTORY);
  plt.attach_key_press_event_callback(on_key_press);
  win.resize({ 1280, 720 });
  win.show();
  gfx.open_device(gfx.preferred_device());
  gfx.request_presentation_mode(oberon::presentation_mode::fifo);
  while (!win.quit_requested())
  {
    plt.drain_event_queue();
    gfx.begin_frame();
    gfx.draw_test_image();
    gfx.end_frame();
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  return app.run(app_run, argc, argv);
}
