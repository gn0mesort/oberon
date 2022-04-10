#ifndef OBERON_RENDER_WINDOW_HPP
#define OBERON_RENDER_WINDOW_HPP

namespace oberon {

  class render_window {
  public:
    inline virtual ~render_window() noexcept = 0;
  };

  render_window::~render_window() noexcept { }

  class render_image {
  public:
    inline virtual ~render_image() noexcept = 0;
  };

  render_image::~render_image() noexcept { }

}

#endif
