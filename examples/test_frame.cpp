#include <cstdio>

#include <memory>

#include <unistd.h>

#include <oberon/application.hpp>
#include <oberon/io_subsystem.hpp>

int oberon_main(oberon::context& ctx) {
  std::printf("0x%08x\n", ctx.io().x_wm_protocols_atom());
  return 0;
}

int main() {
  auto app = oberon::application{ };
  return app.run(oberon_main);
}
