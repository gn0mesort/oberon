#include "oberon/internal/base/window_impl.hpp"

#include "oberon/window.hpp"

namespace oberon::internal::base {

  void window_impl_dtor::operator()(ptr<window_impl> p) const noexcept {
    delete p;
  }

}
