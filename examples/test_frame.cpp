#include <cstdio>

#include <unistd.h>

#include <oberon/application.hpp>
#include <oberon/bounds.hpp>
#include <oberon/window.hpp>
#include <oberon/events.hpp>

int oberon_main(oberon::context& ctx) {
  using namespace oberon;
  auto win = window{ ctx, "Hello, X11", { { 200, 200 }, { 640, 480 } } };
  auto events = event_dispatcher{ ctx };
  events.set_window_message_handler([&win](const u32, const ptr<void> message) {
    win.accept_message(message);
  });
  win.show();
  while (!win.is_destroy_signaled())
  {
    events.wait_for_event();
  }

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
