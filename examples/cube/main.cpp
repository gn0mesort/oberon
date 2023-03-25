/**
 * @file main.cpp
 * @brief "Test Image" example program.
 * @author Alexander Rothman <gnomesort@megate.ch>
 * @date 2023
 * @copyright AGPL-3.0+
 */
#include <chrono>

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

void toggle_immediate_present(oberon::graphics& gfx) {
  if (gfx.current_presentation_mode() == oberon::presentation_mode::fifo)
  {
    gfx.request_presentation_mode(oberon::presentation_mode::immediate);
  }
  else
  {
    gfx.request_presentation_mode(oberon::presentation_mode::fifo);
  }
}

void on_key_press(oberon::platform& plt, const oberon::u32, const oberon::key, const bool) {
  auto& inpt = plt.input();
  auto& win = plt.window();
  auto& gfx = plt.graphics();
  if (inpt.key_is_pressed(oberon::key::escape))
  {
    win.request_quit();
  }
  if (inpt.key_is_just_pressed(oberon::key::enter) && inpt.modifier_key_is_active(oberon::modifier_key::alt))
  {
    toggle_fullscreen(win);
  }
  if (inpt.key_is_just_pressed(oberon::key::insert))
  {
    toggle_immediate_present(gfx);
  }
}

int app_run(const int, const oberon::ptr<oberon::csequence>, oberon::platform& plt) {
  auto& win = plt.window();
  auto& gfx = plt.graphics();
  plt.attach_key_press_event_callback(on_key_press);
  win.resize({ 1280, 720 });
  win.show();
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
  auto& buf = gfx.allocate_buffer(oberon::buffer_type::vertex, vertices.size() * sizeof(float));
  auto ub = oberon::uniform_buffer{ };
  ub.model = oberon::identity<oberon::mat4>();
  ub.model = oberon::translate(ub.model, { 0, 0, 0 });
  ub.view = oberon::lookAt(oberon::vec3{ 0.0f, 0.0f, 5.0f }, oberon::vec3{ 0.0f, 0.0f, 0.0f }, oberon::vec3{ 0.0f, 1.0f, 0.0f });
  ub.projection = oberon::perspective(106.0f, 16.0f / 9.0f, 0.1f, 100.0f);
  buf.write(vertices);
  auto start = decltype(std::chrono::steady_clock::now()){ };
  auto end = decltype(std::chrono::steady_clock::now()){ };
  auto dt = std::chrono::duration<float>{ };
  while (!win.quit_requested())
  {
    start = std::chrono::steady_clock::now();
    plt.drain_event_queue();
    ub.model = oberon::rotate(ub.model, oberon::radians(30.0f) * dt.count(), { 0.0f, 1.0f, 0.0f });
    ub.model = oberon::rotate(ub.model, oberon::radians(30.0f) * dt.count(), { 1.0f, 0.0f, 0.0f });
    gfx.write_uniform_buffer(ub);
    gfx.draw_buffer_unlit_pc(buf);
    gfx.submit_and_present_frame();
    end = std::chrono::steady_clock::now();
    dt = end - start;
  }
  gfx.flush_device_queues();
  gfx.free_buffer(buf);
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  app.add_initial_search_path(MESON_BUILD_DIRECTORY);
  return app.run(app_run, argc, argv);
}
