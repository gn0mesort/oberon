/**
 * @file main.cpp
 * @brief "Test Image" example program.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/oberon.hpp"

void toggle_fullscreen(oberon::render_window& win) {
  if (win.current_display_style() == oberon::display_style::windowed)
  {
    win.change_display_style(oberon::display_style::fullscreen_composited);
  }
  else if (win.current_display_style() == oberon::display_style::fullscreen_composited)
  {
    win.change_display_style(oberon::display_style::windowed);
  }
}

void toggle_immediate_present(oberon::render_window& win) {
  if (win.current_presentation_mode() == oberon::presentation_mode::fifo)
  {
    win.request_presentation_mode(oberon::presentation_mode::immediate);
  }
  else
  {
    win.request_presentation_mode(oberon::presentation_mode::fifo);
  }
}

bool on_key_press(oberon::render_window& win) {
  if (win.is_key_just_pressed(oberon::key::escape))
  {
    win.hide();
    return true;
  }
  if (win.is_key_just_pressed(oberon::key::enter) && win.is_modifier_pressed(oberon::modifier_key::alt))
  {
    toggle_fullscreen(win);
  }
  if (win.is_key_just_pressed(oberon::key::insert))
  {
    toggle_immediate_present(win);
  }
  return false;
}

int app_run(const int, const oberon::ptr<oberon::csequence>, oberon::system& sys) {
  auto& device = sys.preferred_graphics_device();
  auto win = oberon::render_window{ device, device.name(), { { 0, 0 }, { 1280, 720 } } };
  win.show();
  auto quit = false;
  auto ev = oberon::event{ };
  while (!quit)
  {
    while ((ev = win.poll_events()))
    {
      switch (ev.type)
      {
      case oberon::event_type::window_close:
        win.hide();
        quit = true;
        break;
      case oberon::event_type::key_press:
        quit = on_key_press(win);
        break;
      default:
        break;
      }
    }
    win.draw_test_image();
    win.swap_buffers();
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  return app.run(app_run, argc, argv);
}
