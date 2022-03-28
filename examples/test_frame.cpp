#include <cstdio>

#include <unistd.h>

#include <oberon/system.hpp>
#include <oberon/render_window.hpp>

int oberon_main(oberon::system& sys, oberon::context& ctx) {
  auto window = oberon::render_window{ ctx, { { 0, 0 }, { 640, 480 } } };
  window.show();
  sleep(5);
  return 0;
}

int main() {
#ifndef NDEBUG
  constexpr auto debug = true;
#else
  constexpr auto debug = false;
#endif
  auto sys = oberon::system{ };
  sys.set_hint(oberon::system::hint::debug_context, debug);
  return sys.run(oberon_main);
}
