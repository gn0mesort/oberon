#include <cstdio>

#include <memory>

#include <unistd.h>

#include <oberon/application.hpp>
#include <oberon/io_subsystem.hpp>

int oberon_main(oberon::context& ctx) {
  using namespace oberon;

  for (auto i = usize{ 0 }; i < X_ATOM_MAX; ++i)
  {
    std::printf("0x%08x\n", ctx.io().x_atom(static_cast<x_atom>(i)));
  }
  return 0;
}

int main() {
  auto app = oberon::application{ };
  return app.run(oberon_main);
}
