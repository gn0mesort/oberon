/**
 * @file main.cpp
 * @brief Main application entry point.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

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
  if (inpt.key_is_just_pressed(oberon::key::character_d))
  {
    plt.graphics().dirty_renderer();
  }
}

void on_window_resize(oberon::platform& plt, const oberon::window_extent& extent) {
  std::cout << "Rebuilding Vulkan Swapchain with extent: { " << extent.width << ", " << extent.height
            << " }" << std::endl;
  plt.graphics().dirty_renderer();
}

void print_rect(const oberon::window_rect& rect) {
  std::cout << "Current Rect: { {" << rect.offset.x << ", " << rect.offset.y << " }, { " << rect.extent.width << ", "
            << rect.extent.height << " } }" << std::endl;
}

int app_run(const int, const oberon::ptr<oberon::csequence>, oberon::platform& plt) {
  auto& win = plt.window();
  plt.attach_key_press_event_callback(on_key_press);
  //plt.attach_window_resize_event_callback(on_window_resize);
  win.resize({ 1280, 720 });
  win.show();
  plt.graphics().open_device(plt.graphics().preferred_device());
  while (!win.quit_requested())
  {
    plt.drain_event_queue();
    plt.graphics().begin_frame();
    plt.graphics().draw_test_image();
    plt.graphics().end_frame();
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  return app.run(app_run, argc, argv);
}
