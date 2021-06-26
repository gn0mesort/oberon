#include <cstdio>

#include <unistd.h>

#include <oberon/errors.hpp>
#include <oberon/debug_context.hpp>
#include <oberon/bounds.hpp>
#include <oberon/events.hpp>
#include <oberon/window.hpp>

int main() {
  try
  {
    auto ctx = oberon::debug_context{
      "window_create",
      1, 0, 0,
      { "VK_LAYER_KHRONOS_validation" }
    };
    auto window = oberon::window{ ctx, { { 0, 0 }, { 320, 240 } } };
    auto ev = oberon::event{ };
    std::printf("Created window %" PRIdMAX ".\n", window.id());
    std::printf("Initial window size %zux%zu.\n", window.width(), window.height());
    while (!window.should_close())
    {
      while (ctx.poll_events(ev))
      {
        window.notify(ev);
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
  }
  catch (const oberon::error& err)
  {
    std::fprintf(stderr, "%s\n", err.message());
    return err.result();
  }
  return 0;
}
