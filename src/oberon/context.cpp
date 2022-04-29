#include "oberon/context.hpp"

#include "oberon/errors.hpp"
#include "oberon/io_subsystem.hpp"
#include "oberon/graphics_subsystem.hpp"
#include "oberon/window.hpp"
#include "oberon/bounds.hpp"

#include "oberon/linux/window.hpp"

namespace oberon::detail {

  ptr<context> subsystem_factory::create_context(const readonly_ptr<void> config) {
    auto ctx = new context{ };
    v_create_subsystems(ctx->m_subsystems, config);
    return ctx;
  }

  void subsystem_factory::destroy_context(const ptr<context> ctx) noexcept {
    v_destroy_subsystems(ctx->m_subsystems);
    delete ctx;
  }

}

namespace oberon {

  abstract_subsystem& context::get_subsystem(const subsystem_type type) {
    switch (type)
    {
    case subsystem_type::io:
      return *m_subsystems.io;
    case subsystem_type::graphics:
      return *m_subsystems.gfx;
    default:
      throw not_implemented_error{ };
    }
  }

  abstract_io_subsystem& context::get_io_subsystem() {
    return *m_subsystems.io;
  }

  abstract_graphics_subsystem& context::get_graphics_subsystem() {
    return *m_subsystems.gfx;
  }

  ptr<abstract_window> context::create_window(const std::string_view title, const bounding_rect& bounds) {
    if (m_subsystems.io->implementation() == subsystem_implementation::linux_xcb &&
        m_subsystems.gfx->implementation() == subsystem_implementation::vulkan)
    {
      return new oberon::linux::window{ *this, title, bounds };
    }
    else
    {
      throw not_implemented_error{ };
    }
  }

}
