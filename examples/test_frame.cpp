#include <cstdio>

#include <unistd.h>

#include <oberon/application.hpp>
#include <oberon/bounds.hpp>
#include <oberon/window.hpp>
#include <oberon/events.hpp>

int oberon_main(oberon::context& ctx) {
  using namespace oberon;
  auto win_conf = window::config{ };
  win_conf.title = "Hello, X11";
  win_conf.bounds = { { 200, 200 }, { 640, 480 } };
  auto win = window{ ctx, win_conf };
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
    std::fprintf(stderr, "%s (%s:%d): \"%s\"\n", err.type(), err.location().file_name(), err.location().line(),
                 err.message());
    return err.result();
  }
  catch (const std::exception& err)
  {
    std::fprintf(stderr, "exception: \"%s\"\n", err.what());
    return 1;
  }
}
