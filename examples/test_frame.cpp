#include <cstdio>

#include <memory>

#include <unistd.h>

#include <oberon/context.hpp>
#include <oberon/window.hpp>
#include <oberon/bounds.hpp>
#include <oberon/linux/application.hpp>

int oberon_main(oberon::context& ctx, const oberon::ptr<void>) {
  auto win = std::unique_ptr<oberon::abstract_window>{ ctx.create_window("Hello, world!", { { 0, 0 }, { 640, 480 } }) };
  win->show();
  sleep(5);
  win->hide();
  return 0;
}

int main() {
  auto app = oberon::linux::application{ };
#ifndef NDEBUG
  app.set_vulkan_debug_flag(true);
#endif
  return app.run(oberon_main);
}
