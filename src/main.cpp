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

void on_key_press(oberon::platform& plt, const oberon::u32, const oberon::key, const bool) {
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

void on_mouse_movement(oberon::platform&, const oberon::mouse_offset& screen_position,
                       const oberon::mouse_offset& window_position) {
  auto [ screen_x, screen_y ] = screen_position;
  auto [ window_x, window_y ] = window_position;
  std::printf("{ %hu, %hu }, { %hu, %hu }\n", screen_x, screen_y, window_x, window_y);
}

void on_mouse_button_press(oberon::platform& plt, const oberon::u32, const oberon::mouse_button) {
  auto& inpt = plt.input();
  std::printf("Left = %s, Middle = %s, Right = %s\n", inpt.mouse_button_is_pressed(oberon::mouse_button::left) ?
      "Pressed" : "Released", inpt.mouse_button_is_pressed(oberon::mouse_button::middle) ? "Pressed" : "Release",
      inpt.mouse_button_is_pressed(oberon::mouse_button::right) ? "Pressed" : "Released");
  if (plt.input().mouse_button_is_pressed(oberon::mouse_button::right))
  {
    plt.window().request_quit();
  }
  if (plt.input().mouse_button_is_pressed(oberon::mouse_button::button_1))
  {
    std::printf("Wow!\n");
  }
}

int app_run(const int, const oberon::ptr<oberon::csequence>, oberon::platform& plt) {
  auto& win = plt.window();
  plt.attach_key_press_event_callback(on_key_press);
  plt.attach_mouse_movement_event_callback(on_mouse_movement);
  plt.attach_mouse_button_press_event_callback(on_mouse_button_press);
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
