#include "oberon/interfaces/disposable.hpp"

#include "oberon/debug.hpp"

namespace oberon {
namespace interfaces {

  void disposable::dispose() noexcept {
    if (!is_disposed())
    {
      v_dispose();
      v_set_disposed();
    }
    OBERON_POSTCONDITION(is_disposed());
  }

  bool disposable::is_disposed() const noexcept {
    return v_is_disposed();
  }

}
}
