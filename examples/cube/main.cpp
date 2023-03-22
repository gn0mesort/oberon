/**
 * @file main.cpp
 * @brief "Test Image" example program.
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
  auto vertices = std::array<oberon::vertex_pc, 6>{
    oberon::vertex_pc{ { -0.5f, 0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
    oberon::vertex_pc{ { 0.5f, 0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
    oberon::vertex_pc{ { 0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },

    oberon::vertex_pc{ { 0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    oberon::vertex_pc{ { -0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
    oberon::vertex_pc{ { -0.5f, 0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
  };
  auto& buf = gfx.allocate_buffer(oberon::buffer_type::vertex, vertices.size() * sizeof(oberon::vertex_pc));
  auto vertices2 = std::array<oberon::vertex_pc, 3>{
    oberon::vertex_pc{ { -1.0f, 0.5f, 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
    oberon::vertex_pc{ { -0.5, 0.5f, 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
    oberon::vertex_pc{ { -0.5, 0.0f, 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
  };
  auto& buf2 = gfx.allocate_buffer(oberon::buffer_type::vertex, vertices2.size() * sizeof(oberon::vertex_pc));
  buf2.write(vertices2);
  buf.write(vertices);
  while (!win.quit_requested())
  {
    plt.drain_event_queue();
    gfx.draw_buffer_unlit_pc(buf);
    gfx.draw_buffer_unlit_pc(buf2);
    gfx.submit_and_present_frame();
  }
  gfx.flush_device_queues();
  gfx.free_buffer(buf);
  gfx.free_buffer(buf2);
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  app.add_initial_search_path(MESON_BUILD_DIRECTORY);
  return app.run(app_run, argc, argv);
}
