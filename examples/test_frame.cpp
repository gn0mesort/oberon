#include <cstdio>

#include <unistd.h>

#include <oberon/application.hpp>
#include <oberon/bounds.hpp>
#include <oberon/window.hpp>
#include <oberon/events.hpp>

int oberon_main(oberon::context& ctx) {
  using namespace oberon;

  auto win = window{ ctx, "Hello, X11", { { 100, 100 }, { 640, 480 } } };
  auto ev = event_variant{ };
  win.show();
  while (!(win.get_signals() & window_signal_bits::destroy_bit))
  {
    while ((ev = wait_for_event(ctx)).index())
    {
      std::visit([&](auto&& arg){
        using EventType = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<EventType, window_message_event>)
        {
          win.accept_message(reinterpret_cast<window_message_event&&>(arg).data);
        }
      }, ev);
    }
  }
  win.hide();

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
