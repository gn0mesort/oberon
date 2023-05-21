/**
 * @file main.cpp
 * @brief "Cube" example program.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include <cstdlib>

#include <iostream>

#include <oberon/oberon.hpp>

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
  using namespace oberon::fundamental_types;

  auto& device = sys.preferred_graphics_device();
  auto win = oberon::window{ device, device.name(), { { 0, 0 }, { 1920, 1080 } } };
  auto win2 = oberon::window{ device, device.name(), { { 0, 0 }, { 640, 360 } } };
  win.show();
  win2.show();
  auto render = oberon::renderer{ device, win, 4 };
  auto render2 = oberon::renderer{ device, win2, 4 };
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
  auto cam2 = oberon::camera{ proj };
  auto sw = oberon::stopwatch{ };
  auto dt = oberon::stopwatch::duration{ };
  auto quit = false;
  auto ev = oberon::event{ };
  auto cam_pos = oberon::glm::vec3{ 0.0f, 0.0f, 5.0f };
  cam.look_at(cam_pos, { 0.0f, 0.0f, 0.0f });
  cam2.look_at({ 0.0f, -1.0f, 2.0f }, { 0.0f, 0.0f, 0.0f });
  constexpr auto MOVE_SPEED = 5.0f;
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
        if (win.is_key_pressed(oberon::key::character_w))
        {
          cam_pos.y -= MOVE_SPEED * dt.count();
        }
        if (win.is_key_pressed(oberon::key::character_s))
        {
          cam_pos.y += MOVE_SPEED * dt.count();
        }
        if (win.is_key_pressed(oberon::key::character_a))
        {
          cam_pos.z -= MOVE_SPEED * dt.count();
        }
        if (win.is_key_pressed(oberon::key::character_d))
        {
          cam_pos.z += MOVE_SPEED * dt.count();
        }
        if (win.is_key_just_pressed(oberon::key::tab))
        {
        }
        cam.look_at(cam_pos, { 0.0f, 0.0f, 0.0f });
        break;
      default:
        break;
      }
    }
    while ((ev = win2.poll_events()))
    {
      switch (ev.type)
      {
      case oberon::event_type::window_close:
        win2.hide();
        break;
      case oberon::event_type::key_press:
        on_key_press(win2);
        break;
      default:
        break;
      }
    }
    auto frame = render.begin_frame();
    auto frame2 = render2.begin_frame();
    cube.rotate(oberon::glm::radians(30.0f) * dt.count(), { 0.0f, 1.0f, 0.0f });
    frame.draw(cam, cube);
    frame2.draw(cam2, cube);
    render.end_frame(win, std::move(frame));
    render2.end_frame(win2, std::move(frame2));
    dt = sw.reset();
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
