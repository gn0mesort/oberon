#ifndef OBERON_RENDERER_3D_HPP
#define OBERON_RENDERER_3D_HPP

#include "object.hpp"
#include "dependency.hpp"

namespace oberon {
namespace detail {

  struct renderer_3d_impl;

}

  class window;

  class renderer_3d : public object {
  private:
    virtual void v_dispose() noexcept override;
  protected:
    dependency<window> m_win_dep;

    renderer_3d(window& win, const ptr<detail::renderer_3d_impl> impl);
  public:
    renderer_3d(window& win);

    virtual ~renderer_3d() noexcept;

    bool should_rebuild() const;
    renderer_3d& rebuild();

    renderer_3d& begin_frame();
    renderer_3d& end_frame();
    renderer_3d& draw_test_frame();
  };

}

#endif
