#include <cstdio>

#include <unistd.h>

#include "oberon/oberon.hpp"

void toggle_fullscreen(oberon::window& win) {
  if (win.current_display_style() == oberon::window::display_style::windowed)
  {
    std::printf("Toggling fullscreen.\n");
    win.change_display_style(oberon::window::display_style::fullscreen_composited);
  }
  else if (win.current_display_style() == oberon::window::display_style::fullscreen_composited)
  {
    std::printf("Toggling windowed.\n");
    win.change_display_style(oberon::window::display_style::windowed);
  }
}

void on_key_press(oberon::environment& env) {
  auto& inpt = env.input();
  auto& win = env.window();
  if (inpt.key_is_pressed(oberon::key::escape))
  {
    win.request_quit();
  }
  if (inpt.key_is_just_pressed(oberon::key::enter) && inpt.modifier_key_is_active(oberon::modifier_key::alt))
  {
    toggle_fullscreen(win);
  }
}

int app_run(oberon::environment& env) {
  auto& win = env.window();
  env.attach_key_event_callback(on_key_press);
  win.resize({ 1280, 720 });
  win.show();
  while (!win.quit_requested())
  {
    env.drain_event_queue();
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  return app.run(app_run);
}
