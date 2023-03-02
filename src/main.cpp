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
  if (env.input.key_is_pressed(oberon::key::escape))
  {
    env.window.request_quit();
  }
  if (env.input.key_is_just_pressed(oberon::key::enter) && env.input.modifier_key_is_active(oberon::modifier_key::alt))
  {
    toggle_fullscreen(env.window);
  }
}

int app_run(oberon::environment& env) {
  env.system.attach_key_event_callback(on_key_press);
  env.window.resize({ 1280, 720 });
  env.window.show();
  while (!env.window.quit_requested())
  {
    env.system.drain_event_queue();
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  return app.run(app_run);
}
