#include <cstdio>

#include <oberon/system.hpp>

int oberon_main(oberon::system& sys, oberon::context& ctx) {
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
