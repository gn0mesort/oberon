#include "oberon/internal/base/render_window_impl.hpp"

#include "oberon/render_window.hpp"

namespace oberon::internal::base {

  void render_window_impl_dtor::operator()(ptr<render_window_impl> p) const noexcept {
    delete p;
  }

}
