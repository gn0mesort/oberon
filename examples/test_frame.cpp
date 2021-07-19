#include <cstdio>

#include <oberon/errors.hpp>
#include <oberon/debug_context.hpp>
#include <oberon/bounds.hpp>
#include <oberon/events.hpp>
#include <oberon/window.hpp>
#include <oberon/renderer_3d.hpp>

oberon::cstring to_string(const bool b) {
  return b ? "true" : "false";
}

int main() {
  try
  {
    auto ctx = oberon::debug_context{
      "Test Frame",
      1, 0, 0,
      { "VK_LAYER_KHRONOS_validation" }
    };
    auto win = oberon::window{ ctx, { { 0, 0 }, { 640, 480 } } };
    auto rnd = oberon::renderer_3d{ win };
    auto ev = oberon::event{ };
    auto quit = false;
    while (!quit)
    {
      while (ctx.poll_events(ev))
      {
        switch (ev.type)
        {
        case oberon::event_type::window_hide:
          quit = true;
          break;
        default:
          break;
        }
      }
      if (rnd.should_rebuild())
      {
        rnd.rebuild();
      }
      rnd.begin_frame();
      rnd.draw_test_frame();
      rnd.end_frame();
    }
    rnd.dispose();
    win.dispose();
    ctx.dispose();
  }
  catch (const oberon::error& err)
  {
    std::fprintf(stderr, "%s\n", err.message());
    return err.result();
  }
  return 0;
}
