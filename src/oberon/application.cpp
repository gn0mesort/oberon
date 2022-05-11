#include "oberon/application.hpp"

#include "oberon/debug.hpp"

#include "oberon/detail/io_subsystem.hpp"
#include "oberon/detail/graphics_subsystem.hpp"

namespace oberon {

  // Post: io, graphics
  context::context(const ptr<detail::io_subsystem> io, const ptr<detail::graphics_subsystem> graphics) :
  m_io{ io }, m_graphics{ graphics } {
    OBERON_POSTCONDITION(m_io);
    OBERON_POSTCONDITION(m_graphics);
  }

  detail::io_subsystem& context::io() {
    return *m_io;
  }

  detail::graphics_subsystem& context::graphics() {
    return *m_graphics;
  }

  int application::run(const entry_point& application_entry) {
    auto result_code = int{ 0 };
    {
      // Build up the application context.
      // Dependencies between subsystems matter.
      auto io = new detail::io_subsystem{ };
      auto graphics = new detail::graphics_subsystem{ *io, 0 };

      // Construct context object, wrap subsystems, and execute application entry point function.
      auto ctx = context{ io, graphics };
      result_code = application_entry(ctx);

      // Tear down the application context.
      // Dependencies still matter.
      delete graphics;
      delete io;
    }
    return result_code;
  }

}
