#include <cstdio>

#include <unistd.h>

#include <oberon/linux/application.hpp>

int oberon_main(oberon::context& ctx, const oberon::ptr<void>) {
  return 0;
}

int main() {
  auto app = oberon::linux::application{ };
#ifndef NDEBUG
  app.set_vulkan_debug_flag(true);
#endif
  return app.run(oberon_main);
}
