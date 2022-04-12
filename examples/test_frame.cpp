#include <cstdio>

#include <unistd.h>

#include <oberon/linux/application.hpp>
#include <oberon/linux/render_window.hpp>
#include <oberon/debug.hpp>

int oberon_main(oberon::linux::window_system& win_sys, oberon::linux::render_system& rnd_sys,
                const oberon::ptr<void> user) {
  auto window = oberon::linux::render_window{ win_sys, rnd_sys, "Hello, world!", { { 0, 0 }, { 640, 480 } } };
  window.show();
  sleep(5);
  return 0;
}

int main() {
  auto app = oberon::linux::application{ };
#ifndef NDEBUG
  app.set_vulkan_debug_flag(true);
#endif
  return app.run(oberon_main);
}
