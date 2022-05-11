#ifndef OBERON_EVENTS_HPP
#define OBERON_EVENTS_HPP

#include <functional>

#include "basics.hpp"

namespace oberon::detail {

  class io_subsystem;

  void default_window_message_handler(const u32 window_id, const ptr<void> message);

}

namespace oberon {

  class context;

  class event_dispatcher final {
  public:
    using window_message_handler = std::function<void(const u32 window_id, const ptr<void> message)>;
  private:
    ptr<detail::io_subsystem> m_io{ };

    window_message_handler m_window_message_handler{ detail::default_window_message_handler };

    bool process_event(const ptr<void> ev);
  public:

    event_dispatcher(context& ctx);
    event_dispatcher(const event_dispatcher& other) = delete;
    event_dispatcher(event_dispatcher&& other) = delete;

    ~event_dispatcher() noexcept = default;

    event_dispatcher& operator=(const event_dispatcher& rhs) = delete;
    event_dispatcher& operator=(event_dispatcher&& rhs) = delete;

    void set_window_message_handler(const window_message_handler& handler);

    bool poll_for_event();
    void wait_for_event();
  };

}

#endif
