#include <cstdio>

#include <unistd.h>

#include <oberon/errors.hpp>
#include <oberon/debug_context.hpp>
#include <oberon/bounds.hpp>
#include <oberon/events.hpp>
#include <oberon/window.hpp>
#include <oberon/renderer_3d.hpp>

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
    while (!win.should_close())
    {
      while (ctx.poll_events(ev))
      {
        win.notify(ev);
        if (ev.type == oberon::event_type::window_configure)
        {
          std::printf(
            "Window %" PRIdMAX " ( %zu, %zu, %zu, %zu, %s )\n",
            ev.window_id,
            ev.data.window_configure.bounds.position.x, ev.data.window_configure.bounds.position.y,
            ev.data.window_configure.bounds.size.width, ev.data.window_configure.bounds.size.height,
            ev.data.window_configure.override_wm_redirection ? "true" : "false"
          );
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
