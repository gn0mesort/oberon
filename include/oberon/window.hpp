#ifndef OBERON_WINDOW_HPP
#define OBERON_WINDOW_HPP

#include "object.hpp"

namespace oberon {
namespace detail {

  struct window_impl;

}
namespace events {

  struct window_expose_data;
  struct window_message_data;
  struct window_configure_data;

}

  class context;
  struct bounding_rect;
  struct extent_2d;
  struct event;

  class window : public object {
  private:
    virtual void v_dispose() noexcept override;
  protected:
    window(const context& ctx, const ptr<detail::window_impl> child_impl);
  public:
    window(const context& ctx);
    window(const context& ctx, const bounding_rect& bounds);

    virtual ~window() noexcept;

    imax id() const;

    bool should_close() const;

    const extent_2d& size() const;
    usize width() const;
    usize height() const;

    window& notify(const event& ev);
    window& notify(const events::window_expose_data& expose);
    window& notify(const events::window_message_data& message);
    window& notify(const events::window_configure_data& configure);
    // window_message translate_message(const events::window_message_data& message) const;
  };

}

#endif
