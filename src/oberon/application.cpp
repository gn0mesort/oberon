#include "oberon/application.hpp"

#include "oberon/debug.hpp"
#include "oberon/io_subsystem.hpp"
#include "oberon/graphics_subsystem.hpp"

namespace oberon {

  context::context(const ptr<io_subsystem> io, const ptr<graphics_subsystem> graphics) :
  m_io{ io }, m_graphics{ graphics } {
    OBERON_POSTCONDITION(m_io);
    OBERON_POSTCONDITION(m_graphics);
  }

  io_subsystem& context::io() {
    return *m_io;
  }

  graphics_subsystem& context::graphics() {
    return *m_graphics;
  }

  int application::run(const ptr<entry_point> application_entry) {
    auto result_code = int{ 0 };
    {
      // Build up the application context.
      // Dependencies between subsystems matter.
      auto io = new io_subsystem{ };
      auto graphics = new graphics_subsystem{ *io, 0 };

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
