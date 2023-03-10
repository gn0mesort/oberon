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
}

void on_window_resize(oberon::platform& plt, const oberon::window_extent& extent) {
  std::cout << "Rebuilding Vulkan Swapchain with extent: { " << extent.width << ", " << extent.height
            << " }" << std::endl;
  plt.graphics().reinitialize_renderer();
}

std::vector<char> read_shader_binary(const std::filesystem::path& file) {
  auto f_in = std::ifstream{ file, std::ios::binary | std::ios::ate };
  auto sz = f_in.tellg();
  auto buffer = std::vector<char>(sz);
  f_in.seekg(std::ios::beg);
  f_in.read(buffer.data(), buffer.size());
  return buffer;
}

int app_run(const int, const oberon::ptr<oberon::csequence>, oberon::platform& plt) {
  auto& win = plt.window();
  plt.attach_key_press_event_callback(on_key_press);
  plt.attach_window_resize_event_callback(on_window_resize);
  win.resize({ 1280, 720 });
  win.show();
  auto test_frame = oberon::usize{ 0 };
  {
    auto vert_binary = read_shader_binary(plt.system().executable_path() / "data/oberon/shaders/test_frame.vert.spv");
    auto vert_stage = plt.graphics().intern_pipeline_stage_binary(oberon::pipeline_stage::vertex, vert_binary);
    auto frag_binary = read_shader_binary(plt.system().executable_path() / "data/oberon/shaders/test_frame.frag.spv");
    auto frag_stage = plt.graphics().intern_pipeline_stage_binary(oberon::pipeline_stage::fragment, frag_binary);
    auto program = oberon::render_program{ };
    program.vertex_stage_id = vert_stage.id;
    program.fragment_stage_id = frag_stage.id;
    test_frame = plt.graphics().intern_render_program(program);
  }
  while (!win.quit_requested())
  {
    plt.drain_event_queue();
    plt.graphics().begin_frame();
    plt.graphics().draw(3, test_frame);
    plt.graphics().end_frame();
  }
  return 0;
}

int main(int argc, char** argv) {
  auto app = oberon::application{ };
  return app.run(app_run, argc, argv);
}
