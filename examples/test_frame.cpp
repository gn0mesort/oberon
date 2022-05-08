#include <cstdio>

#include <oberon/application.hpp>
#include <oberon/bounds.hpp>
#include <oberon/window.hpp>

int oberon_main(oberon::context& ctx) {
  using namespace oberon;

  auto win = window{ ctx, "Hello, X11", { { 100, 100 }, { 640, 480 } } };
  win.show();
  while (true);
  return 0;
}

int main() {
  try
  {
    auto app = oberon::application{ };
    return app.run(oberon_main);
  }
  catch (const oberon::error& err)
  {
    std::fprintf(stderr, "Error %s: %s\n", err.type(), err.message());
    return err.result();
  }
  catch (const std::exception& err)
  {
    std::fprintf(stderr, "Error: %s\n", err.what());
    return 1;
  }
}
