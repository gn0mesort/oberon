#include <cstdio>

#include <unistd.h>

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
      "window_create",
      1, 0, 0,
      { "VK_LAYER_KHRONOS_validation" }
    };
    auto win = oberon::window{ ctx, { { 0, 0 }, { 320, 240 } } };
    auto rnd = oberon::renderer_3d{ win };
    auto ev = oberon::event{ };
    std::printf("Created window %" PRIdMAX ".\n", win.id());
    std::printf("Initial window size %zux%zu.\n", win.width(), win.height());
    auto quit = false;
    while (!quit)
    {
      while (ctx.poll_events(ev))
      {
        switch (ev.type)
        {
        case oberon::event_type::window_configure:
          std::printf(
            "Window reconfigured: { bounds: { position: { %zd, %zd }, size: { %zu, %zu } }, %s, %s }\n",
            ev.data.window_configure.bounds.position.x, ev.data.window_configure.bounds.position.y,
            ev.data.window_configure.bounds.size.width, ev.data.window_configure.bounds.size.height,
            to_string(ev.data.window_configure.was_repositioned), to_string(ev.data.window_configure.was_resized)
          );
          break;
        case oberon::event_type::window_hide:
          quit = true;
          break;
        default:
          break;
        }
      }
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
