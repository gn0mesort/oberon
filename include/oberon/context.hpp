#ifndef OBERON_CONTEXT_HPP
#define OBERON_CONTEXT_HPP

namespace oberon {

  class context {
  public:
    inline virtual ~context() noexcept = 0;
  };

  context::~context() noexcept { }

  class onscreen_context : public context {
  public:
    inline virtual ~onscreen_context() noexcept = 0;
  };

  onscreen_context::~onscreen_context() noexcept { }

  class offscreen_context : public context {
  public:
    inline virtual ~offscreen_context() noexcept = 0;
  };

  offscreen_context::~offscreen_context() noexcept { }

}

#endif
