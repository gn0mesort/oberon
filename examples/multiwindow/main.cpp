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
  auto win = oberon::render_window{ device, device.name(), { { 0, 0 }, { 1920, 1080 } } };
  auto win2 = oberon::render_window{ device, device.name(), { { 0, 0 }, { 640, 360 } } };
  win.show();
  win2.show();
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
  {
  auto cam = oberon::camera{ device, proj, { 0, 0, 0 } };
  auto cam2 = oberon::camera{ device, proj, { 0, 0, 0 } };

  auto sw = oberon::stopwatch{ };
  auto dt = oberon::stopwatch::duration{ };
  auto quit = false;
  auto ev = oberon::event{ };
  auto cam_pos = oberon::glm::vec3{ 0.0f, 0.0f, 5.0f };
  cam.look_at(cam_pos, { 0.0f, 0.0f, 0.0f });
  cam2.look_at({ 0.0f, 0.0f, 2.0f }, { 0.0f, 0.0f, 0.0f });
  win.change_active_camera(cam);
  win2.change_active_camera(cam2);
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
    cube.rotate(oberon::glm::radians(30.0f) * dt.count(), { 0.0f, 1.0f, 0.0f });
    win.draw(cube);
    win2.draw(cube);
    win.swap_buffers();
    win2.swap_buffers();
    dt = sw.reset();
  }
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
