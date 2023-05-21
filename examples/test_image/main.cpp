/**
 * @file main.cpp
 * @brief "Test Image" example program.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/oberon.hpp"

void toggle_fullscreen(oberon::window& win) {
  if (win.display_style() == oberon::display_style::windowed)
  {
    win.display_style(oberon::display_style::fullscreen_composited);
  }
  else if (win.display_style() == oberon::display_style::fullscreen_composited)
  {
    win.display_style(oberon::display_style::windowed);
  }
}

void toggle_immediate_present(oberon::window& win) {
  if (win.presentation_mode() == oberon::presentation_mode::fifo)
  {
    win.presentation_mode(oberon::presentation_mode::immediate);
  }
  else
  {
    win.presentation_mode(oberon::presentation_mode::fifo);
  }
}

bool on_key_press(oberon::window& win) {
  if (win.is_key_just_pressed(oberon::key::escape))
  {
    win.hide();
    return true;
  }
  if (win.is_key_just_pressed(oberon::key::enter) && win.is_modifier_key_active(oberon::modifier_key::alt))
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
  auto win = oberon::window{ device, device.name(), { { 0, 0 }, { 1280, 720 } } };
  win.show();
  auto render = oberon::renderer{ device, win, 4 };
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
    auto frame = render.begin_frame();
    frame.draw_test_image();
    render.end_frame(win, std::move(frame));
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  const auto renderdoc_device_uuid = std::getenv("RENDERDOC_DEVICE_UUID");
  if (renderdoc_device_uuid)
  {
    return app.run_device_exclusive(renderdoc_device_uuid, app_run, argc, argv);
  }
  return app.run(app_run, argc, argv);
}
