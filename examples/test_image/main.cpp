/**
 * @file main.cpp
 * @brief "Test Image" example program.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include "oberon/oberon.hpp"

void toggle_fullscreen(oberon::window& win) {
  if (win.current_display_style() == oberon::display_style::windowed)
  {
    win.change_display_style(oberon::display_style::fullscreen_composited);
  }
  else if (win.current_display_style() == oberon::display_style::fullscreen_composited)
  {
    win.change_display_style(oberon::display_style::windowed);
  }
}

void toggle_immediate_present(oberon::window& win) {
  if (win.current_presentation_mode() == oberon::presentation_mode::fifo)
  {
    win.request_presentation_mode(oberon::presentation_mode::immediate);
  }
  else
  {
    win.request_presentation_mode(oberon::presentation_mode::fifo);
  }
}

bool on_key_press(oberon::window& win) {
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
  auto win = oberon::window{ device, device.name(), { { 0, 0 }, { 1280, 720 } } };
  win.show();
  auto render = oberon::renderer{ device, win };
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
    auto frame = render.begin_frame(win);
    frame.draw_test_image();
    render.end_frame(std::move(frame));
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  const auto renderdoc_device_uuid = std::getenv("RENDERDOC_DEVICE_UUID");
  if (renderdoc_device_uuid)
  {
    app.set_exclusive_device_uuid(renderdoc_device_uuid);
    app.enable_exclusive_device_mode(true);
  }
  return app.run(app_run, argc, argv);
}
