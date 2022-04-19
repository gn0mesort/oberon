#include "oberon/linux/application.hpp"

#include "oberon/context.hpp"
#include "oberon/debug.hpp"

#include "oberon/linux/io_subsystem.hpp"
#include "oberon/linux/graphics_subsystem.hpp"


#include <array>
#include <iostream>

namespace oberon::linux {

  void application::subsystem_factory::v_create_subsystems(oberon::detail::subsystem_storage& subsystems,
                                                           const readonly_ptr<void> config) {
    OBERON_PRECONDITION(subsystems.io == nullptr);
    OBERON_PRECONDITION(subsystems.gfx == nullptr);
    const auto& app_config = *static_cast<readonly_ptr<application::config>>(config);
    auto io = new io_subsystem{ app_config.io.x_display };
    subsystems.io = io;
    subsystems.gfx = new graphics_subsystem{ *io, app_config.gfx.vk_device_index,
                                             app_config.gfx.vk_create_debug_instance };
    OBERON_POSTCONDITION(subsystems.io != nullptr);
    OBERON_POSTCONDITION(subsystems.gfx != nullptr);
  }

  void application::subsystem_factory::v_destroy_subsystems(oberon::detail::subsystem_storage& subsystems) noexcept {
    if (subsystems.gfx)
    {
      delete static_cast<ptr<graphics_subsystem>>(subsystems.gfx);
      subsystems.gfx = nullptr;
    }
    if (subsystems.io)
    {
      delete static_cast<ptr<io_subsystem>>(subsystems.io);
      subsystems.io = nullptr;
    }
    OBERON_POSTCONDITION(subsystems.io == nullptr);
    OBERON_POSTCONDITION(subsystems.gfx == nullptr);
  }

  void application::set_user_data(const ptr<void> user) {
    m_config.user_data = user;
  }

  void application::set_vulkan_debug_flag(const bool flag) {
    m_config.gfx.vk_create_debug_instance = flag;
  }

  void application::set_vulkan_device(const u32 index) {
    m_config.gfx.vk_device_index = index;
  }

  int application::run(const ptr<entry_point> main) const {
    auto status = 0;
    auto ctx = static_cast<ptr<context>>(nullptr);
    try
    {
      ctx = (subsystem_factory{ }).create_context(reinterpret_cast<readonly_ptr<void>>(&m_config));
      status = main(*ctx, m_config.user_data);
    }
    catch (const error& err)
    {
      std::cerr << "Error: " << err.message() << std::endl;
      status = err.result();
    }
    catch (const std::exception& err)
    {
      std::cerr << "Error: " << err.what() << std::endl;
      status = 1;
    }
    (subsystem_factory{ }).destroy_context(ctx);
    return status;
  }

}
