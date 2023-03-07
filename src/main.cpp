/**
 * @file main.cpp
 * @brief Main application entry point.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include <iostream>
#include <iomanip>

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

int app_run(const int, const oberon::ptr<oberon::csequence>, oberon::platform& plt) {
  auto& win = plt.window();
  plt.attach_key_press_event_callback(on_key_press);
  win.resize({ 1280, 720 });
  win.show();
  auto i = 0;
  for (const auto& gfx_device : plt.graphics().available_devices())
  {
    std::cout << "Graphics Device " << i++ <<  ": " << std::endl
              << "\t  Type: " << oberon::to_string(gfx_device.type) << std::endl
              << "\t  Name: " << gfx_device.name << std::endl
              << "\tDriver: " << gfx_device.driver_name << std::endl
              << "\t  Info: " << gfx_device.driver_info << std::endl
              << "\tHandle: 0x" << std::hex << gfx_device.handle << std::dec << std::endl;
  }
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
