/**
 * @file main.cpp
 * @brief Main application entry point.
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

void on_key_press(oberon::platform& plt) {
  auto& inpt = plt.input();
  auto& win = plt.window();
  if (inpt.key_is_pressed(oberon::key::escape))
  {
    win.request_quit();
  }
  if (inpt.key_is_just_pressed(oberon::key::enter) && inpt.modifier_key_is_active(oberon::modifier_key::alt))
  {
    toggle_fullscreen(win);
  }
}

int app_run(const int argc, const oberon::ptr<oberon::csequence> argv, oberon::platform& plt) {
  auto& win = plt.window();
  plt.attach_key_event_callback(on_key_press);
  win.resize({ 1280, 720 });
  win.show();
  while (!win.quit_requested())
  {
    plt.drain_event_queue();
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  return app.run(app_run, argc, argv);
}
