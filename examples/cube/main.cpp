/**
 * @file main.cpp
 * @brief "Cube" example program.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include <cstdlib>

#include <oberon/oberon.hpp>

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
  auto win = oberon::window{ device, device.name(), { { 0, 0 }, { 1920, 1080 } } };
  win.show();
  auto render = oberon::renderer{ device, win };
  auto vertices = std::array<float, 288>{
     0.5, -0.5, -0.5, 1.0,  1.0, 0.0, 0.0, 1.0,
     0.5,  0.5, -0.5, 1.0,  1.0, 0.0, 0.0, 1.0,
    -0.5,  0.5, -0.5, 1.0,  1.0, 0.0, 0.0, 1.0,
    -0.5,  0.5, -0.5, 1.0,  1.0, 0.0, 0.0, 1.0,
    -0.5, -0.5, -0.5, 1.0,  1.0, 0.0, 0.0, 1.0,
     0.5, -0.5, -0.5, 1.0,  1.0, 0.0, 0.0, 1.0,

    -0.5,  0.5,  0.5, 1.0,  0.0, 1.0, 0.0, 1.0,
     0.5,  0.5,  0.5, 1.0,  0.0, 1.0, 0.0, 1.0,
     0.5, -0.5,  0.5, 1.0,  0.0, 1.0, 0.0, 1.0,
     0.5, -0.5,  0.5, 1.0,  0.0, 1.0, 0.0, 1.0,
    -0.5, -0.5,  0.5, 1.0,  0.0, 1.0, 0.0, 1.0,
    -0.5,  0.5,  0.5, 1.0,  0.0, 1.0, 0.0, 1.0,

    -0.5, -0.5,  0.5, 1.0,  0.0, 0.0, 1.0, 1.0,
    -0.5, -0.5, -0.5, 1.0,  0.0, 0.0, 1.0, 1.0,
    -0.5,  0.5, -0.5, 1.0,  0.0, 0.0, 1.0, 1.0,
    -0.5,  0.5, -0.5, 1.0,  0.0, 0.0, 1.0, 1.0,
    -0.5,  0.5,  0.5, 1.0,  0.0, 0.0, 1.0, 1.0,
    -0.5, -0.5,  0.5, 1.0,  0.0, 0.0, 1.0, 1.0,

     0.5,  0.5, -0.5, 1.0,  1.0, 0.0, 1.0, 1.0,
     0.5, -0.5, -0.5, 1.0,  1.0, 0.0, 1.0, 1.0,
     0.5, -0.5,  0.5, 1.0,  1.0, 0.0, 1.0, 1.0,
     0.5, -0.5,  0.5, 1.0,  1.0, 0.0, 1.0, 1.0,
     0.5,  0.5,  0.5, 1.0,  1.0, 0.0, 1.0, 1.0,
     0.5,  0.5, -0.5, 1.0,  1.0, 0.0, 1.0, 1.0,

    -0.5,  0.5, -0.5, 1.0,  1.0, 1.0, 0.0, 1.0,
     0.5,  0.5, -0.5, 1.0,  1.0, 1.0, 0.0, 1.0,
     0.5,  0.5,  0.5, 1.0,  1.0, 1.0, 0.0, 1.0,
     0.5,  0.5,  0.5, 1.0,  1.0, 1.0, 0.0, 1.0,
    -0.5,  0.5,  0.5, 1.0,  1.0, 1.0, 0.0, 1.0,
    -0.5,  0.5, -0.5, 1.0,  1.0, 1.0, 0.0, 1.0,

     0.5, -0.5,  0.5, 1.0,  0.0, 1.0, 1.0, 1.0,
     0.5, -0.5, -0.5, 1.0,  0.0, 1.0, 1.0, 1.0,
    -0.5, -0.5, -0.5, 1.0,  0.0, 1.0, 1.0, 1.0,
    -0.5, -0.5, -0.5, 1.0,  0.0, 1.0, 1.0, 1.0,
    -0.5, -0.5,  0.5, 1.0,  0.0, 1.0, 1.0, 1.0,
     0.5, -0.5,  0.5, 1.0,  0.0, 1.0, 1.0, 1.0
  };
  auto cube = oberon::mesh{ device, oberon::vertex_type::position_color, vertices };
  const auto proj = oberon::glm::perspective(oberon::glm::radians(106.0f), 16.0f / 9.0f, 0.1f, 100.0f);
  auto cam = oberon::camera{ proj };

  auto sw = oberon::stopwatch{ };
  auto dt = oberon::stopwatch::duration{ };
  auto quit = false;
  auto ev = oberon::event{ };
  cam.look_at({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 0.0f });
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
    cube.rotate(oberon::glm::radians(30.0f) * dt.count(), { 0.0f, 1.0f, 0.0f });
    frame.draw(cam, cube);
    render.end_frame(std::move(frame));
    dt = sw.reset();
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
